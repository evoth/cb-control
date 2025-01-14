#include "proxy.h"

void CameraProxy::sendEvent(std::unique_ptr<EventPacket> event) {
  receiveEvent(
      std::make_unique<EventContainer>(id, std::vector<Buffer>{event->pack()}));
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

void CameraWrapper::getEvents() {
  if (!camera)
    return;
  while (std::unique_ptr<EventPacket> event = camera->popEvent()) {
    pushCameraEvent(std::move(event));
  }
}

void CameraWrapper::pushCameraEvent(std::unique_ptr<EventPacket> event) {
  const Buffer eventBuffer = event->pack();

  if (auto setPropEvent = EventPacket::unpackAs<SetPropEvent>(eventBuffer)) {
    const CameraProp prop = static_cast<CameraProp>(setPropEvent->propCode);
    const CameraPropValue value(setPropEvent->valueNumerator,
                                setPropEvent->valueDenominator);

    if (props.contains(prop) && props.at(prop) == value)
      return;  // Omit unnecessary event
    props[prop] = value;
  }

  if (!eventContainer)
    eventContainer =
        std::make_unique<EventContainer>(id, std::vector<Buffer>{});
  eventContainer->events.push_back(std::move(eventBuffer));
}

std::unique_ptr<EventContainer> CameraWrapper::popEvent() {
  getEvents();
  pushEvent(std::move(eventContainer));
  return EventProxy<EventContainer>::popEvent();
}

void CameraWrapper::handleEvent(const Buffer& event) {
  getEvents();
  // TODO: Check state to see if action is needed
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
        pushCameraEvent<ConnectEvent>(false);
    }
  } else if (!camera) {
    // Camera is required for other actions; push disconnect event if nullptr
    pushCameraEvent<ConnectEvent>(false);
  } else if (EventPacket::unpackAs<CaptureEvent>(event)) {
    camera->capture();
  } else if (auto setPropEvent = EventPacket::unpackAs<SetPropEvent>(event)) {
    const CameraProp prop = static_cast<CameraProp>(setPropEvent->propCode);
    const CameraPropValue value(setPropEvent->valueNumerator,
                                setPropEvent->valueDenominator);

    if (props.contains(prop) && props.at(prop) == value)
      return;  // Omit unnecessary action
    camera->setProp(prop, value);
  }
}

void CameraWrapper::receiveEvent(std::unique_ptr<EventContainer> container) {
  if (container->id != id)
    return;

  for (const Buffer& event : container->events) {
    try {
      handleEvent(event);
    } catch (Exception& e) {
      pushCameraEvent(std::make_unique<ExceptionEvent>(e));
    }
  }
}