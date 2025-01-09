#ifndef CB_CONTROL_EVENT_H
#define CB_CONTROL_EVENT_H

#include "exception.h"
#include "logger.h"
#include "socket.h"

#include <mutex>
#include <queue>
#include <utility>

template <typename T>
class EventEmitter {
 public:
  virtual ~EventEmitter() = default;

  void pushEvent(std::unique_ptr<T> event) {
    std::lock_guard lock(eventsMutex);
    events.push(std::move(event));
    Logger::log("New result event:");
    Logger::log(*events.back());
  }

  template <typename U, typename... Args>
    requires std::derived_from<U, T>
  void pushEvent(Args&&... args) {
    pushEvent(std::make_unique<U>(std::forward<Args>(args)...));
  }

  virtual std::unique_ptr<T> popEvent() {
    std::lock_guard lock(eventsMutex);
    if (events.empty())
      return nullptr;
    std::unique_ptr<T> result = std::move(events.front());
    events.pop();
    return result;
  }

 protected:
  virtual void getEvents() = 0;

 private:
  std::mutex eventsMutex;
  std::queue<std::unique_ptr<T>> events;
};

template <typename T>
class EventProxy : public EventEmitter<T> {
 public:
  virtual ~EventProxy() = default;

  virtual void receiveEvent(std::unique_ptr<T> event) = 0;
};

class EventPacket : public TCPPacket {
 public:
  uint32_t length = 0;
  uint32_t eventCode = 0;

  EventPacket(uint32_t eventCode = 0) : eventCode(eventCode) {
    lengthField(this->length);
    typeField(this->eventCode);
  }

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
  EventContainer(std::string id = "", std::vector<Buffer> events = {})
      : EventPacket(0x01), id(id), events(events) {
    field(this->id);
    field<EventPacket>(this->events);
  }
};

class ExceptionEvent : public EventPacket {
 public:
  uint16_t contextCode = 0;
  uint16_t typeCode = 0;

  ExceptionEvent(uint16_t contextCode = 0, uint16_t typeCode = 0)
      : EventPacket(0x02), contextCode(contextCode), typeCode(typeCode) {
    field(this->contextCode);
    field(this->typeCode);
  }

  ExceptionEvent(Exception& e)
      : contextCode(static_cast<uint16_t>(e.context)),
        typeCode(static_cast<uint16_t>(e.type)) {}
};

class ConnectEvent : public EventPacket {
 public:
  bool isConnected = false;

  ConnectEvent(bool isConnected = false)
      : EventPacket(0x03), isConnected(isConnected) {
    field(this->isConnected);
  }
};

class CaptureEvent : public EventPacket {
 public:
  CaptureEvent() : EventPacket(0x04) {}
};

class SetPropEvent : public EventPacket {
 public:
  uint16_t propCode = 0;
  uint32_t valueNumerator = 0;
  uint32_t valueDenominator = 0;

  SetPropEvent(uint16_t propCode = 0,
               uint32_t valueNumerator = 0,
               uint32_t valueDenominator = 0)
      : EventPacket(0x05),
        propCode(propCode),
        valueNumerator(valueNumerator),
        valueDenominator(valueDenominator) {
    field(this->propCode);
    field(this->valueNumerator);
    field(this->valueDenominator);
  }
};

#endif