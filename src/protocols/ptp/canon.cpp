#include "canon.h"

void CanonPTPCamera::openSession() {
  PTP::openSession();

  if (!isEos())
    throw Exception(ExceptionContext::CameraConnect,
                    ExceptionType::UnsupportedCamera);

  // TODO: Include specific exceptions?
  uint32_t remoteMode = isEosM() ? 0x15 : 0x01;
  mesg(CanonOperationCode::EOSSetRemoteMode, {remoteMode});
  mesg(CanonOperationCode::EOSSetEventMode, {0x01});

  // TODO: EVF and capture target logic?
  // TODO: Include specific exceptions?
  if (isEosM()) {
    mesg(CanonOperationCode::EOSSetEventMode, {0x02});
    // eosSetDeviceProp(EOSPropertyCode::EVFOutputDevice, 0x08);
  }

  invalidateCachedDI();

  startEventThread();
}

void CanonPTPCamera::closeSession() {
  stopEventThread();

  // if (isEosM())
  //   eosSetDeviceProp(EOSPropertyCode::EVFOutputDevice, 0x00);

  // Get events?

  mesg(CanonOperationCode::EOSSetRemoteMode, {0x01});
  mesg(CanonOperationCode::EOSSetEventMode, {0x00});

  PTP::closeSession();
}

std::unique_ptr<DeviceInfo> CanonPTPCamera::getDeviceInfo() {
  std::unique_ptr<DeviceInfo> deviceInfo = PTP::getDeviceInfo();

  deviceInfo->vendorExtensionId =
      static_cast<uint32_t>(VendorExtensionId::Canon);

  if (deviceInfo->isOpSupported(CanonOperationCode::EOSGetDeviceInfoEx)) {
    Buffer data = recv(CanonOperationCode::EOSGetDeviceInfoEx).data;
    EOSDeviceInfo eosDeviceInfo;
    eosDeviceInfo.unpack(data);
    deviceInfo->devicePropertiesSupported.insert(
        deviceInfo->devicePropertiesSupported.end(),
        eosDeviceInfo.devicePropertiesSupported.begin(),
        eosDeviceInfo.devicePropertiesSupported.end());
  }

  return deviceInfo;
}

// TODO: Event monitoring and error checking/handling
void CanonPTPCamera::capture() {
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

void CanonPTPCamera::getEvents() {
  Buffer data = recv(CanonOperationCode::EOSGetEvent).data;
  EOSEventData eventData;
  eventData.unpack(data);
  for (const Buffer& event : eventData.events) {
    if (auto propChanged = EOSEventPacket::unpackAs<EOSPropChanged>(event)) {
      // TODO: Figure out a good way to detect capture
      // if (propChanged->propertyCode == EOSPropertyCode::AvailableShots) {
      //   pushEvent<CaptureEvent>();
      //   continue;
      // }

      std::optional<CameraProp> prop =
          EOSProps.findKey(propChanged->propertyCode);
      if (!prop.has_value() || !EOSPropValues.contains(prop.value()))
        continue;

      std::optional<CameraPropValue> value =
          EOSPropValues.at(prop.value()).findValue(propChanged->propertyValue);
      if (!value.has_value())
        continue;

      pushEvent<SetPropEvent>(static_cast<uint16_t>(prop.value()),
                              value.value().first, value.value().second);
    }
  }
}

void CanonPTPCamera::setProp(CameraProp prop, CameraPropValue value) {
  std::optional<uint32_t> canonProp = EOSProps.findValue(prop);
  if (!canonProp.has_value() || !EOSPropValues.contains(prop) ||
      !isPropSupported(canonProp.value()))
    throw Exception(ExceptionContext::CameraSetProp,
                    ExceptionType::UnsupportedProperty);

  std::optional<uint32_t> canonValue = EOSPropValues.at(prop).findKey(value);
  if (!canonValue.has_value())
    throw Exception(ExceptionContext::CameraSetProp,
                    ExceptionType::UnsupportedValue);

  eosSetDeviceProp(canonProp.value(), canonValue.value());
}