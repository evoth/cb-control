#ifndef CB_CONTROL_DISCOVERY_H
#define CB_CONTROL_DISCOVERY_H

#include <cb/event.h>
#include <cb/proxy.h>

namespace cb {

enum class DiscoveryMethod {
  SSDP,
};

class DiscoveryService : public EventEmitter<EventContainer> {
 public:
  DiscoveryService(DiscoveryMethod discoveryMethod)
      : discoveryMethod(discoveryMethod) {}

  virtual std::unique_ptr<CameraProxy> createCamera(
      std::unique_ptr<DiscoveryAddEvent> addEvent) = 0;

 protected:
  std::string createId(std::string connectionAddress) {
    return std::to_string(static_cast<int>(discoveryMethod)) + "|" +
           connectionAddress;
  }

 private:
  DiscoveryMethod discoveryMethod;
};

}  // namespace cb

#endif