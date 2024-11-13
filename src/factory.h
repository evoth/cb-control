#ifndef CB_CONTROL_FACTORY_H
#define CB_CONTROL_FACTORY_H

#include "camera.h"
#include "ptp/ip.h"

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

  bool isSupported() override;
  std::unique_ptr<Camera> create() override;

 private:
  const std::array<uint8_t, 16> clientGuid;
  const std::string clientName;
  const std::string ip;
  const int port;
};

#endif