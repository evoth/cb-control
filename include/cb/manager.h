#ifndef CB_CONTROL_MANAGER_H
#define CB_CONTROL_MANAGER_H

#include <cb/discovery.h>
#include <cb/event.h>
#include <cb/proxy.h>

class CameraManager : public EventProxy<EventContainer> {
 public:
  std::unique_ptr<EventContainer> popEvent() override;

  void receiveEvent(std::unique_ptr<EventContainer> event) override;

 protected:
  void getEvents() override;

 private:
  std::map<std::string, std::unique_ptr<CameraProxy>> cameras;
  std::vector<std::unique_ptr<DiscoveryService>> disoveryServices;
};

#endif