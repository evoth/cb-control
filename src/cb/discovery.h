#ifndef CB_CONTROL_DISCOVERY_H
#define CB_CONTROL_DISCOVERY_H

#include <cb/event.h>
#include <cb/proxy.h>

namespace cb {

class DiscoveryService : public EventProxy<EventContainer> {
 public:
  DiscoveryService(std::map<std::string, std::unique_ptr<CameraProxy>>& cameras)
      : cameras(cameras) {}

  virtual std::unique_ptr<CameraProxy> createCamera(
      std::unique_ptr<DiscoveryAddEvent> event) = 0;

  void receiveEvent(std::unique_ptr<EventContainer> event) override;

 private:
  std::map<std::string, std::unique_ptr<CameraProxy>>& cameras;
};

}  // namespace cb

#endif