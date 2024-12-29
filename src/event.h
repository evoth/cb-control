#ifndef CB_CONTROL_EVENT_H
#define CB_CONTROL_EVENT_H

#include "logger.h"
#include "packet.h"

#include <mutex>
#include <queue>
#include <utility>

template <typename T>
class EventManager {
 public:
  virtual ~EventManager() = default;

  virtual void requestEvent(T&) {}

  template <typename U, typename... Args>
    requires std::derived_from<U, T>
  void pushEvent(Args&&... args) {
    std::lock_guard lock(eventsMutex);
    events.push(std::make_unique<U>(std::forward<Args>(args)...));
    Logger::log("New result event:");
    Logger::log(*events.back());
  }

  void pushEvent(std::unique_ptr<T> event) {
    std::lock_guard lock(eventsMutex);
    events.push(std::move(event));
    Logger::log("New result event:");
    Logger::log(*events.back());
  }

  virtual std::unique_ptr<T> popEvent() {
    std::lock_guard lock(eventsMutex);
    if (events.empty())
      return nullptr;
    std::unique_ptr<T> result = std::move(events.front());
    events.pop();
    return result;
  }

 private:
  std::mutex eventsMutex;
  std::queue<std::unique_ptr<T>> events;
};

class EventPacket : public Packet {
 public:
  uint32_t length = 0;
  uint32_t eventId = 0;

  EventPacket(uint32_t eventId = 0) : eventId(eventId) {
    lengthField(this->length);
    typeField(this->eventId);
  }
};

class ExceptionEvent : public EventPacket {
 public:
  uint16_t context = 0;
  uint16_t type = 0;

  ExceptionEvent(uint16_t context = 0, uint16_t type = 0)
      : EventPacket(0x01), context(context), type(type) {
    field(this->context);
    field(this->type);
  }
};

class ConnectedEvent : public EventPacket {
 public:
  bool isConnected = false;

  ConnectedEvent(bool isConnected = false)
      : EventPacket(0x02), isConnected(isConnected) {
    field(this->isConnected);
  }
};

#endif