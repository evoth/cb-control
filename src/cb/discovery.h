#ifndef CB_CONTROL_DISCOVERY_H
#define CB_CONTROL_DISCOVERY_H

#include <cb/event.h>
#include <cb/proxy.h>

namespace cb {

enum class DiscoveryMethod {
  SSDP,
};

class DiscoveryService : public EventEmitter<DiscoveryEvent>,
                         public EventDispatcher {
 public:
  DiscoveryService(DiscoveryMethod discoveryMethod)
      : discoveryMethod(discoveryMethod) {}

  virtual std::unique_ptr<CameraProxy> createCamera(
      const std::unique_ptr<DiscoveryAddEvent>& addEvent) = 0;

 protected:
  DiscoveryMethod discoveryMethod;
};

}  // namespace cb

#endif