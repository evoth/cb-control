#ifndef CB_CONTROL_PROXY_H
#define CB_CONTROL_PROXY_H

#include "camera.h"
#include "event.h"
#include "factory.h"

class CameraProxy : public Camera, public EventProxy<EventContainer> {
 public:
  void connect() override;
  void disconnect() override;
  void capture() override;
  void setProp(CameraProp prop, CameraPropValue value) override;

 protected:
  std::string id;

  template <typename U, typename... Args>
    requires std::derived_from<U, EventPacket>
  void pushEvent(Args&&... args) {
    std::unique_ptr<U> event = std::make_unique<U>(std::forward<Args>(args)...);
    EventEmitter::pushEvent(createContainer(std::move(event)));
  }

  std::unique_ptr<EventContainer> createContainer(
      std::unique_ptr<EventPacket> event);

 private:
  void sendEvent(std::unique_ptr<EventPacket> event);
};

class CameraWrapper : public CameraProxy {
 public:
  CameraWrapper(std::array<uint8_t, 16> clientGuid,
                std::string clientName,
                std::string ip,
                int port = 15740)
      : cameraFactory(std::make_unique<PTPCameraFactory>(
            std::make_unique<PTPIPFactory>(clientGuid, clientName, ip, port))) {
  }

  void receiveEvent(std::unique_ptr<EventContainer> event) override;
  // std::unique_ptr<EventContainer> popEvent() override;

 private:
  std::unique_ptr<Factory<EventCamera>> cameraFactory;
  std::unique_ptr<EventCamera> camera;

  void handleEvent(const Buffer& event);
};

#endif