#ifndef CB_CONTROL_PTP_NIKON_H
#define CB_CONTROL_PTP_NIKON_H

#include <cb/ptp/ptp.h>
#include "nikonData.h"

namespace cb {

class NikonPTPCamera : public PTPCamera {
 public:
  NikonPTPCamera(PTP&& ptp)
      : PTPCamera(std::move(ptp), VendorExtensionId::Nikon) {}

  void capture() override {}
  void setProp(CameraProp, CameraPropValue) override {}

 protected:
  void openSession() override;
  void closeSession() override;

  void getEvents() override;

  std::unique_ptr<DeviceInfo> getDeviceInfo() override;
};

}  // namespace cb

#endif