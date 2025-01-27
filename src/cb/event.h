#ifndef CB_CONTROL_EVENT_H
#define CB_CONTROL_EVENT_H

#include "eventData.h"

#include <functional>
#include <map>
#include <mutex>
#include <queue>
#include <utility>

namespace cb {

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
                          std::optional<int> times = std::nullopt);

  bool containsHandler(int handlerId);

  void removeHandler(int handlerId);

  void dispatchEvent(const Buffer& buffer);
  void dispatchEvent(std::unique_ptr<EventPacket> event);
  void dispatchException(const Exception& e);

 private:
  // TODO: Somehow merge event/exception handlers?
  int handlerIdCounter = 0;
  std::map<int, EventHandler<Buffer>> eventHandlers;
  std::map<int, EventHandler<Exception>> exceptionHandlers;
};

}  // namespace cb

#endif