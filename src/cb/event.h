#ifndef CB_CONTROL_EVENT_H
#define CB_CONTROL_EVENT_H

#include <cb/exception.h>
#include <cb/protocols/tcp.h>

#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <utility>

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

template <typename T>
  requires std::derived_from<T, EventPacket>
class EventEmitter {
 public:
  virtual ~EventEmitter() = default;

  void pushEvent(std::unique_ptr<T> event) {
    std::lock_guard lock(eventsMutex);
    events.push(std::move(event));
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
  virtual void getNewEvents() = 0;

 private:
  std::mutex eventsMutex;
  std::queue<std::unique_ptr<T>> events;
};

template <typename T, typename U = void>
using EventHandlerCallback = std::function<U(const T&)>;

template <typename U>
class EventHandler {
 public:
  EventHandler(EventHandlerCallback<U, bool> callback, std::optional<int> times)
      : callback(callback), times(times) {}
  EventHandler(EventHandlerCallback<U, void> callback, std::optional<int> times)
      : callback([callback](const U& event) {
          callback(event);
          return true;
        }),
        times(times) {}

  void call(const U& event) {
    if (!isActive())
      return;
    if (callback(event) && times.has_value())
      (*times)--;
  }

  bool isActive() { return !times.has_value() || times.value() > 0; }

 private:
  EventHandlerCallback<U, bool> callback;
  std::optional<int> times;
};

class EventDispatcher {
 public:
  virtual ~EventDispatcher() = default;

  template <typename U>
    requires std::derived_from<U, EventPacket>
  int addEventHandler(EventHandlerCallback<std::unique_ptr<U>> callback,
                      std::optional<int> times = std::nullopt) {
    EventHandlerCallback<Buffer, bool> filterCallback =
        [callback](const Buffer& buffer) {
          if (auto event = EventPacket::unpackAs<U>(buffer)) {
            callback(event);
            return true;
          }
          return false;
        };
    eventHandlers.emplace(handlerIdCounter,
                          EventHandler<Buffer>(filterCallback, times));
    return handlerIdCounter++;
  }

  template <typename U>
    requires std::derived_from<U, EventPacket>
  int addEventHandler(std::function<void()> callback,
                      std::optional<int> times = std::nullopt) {
    return addEventHandler<U>(
        [callback](const std::unique_ptr<U>&) { callback(); }, times);
  }

  int addExceptionHandler(EventHandlerCallback<Exception> callback,
                          std::optional<int> times = std::nullopt) {
    exceptionHandlers.emplace(handlerIdCounter,
                              EventHandler<Exception>(callback, times));
    return handlerIdCounter++;
  }

  bool containsHandler(int handlerId) {
    return eventHandlers.contains(handlerId) ||
           exceptionHandlers.contains(handlerId);
  }

  void removeHandler(int handlerId) {
    eventHandlers.erase(handlerId);
    exceptionHandlers.erase(handlerId);
  }

  void dispatchEvent(const Buffer& buffer) {
    for (auto it = eventHandlers.begin(); it != eventHandlers.end();) {
      try {
        it->second.call(buffer);
      } catch (const Exception& e) {
        dispatchException(e);
      }

      if (it->second.isActive())
        it++;
      else
        it = eventHandlers.erase(it);
    }
  }

  void dispatchEvent(std::unique_ptr<EventPacket> event) {
    if (!event)
      return;
    dispatchEvent(event->pack());
  }

  void dispatchException(const Exception& e) {
    if (exceptionHandlers.empty())
      throw e;

    for (auto it = exceptionHandlers.begin(); it != exceptionHandlers.end();) {
      it->second.call(e);

      if (it->second.isActive())
        it++;
      else
        it = exceptionHandlers.erase(it);
    }
  }

 private:
  // TODO: Somehow merge event/exception handlers?
  int handlerIdCounter = 0;
  std::map<int, EventHandler<Buffer>> eventHandlers;
  std::map<int, EventHandler<Exception>> exceptionHandlers;
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