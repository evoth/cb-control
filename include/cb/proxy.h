#ifndef CB_CONTROL_PROXY_H
#define CB_CONTROL_PROXY_H

#include <cb/camera.h>
#include <cb/event.h>
#include <cb/factory.h>

#include <map>

class CameraProxy : public Camera, public EventProxy<EventContainer> {
 public:
  void connect() override;
  void disconnect() override;
  void capture() override;
  void setProp(CameraProp prop, CameraPropValue value) override;

 protected:
  std::string id;

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
    // TODO: Exception in case of unsupported factory? And where should it go?
  }

  std::unique_ptr<EventContainer> popEvent() override;
  void receiveEvent(std::unique_ptr<EventContainer> event) override;

 protected:
  void getEvents() override;

 private:
  std::unique_ptr<Factory<EventCamera>> cameraFactory;
  std::unique_ptr<EventCamera> camera;

  std::unique_ptr<EventContainer> eventContainer;

  // bool isCapturing;
  std::map<CameraProp, CameraPropValue> props;

  void pushCameraEvent(std::unique_ptr<EventPacket> event);

  template <typename T, typename... Args>
    requires std::derived_from<T, EventPacket>
  void pushCameraEvent(Args&&... args) {
    pushCameraEvent(std::make_unique<T>(std::forward<Args>(args)...));
  }

  void handleEvent(const Buffer& event);
};

#endif