#ifndef CB_CONTROL_MANAGER_H
#define CB_CONTROL_MANAGER_H

#include <cb/discovery.h>
#include <cb/event.h>
#include <cb/proxy.h>

namespace cb {

class CameraManager : public EventEmitter<EventPacket> {
 public:
  std::unique_ptr<EventPacket> popEvent() override;

 protected:
  void getNewEvents() override;

 private:
  std::map<std::string, std::unique_ptr<CameraProxy>> cameras;
  std::vector<std::unique_ptr<DiscoveryService>> disoveryServices;
};

}  // namespace cb

#endif