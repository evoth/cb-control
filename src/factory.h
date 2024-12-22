#ifndef CB_CONTROL_FACTORY_H
#define CB_CONTROL_FACTORY_H

#include "camera.h"
#include "ptp/ip.h"

class CameraFactory {
 public:
  virtual ~CameraFactory() = default;

  virtual bool isSupported() const = 0;
  virtual std::unique_ptr<Camera> create() const = 0;
};

class PTPFactory {
 public:
  static std::unique_ptr<Camera> create(
      std::unique_ptr<PTPTransport> transport);
};

class PTPIPFactory : public CameraFactory {
 public:
  PTPIPFactory(std::array<uint8_t, 16> clientGuid,
               std::string clientName,
               std::string ip,
               int port = 15740)
      : clientGuid(clientGuid), clientName(clientName), ip(ip), port(port) {}

  bool isSupported() const override;
  std::unique_ptr<Camera> create() const override;

 private:
  const std::array<uint8_t, 16> clientGuid;
  const std::string clientName;
  const std::string ip;
  const int port;
};

#endif