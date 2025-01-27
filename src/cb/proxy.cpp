#include <cb/proxy.h>

namespace cb {

void CameraProxy::connect() {
  dispatchEvent(std::make_unique<ConnectEvent>());
}

void CameraProxy::disconnect() {
  dispatchEvent(std::make_unique<DisconnectEvent>());
}

void CameraProxy::capture() {
  dispatchEvent(std::make_unique<CaptureEvent>());
}

void CameraProxy::setProp(CameraProp prop, CameraPropValue value) {
  dispatchEvent(std::make_unique<SetPropEvent>(static_cast<uint16_t>(prop),
                                               value.first, value.second));
}

void CameraWrapper::getNewEvents() {
  if (!camera)
    return;
  while (std::unique_ptr<CameraEvent> event = camera->popEvent()) {
    pushEvent(std::move(event));
  }
}

std::unique_ptr<CameraEvent> CameraWrapper::popEvent() {
  getNewEvents();
  return Camera::popEvent();
}

}  // namespace cb