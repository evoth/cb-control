#ifndef CB_CONTROL_EVENTDATA_H
#define CB_CONTROL_EVENTDATA_H

#include <cb/exception.h>
#include <cb/packet.h>
#include <cb/protocols/tcp.h>

namespace cb {

class EventPacket : public TCPPacket {
 public:
  uint32_t length = 0;
  uint32_t eventCode = 0;

  EventPacket(uint32_t eventCode) : eventCode(eventCode) {
    lengthField(this->length);
    typeField(this->eventCode);
  }
  EventPacket() : EventPacket(0) {}

  template <typename T>
    requires(std::derived_from<T, EventPacket>)
  static std::unique_ptr<T> unpackAs(const Buffer& buffer) {
    return Packet::unpackAs<EventPacket, T>(buffer);
  }
};

class EventContainer : public EventPacket {
 public:
  std::string id;
  std::vector<Buffer> events;

  // TODO: Use move semantics for events
  EventContainer(std::string id, std::vector<Buffer> events)
      : EventPacket(0x01), id(id), events(events) {
    field(this->id);
    field<EventPacket>(this->events);
  }
  EventContainer() : EventContainer("", {}) {}
};

class CameraEvent : public virtual EventPacket {};

class ExceptionEvent : public CameraEvent {
 public:
  uint16_t contextCode = 0;
  uint16_t typeCode = 0;

  ExceptionEvent(uint16_t contextCode, uint16_t typeCode)
      : EventPacket(0x02), contextCode(contextCode), typeCode(typeCode) {
    field(this->contextCode);
    field(this->typeCode);
  }
  ExceptionEvent() : ExceptionEvent(0, 0) {}

  ExceptionEvent(const Exception& e)
      : ExceptionEvent(static_cast<uint16_t>(e.context),
                       static_cast<uint16_t>(e.type)) {}
};

class ConnectEvent : public CameraEvent {
 public:
  ConnectEvent() : EventPacket(0x03) {}
};

class DisconnectEvent : public CameraEvent {
 public:
  DisconnectEvent() : EventPacket(0x04) {}
};

class CaptureEvent : public CameraEvent {
 public:
  CaptureEvent() : EventPacket(0x05) {}
};

class SetPropEvent : public CameraEvent {
 public:
  uint16_t propCode = 0;
  uint32_t valueNumerator = 0;
  uint32_t valueDenominator = 0;

  SetPropEvent(uint16_t propCode,
               uint32_t valueNumerator,
               uint32_t valueDenominator)
      : EventPacket(0x06),
        propCode(propCode),
        valueNumerator(valueNumerator),
        valueDenominator(valueDenominator) {
    field(this->propCode);
    field(this->valueNumerator);
    field(this->valueDenominator);
  }
  SetPropEvent() : SetPropEvent(0, 0, 0) {}
};

class DiscoveryAddEvent : public EventPacket {
 public:
  uint16_t methodCode = 0;
  std::string connectionAddress;
  std::string serialNumber;
  std::string manufacturer;
  std::string model;
  std::string name;

  DiscoveryAddEvent(uint16_t methodCode,
                    std::string connectionAddress,
                    std::string serialNumber,
                    std::string manufacturer,
                    std::string model,
                    std::string name)
      : EventPacket(0x07),
        methodCode(methodCode),
        connectionAddress(connectionAddress),
        serialNumber(serialNumber),
        manufacturer(manufacturer),
        model(model),
        name(name) {
    field(this->methodCode);
    field(this->connectionAddress);
    field(this->serialNumber);
    field(this->manufacturer);
    field(this->model);
    field(this->name);
  }
  DiscoveryAddEvent() : DiscoveryAddEvent(0, "", "", "", "", "") {}
};

class DiscoveryRemoveEvent : public EventPacket {
 public:
  DiscoveryRemoveEvent() : EventPacket(0x08) {}
};

}  // namespace cb

#endif