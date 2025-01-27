#ifndef CB_CONTROL_PROXY_H
#define CB_CONTROL_PROXY_H

#include <cb/camera.h>
#include <cb/event.h>
#include <cb/factory.h>

#include <map>

namespace cb {

class CameraProxy : public Camera, public EventManager<CameraEvent> {
 public:
  void connect() override;
  void disconnect() override;
  void capture() override;
  void setProp(CameraProp prop, CameraPropValue value) override;
};

class CameraWrapper : public CameraProxy {
 public:
  CameraWrapper(std::array<uint8_t, 16> clientGuid,
                std::string clientName,
                std::string ip,
                int port = 15740)
      : cameraFactory(std::make_unique<PTPCameraFactory>(
            std::make_unique<PTPIPFactory>(clientGuid, clientName, ip, port))) {
    onEvent<ConnectEvent>([this]() {
      if (!camera)
        camera = cameraFactory->create();
      if (camera)
        camera->connect();
    });

    onEvent<DisconnectEvent>([this]() {
      if (camera)
        camera->disconnect();
      else
        pushEvent<DisconnectEvent>();
    });

    onEvent<CaptureEvent>([this]() {
      if (!camera)
        pushEvent<DisconnectEvent>();
      camera->capture();
    });

    onEvent<SetPropEvent>(
        [this](const std::unique_ptr<SetPropEvent>& setPropEvent) {
          if (!camera)
            pushEvent<DisconnectEvent>();
          const CameraProp prop =
              static_cast<CameraProp>(setPropEvent->propCode);
          const CameraPropValue value(setPropEvent->valueNumerator,
                                      setPropEvent->valueDenominator);
          camera->setProp(prop, value);
        });
  };

  std::unique_ptr<CameraEvent> popEvent() override;

 protected:
  void getNewEvents() override;

 private:
  std::unique_ptr<Factory<EventCamera>> cameraFactory;
  std::unique_ptr<EventCamera> camera;
};

}  // namespace cb

#endif