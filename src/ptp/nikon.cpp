#include "nikon.h"

#include <chrono>

void NikonPTPCamera::openSession() {
  PTP::openSession();

  // TODO: Proper event queue
  nikonEventThread = std::jthread([this](std::stop_token stoken) {
    while (true) {
      if (stoken.stop_requested())
        return;
      recv(NikonOperationCode::CheckEvents);
      std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    }
  });

  invalidateCachedDI();
}

void NikonPTPCamera::closeSession() {
  if (nikonEventThread.joinable()) {
    nikonEventThread.request_stop();
    nikonEventThread.join();
  }

  PTP::closeSession();
}

std::unique_ptr<DeviceInfo> NikonPTPCamera::getDeviceInfo() {
  std::unique_ptr<DeviceInfo> deviceInfo = PTP::getDeviceInfo();

  deviceInfo->vendorExtensionId =
      static_cast<uint32_t>(VendorExtensionId::Nikon);

  return deviceInfo;
}