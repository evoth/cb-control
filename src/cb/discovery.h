#ifndef CB_CONTROL_DISCOVERY_H
#define CB_CONTROL_DISCOVERY_H

#include <cb/event.h>
#include <cb/proxy.h>

namespace cb {

enum class DiscoveryMethod {
  SSDP,
};

class DiscoveryService : public EventProxy<EventContainer> {
 public:
  DiscoveryService(std::map<std::string, std::unique_ptr<CameraProxy>>& cameras,
                   DiscoveryMethod discoveryMethod)
      : cameras(cameras), discoveryMethod(discoveryMethod) {}

  virtual std::unique_ptr<CameraProxy> createCamera(
      std::unique_ptr<DiscoveryAddEvent> addEvent) = 0;

  void receiveEvent(std::unique_ptr<EventContainer> event) override;

 protected:
  std::string createId(std::string connectionAddress) {
    return std::to_string(static_cast<int>(discoveryMethod)) + "|" +
           connectionAddress;
  }

  void pushAndReceive(std::string containerId,
                      std::unique_ptr<EventPacket> event);

 private:
  std::map<std::string, std::unique_ptr<CameraProxy>>& cameras;
  DiscoveryMethod discoveryMethod;
};

}  // namespace cb

#endif