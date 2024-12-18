#include "canon.h"

#include <chrono>

void CanonPTPCamera::openSession() {
  PTP::openSession();

  if (!isEos())
    throw PTPCameraException("Unsupported Canon camera.");

  // TODO: Include specific exceptions?
  uint32_t remoteMode = isEosM() ? 0x15 : 0x01;
  mesg(CanonOperationCode::EOSSetRemoteMode, {remoteMode});
  mesg(CanonOperationCode::EOSSetEventMode, {0x01});

  // TODO: Proper event queue
  eosEventThread = std::jthread([this](std::stop_token stoken) {
    while (true) {
      if (stoken.stop_requested())
        return;
      checkEvents();
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
  });

  // TODO: EVF and capture target logic?
  // TODO: Include specific exceptions?
  if (isEosM()) {
    mesg(CanonOperationCode::EOSSetEventMode, {0x02});
    // eosSetDeviceProp(EOSPropertyCode::EVFOutputDevice, 0x08);
  }

  invalidateCachedDI();
}

void CanonPTPCamera::closeSession() {
  if (eosEventThread.joinable()) {
    eosEventThread.request_stop();
    eosEventThread.join();
  }

  // if (isEosM())
  //   eosSetDeviceProp(EOSPropertyCode::EVFOutputDevice, 0x00);

  // Get events?

  mesg(CanonOperationCode::EOSSetRemoteMode, {0x01});
  mesg(CanonOperationCode::EOSSetEventMode, {0x00});

  PTP::closeSession();
}

DeviceInfo CanonPTPCamera::getDeviceInfo() {
  DeviceInfo deviceInfo = PTP::getDeviceInfo();

  deviceInfo.vendorExtensionId =
      static_cast<uint32_t>(VendorExtensionId::Canon);

  if (deviceInfo.isOpSupported(CanonOperationCode::EOSGetDeviceInfoEx)) {
    Buffer data = recv(CanonOperationCode::EOSGetDeviceInfoEx).data;
    EOSDeviceInfo eosDeviceInfo;
    eosDeviceInfo.unpack(data);
    deviceInfo.devicePropertiesSupported.insert(
        deviceInfo.devicePropertiesSupported.end(),
        eosDeviceInfo.devicePropertiesSupported.begin(),
        eosDeviceInfo.devicePropertiesSupported.end());
  }

  return deviceInfo;
}

// TODO: Event monitoring and error checking/handling
void CanonPTPCamera::triggerCapture() {
  if (isOpSupported(CanonOperationCode::EOSRemoteReleaseOn)) {
    mesg(CanonOperationCode::EOSRemoteReleaseOn, {0x03, 0x00});
    // Get events?
    mesg(CanonOperationCode::EOSRemoteReleaseOff, {0x03});
  } else {
    mesg(CanonOperationCode::EOSRemoteRelease);
  }
}

bool CanonPTPCamera::isEos() {
  return isOpSupported(CanonOperationCode::EOSRemoteRelease) ||
         isOpSupported(CanonOperationCode::EOSRemoteReleaseOn);
}

bool CanonPTPCamera::isEosM() {
  if (!isOpSupported(CanonOperationCode::EOSSetRemoteMode))
    return false;

  return getCachedDI()->model.find("Canon EOS M") != std::string::npos;

  // TODO: Include Powershot models with similar firmware?
}

void CanonPTPCamera::checkEvents() {
  Buffer data = recv(CanonOperationCode::EOSGetEvent).data;
  EOSEventData eventData;
  eventData.unpack(data);
  for (Buffer& event : eventData.events) {
    if (auto propChanged = EOSEventPacket::unpackAs<EOSPropChanged>(event)) {
      Logger::log(">> Prop change (propCode=0x%04x, propValue=0x%04x)",
                  propChanged->propertyCode, propChanged->propertyValue);
      if (propChanged->propertyCode == EOSPropertyCode::Aperture) {
        if (auto val =
                CanonApertureValues.findValue(propChanged->propertyValue))
          Logger::log(">> Aperture change (value=%0.1f)",
                      (float)val->first / val->second);
      }
    }
  }
}