#include <cb/discovery.h>

namespace cb {

void DiscoveryService::receiveEvent(std::unique_ptr<EventContainer> container) {
  for (const Buffer& event : container->events) {
    try {
      if (auto addEvent = EventPacket::unpackAs<DiscoveryAddEvent>(event)) {
        cameras[container->id] = createCamera(std::move(addEvent));
      } else if (auto removeEvent =
                     EventPacket::unpackAs<DiscoveryRemoveEvent>(event)) {
        cameras.erase(container->id);
      }
    } catch (Exception& e) {
      pushEvent<EventContainer>(container->id,
                                std::vector<Buffer>{ExceptionEvent(e).pack()});
    }
  }
}

void DiscoveryService::pushAndReceive(std::string containerId,
                                      std::unique_ptr<EventPacket> event) {
  pushEvent<EventContainer>(containerId, std::vector<Buffer>{event->pack()});
  receiveEvent(std::make_unique<EventContainer>(
      containerId, std::vector<Buffer>{event->pack()}));
}

}  // namespace cb