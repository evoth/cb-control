#ifndef CB_CONTROL_PTP_NIKON_H
#define CB_CONTROL_PTP_NIKON_H

#include "nikonData.h"
#include "ptp.h"

#include <thread>

class NikonPTPCamera : public PTPCamera {
 public:
  NikonPTPCamera(PTP&& ptp)
      : PTPCamera(std::move(ptp), VendorExtensionId::Nikon) {}

  void triggerCapture() override {}
  void setProp(CameraProp, CameraPropValue) override {}

 protected:
  void openSession() override;
  void closeSession() override;

  void checkEvents() override;

  std::unique_ptr<DeviceInfo> getDeviceInfo() override;
};

#endif