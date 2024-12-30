#include "proxy.h"

std::unique_ptr<EventContainer> CameraProxy::createContainer(
    std::unique_ptr<EventPacket> event) {
  return std::make_unique<EventContainer>(id,
                                          std::vector<Buffer>{event->pack()});
}

void CameraProxy::sendEvent(std::unique_ptr<EventPacket> event) {
  receiveEvent(createContainer(std::move(event)));
}

void CameraProxy::connect() {
  sendEvent(std::make_unique<ConnectEvent>(true));
}

void CameraProxy::disconnect() {
  sendEvent(std::make_unique<ConnectEvent>(false));
}

void CameraProxy::capture() {
  sendEvent(std::make_unique<CaptureEvent>());
}

void CameraProxy::setProp(CameraProp prop, CameraPropValue value) {
  sendEvent(std::make_unique<SetPropEvent>(static_cast<uint16_t>(prop),
                                           value.first, value.second));
}

void CameraWrapper::handleEvent(const Buffer& event) {
  if (auto connectEvent = EventPacket::unpackAs<ConnectEvent>(event)) {
    if (connectEvent->isConnected) {
      // Attempt to connect camera
      if (!camera)
        camera = cameraFactory->create();
      if (camera)
        camera->connect();
    } else {
      // Attempt to disconnect camera
      if (camera)
        camera->disconnect();
      else
        pushEvent<ConnectEvent>(false);
    }
  } else if (!camera) {
    // Camera is required for other actions; push disconnect event if nullptr
    pushEvent<ConnectEvent>(false);
  } else if (EventPacket::unpackAs<CaptureEvent>(event)) {
    camera->capture();
  } else if (auto setPropEvent = EventPacket::unpackAs<SetPropEvent>(event)) {
    camera->setProp(
        static_cast<CameraProp>(setPropEvent->propCode),
        {setPropEvent->valueNumerator, setPropEvent->valueDenominator});
  }
}

void CameraWrapper::receiveEvent(std::unique_ptr<EventContainer> container) {
  if (container->id != id)
    return;

  for (const Buffer& event : container->events) {
    try {
      handleEvent(event);
    } catch (Exception& e) {
      EventEmitter::pushEvent(createContainer(e.createEvent()));
    }
  }
}

// Will process events, update state, and build container while omitting
// redundant events
// std::unique_ptr<EventContainer> CameraWrapper::popEvent();