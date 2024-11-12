#ifndef CB_CONTROL_FACTORY_H
#define CB_CONTROL_FACTORY_H

#include "camera.h"
#include "ptp/canon.h"
#include "ptp/ip.h"

#ifdef _WIN32
#include "windows/socket.h"
#endif

class CameraFactory {
 public:
  virtual ~CameraFactory() = default;

  virtual bool isSupported() = 0;
  virtual std::unique_ptr<Camera> create() = 0;
};

class PTPIPFactory : public CameraFactory {
 public:
  PTPIPFactory(std::array<uint8_t, 16> clientGuid,
               std::string clientName,
               std::string ip,
               int port = 15740)
      : clientGuid(clientGuid), clientName(clientName), ip(ip), port(port) {}

  bool isSupported() override {
#ifdef _WIN32
    return true;
#else
    return false;
#endif
  }

  std::unique_ptr<Camera> create() {
#ifdef _WIN32
    std::unique_ptr<PTPIP> ptpip = std::make_unique<PTPIP>(
        std::make_unique<WindowsSocket>(), std::make_unique<WindowsSocket>(),
        clientGuid, clientName, ip);
    // TODO: Detect vendor and return object of respective class
    return std::make_unique<CanonPTPCamera>(std::move(ptpip));
#else
    return nullptr;
#endif
  }

 private:
  const std::array<uint8_t, 16> clientGuid;
  const std::string clientName;
  const std::string ip;
  const int port;
};

#endif