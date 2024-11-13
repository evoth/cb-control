#include "factory.h"
#include "ptp/canon.h"

#if defined(_WIN32)
#include "windows/socket.h"
#elif defined(ESP32)
#include "esp32/socket.h"
#endif

bool PTPIPFactory::isSupported() {
#if defined(_WIN32) || defined(ESP32)
  return true;
#else
  return false;
#endif
}

std::unique_ptr<Camera> PTPIPFactory::create() {
#if defined(_WIN32)
  std::unique_ptr<PTPIP> ptpip = std::make_unique<PTPIP>(
      std::make_unique<WindowsSocket>(), std::make_unique<WindowsSocket>(),
      clientGuid, clientName, ip);
  // TODO: Detect vendor and return object of respective class
  return std::make_unique<CanonPTPCamera>(std::move(ptpip));
#elif defined(ESP32)
  std::unique_ptr<PTPIP> ptpip = std::make_unique<PTPIP>(
      std::make_unique<ESP32Socket>(), std::make_unique<ESP32Socket>(),
      clientGuid, clientName, ip);
  // TODO: Detect vendor and return object of respective class
  return std::make_unique<CanonPTPCamera>(std::move(ptpip));
#else
  return nullptr;
#endif
}