#ifndef CB_CONTROL_MANAGER_H
#define CB_CONTROL_MANAGER_H

#include <cb/discovery.h>
#include <cb/event.h>
#include <cb/proxy.h>

namespace cb {

class CameraManager : public EventManager<EventContainer> {
 public:
  std::unique_ptr<EventContainer> popEvent() override;

  // void receiveEvent(std::unique_ptr<EventContainer> event) override;

 protected:
  void getNewEvents() override;

 private:
  std::map<std::string, std::unique_ptr<CameraProxy>> cameras;
  std::vector<std::unique_ptr<DiscoveryService>> disoveryServices;
};

}  // namespace cb

#endif

// onEvent<EventContainer>(
//         [this](const std::unique_ptr<EventContainer>& container) {
//           if (container->id != id)
//             return;

//           for (const Buffer& event : container->events) {
//             try {
//               getNewEvents();
//               dispatchEvent(event);
//             } catch (Exception& e) {
//               pushCameraEvent(std::make_unique<ExceptionEvent>(e));
//             }
//           }
//         });