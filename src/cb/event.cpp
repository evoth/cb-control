#include <cb/event.h>

namespace cb {

int EventDispatcher::addExceptionHandler(
    EventHandlerCallback<Exception> callback,
    std::optional<int> times) {
  exceptionHandlers.emplace(handlerIdCounter,
                            EventHandler<Exception>(callback, times));
  return handlerIdCounter++;
}

bool EventDispatcher::containsHandler(int handlerId) {
  return eventHandlers.contains(handlerId) ||
         exceptionHandlers.contains(handlerId);
}

void EventDispatcher::removeHandler(int handlerId) {
  eventHandlers.erase(handlerId);
  exceptionHandlers.erase(handlerId);
}

void EventDispatcher::dispatchEvent(const Buffer& buffer) {
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

void EventDispatcher::dispatchEvent(std::unique_ptr<EventPacket> event) {
  if (!event)
    return;
  dispatchEvent(event->pack());
}

void EventDispatcher::dispatchException(const Exception& e) {
  for (auto it = exceptionHandlers.begin(); it != exceptionHandlers.end();) {
    it->second.call(e);

    if (it->second.isActive())
      it++;
    else
      it = exceptionHandlers.erase(it);
  }
}

}  // namespace cb