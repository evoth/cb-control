#ifndef CB_CONTROL_FACTORY_H
#define CB_CONTROL_FACTORY_H

#include "camera.h"
#include "ptp/ip.h"

template <typename T>
class Factory {
 public:
  virtual ~Factory() = default;

  virtual bool isSupported() const = 0;
  virtual std::unique_ptr<T> create() const = 0;
};

class PTPCameraFactory : public Factory<Camera> {
 public:
  PTPCameraFactory(std::unique_ptr<Factory<PTPTransport>> transportFactory)
      : transportFactory(std::move(transportFactory)) {}

  bool isSupported() const override { return transportFactory->isSupported(); }
  std::unique_ptr<Camera> create() const override;

 private:
  std::unique_ptr<Factory<PTPTransport>> transportFactory;
};

class PTPIPFactory : public Factory<PTPTransport> {
 public:
  PTPIPFactory(std::array<uint8_t, 16> clientGuid,
               std::string clientName,
               std::string ip,
               int port = 15740)
      : clientGuid(clientGuid), clientName(clientName), ip(ip), port(port) {}

  bool isSupported() const override;
  std::unique_ptr<PTPTransport> create() const override;

 private:
  const std::array<uint8_t, 16> clientGuid;
  const std::string clientName;
  const std::string ip;
  const int port;
};

class CameraFactory : public Factory<Camera> {
 public:
  CameraFactory(std::array<uint8_t, 16> clientGuid,
                std::string clientName,
                std::string ip,
                int port = 15740)
      : cameraFactory(std::make_unique<PTPCameraFactory>(
            std::make_unique<PTPIPFactory>(clientGuid, clientName, ip, port))) {
  }

  bool isSupported() const override { return cameraFactory->isSupported(); }
  std::unique_ptr<Camera> create() const override {
    return cameraFactory->create();
  }

 private:
  std::unique_ptr<Factory<Camera>> cameraFactory;
};

#endif