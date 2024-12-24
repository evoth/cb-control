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

// TODO: How to handle when camera doesn't exist / isn't connected?
class CameraWrapper : public Camera {
 public:
  CameraWrapper(std::array<uint8_t, 16> clientGuid,
                std::string clientName,
                std::string ip,
                int port = 15740)
      : cameraFactory(std::make_unique<PTPCameraFactory>(
            std::make_unique<PTPIPFactory>(clientGuid, clientName, ip, port))) {
  }

  void connect() override {
    if (!camera) {
      camera = cameraFactory->create();
      if (!camera)
        throw Exception(ExceptionContext::Camera, ExceptionType::IsNull);
    }
    camera->connect();
  }

  void disconnect() override {
    if (camera)
      camera->disconnect();
  }

  bool isConnected() override { return camera && camera->isConnected(); }

  void triggerCapture() override {
    if (camera)
      camera->triggerCapture();
  }

  void setProp(CameraProp prop, CameraPropValue value) override {
    if (camera)
      camera->setProp(prop, value);
  }

 private:
  std::unique_ptr<Factory<Camera>> cameraFactory;
  std::unique_ptr<Camera> camera;
};

#endif