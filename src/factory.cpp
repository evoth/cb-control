#include "factory.h"

#include "logger.h"
#include "ptp/canon.h"
#include "ptp/nikon.h"

#if defined(_WIN32)
#include "windows/socket.h"
#elif defined(ESP32)
#include "esp32/socket.h"
#endif

std::unique_ptr<Camera> PTPCameraFactory::create() const {
  PTP ptp(transportFactory->create());
  ptp.openTransport();
  std::unique_ptr<DeviceInfo> deviceInfo = ptp.getDeviceInfo();

  // Canon and Nikon use the MTP VendorExtensionID instead of their designated
  // ones, so we check the manufacturer string instead
  if (deviceInfo->manufacturer.find("Canon") != std::string::npos) {
    Logger::log("Detected Canon camera.");
    return std::make_unique<CanonPTPCamera>(std::move(ptp));
  } else if (deviceInfo->manufacturer.find("Nikon") != std::string::npos) {
    Logger::log("Detected Nikon camera.");
    return std::make_unique<NikonPTPCamera>(std::move(ptp));
  }

  return nullptr;
}

bool PTPIPFactory::isSupported() const {
#if defined(_WIN32) || defined(ESP32)
  return true;
#else
  return false;
#endif
}

// TODO: Find a less weird preprocessor "control" flow for this
std::unique_ptr<PTPTransport> PTPIPFactory::create() const {
#if defined(_WIN32)
  return std::make_unique<PTPIP>(std::make_unique<WindowsSocket>(),
                                 std::make_unique<WindowsSocket>(), clientGuid,
                                 clientName, ip);
#elif defined(ESP32)
  return std::make_unique<PTPIP>(std::make_unique<ESP32Socket>(),
                                 std::make_unique<ESP32Socket>(), clientGuid,
                                 clientName, ip);
#else
  return nullptr;
#endif
}