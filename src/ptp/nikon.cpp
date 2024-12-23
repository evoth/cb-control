#include "nikon.h"

#include <chrono>

void NikonPTPCamera::openSession() {
  PTP::openSession();

  invalidateCachedDI();

  startEventThread();
}

void NikonPTPCamera::closeSession() {
  stopEventThread();

  PTP::closeSession();
}

std::unique_ptr<DeviceInfo> NikonPTPCamera::getDeviceInfo() {
  std::unique_ptr<DeviceInfo> deviceInfo = PTP::getDeviceInfo();

  deviceInfo->vendorExtensionId =
      static_cast<uint32_t>(VendorExtensionId::Nikon);

  return deviceInfo;
}

void NikonPTPCamera::checkEvents() {
  recv(NikonOperationCode::CheckEvents);
}