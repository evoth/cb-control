#include <cb/factory.h>
#include <cb/logger.h>
#include <cb/platforms/socketImpl.h>

#include "ptp/vendors/canon.h"
#include "ptp/vendors/nikon.h"

namespace cb {

std::unique_ptr<EventCamera> PTPCameraFactory::create() const {
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

  throw Exception(ExceptionContext::Factory, ExceptionType::UnsupportedCamera);
}

// TODO: Find a less weird preprocessor "control" flow for this
std::unique_ptr<PTPTransport> PTPIPFactory::create() const {
#if defined(_WIN32)
  return std::make_unique<PTPIP>(std::make_unique<TCPSocketImpl>(),
                                 std::make_unique<TCPSocketImpl>(), clientGuid,
                                 clientName, ip);
#elif defined(ESP32)
  return std::make_unique<PTPIP>(std::make_unique<TCPSocketImpl>(),
                                 std::make_unique<TCPSocketImpl>(), clientGuid,
                                 clientName, ip);
#else
  throw Exception(ExceptionContext::Factory,
                  ExceptionType::UnsupportedTransport);
#endif
}

}  // namespace cb