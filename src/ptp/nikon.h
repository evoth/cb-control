#ifndef CB_CONTROL_PTP_NIKON_H
#define CB_CONTROL_PTP_NIKON_H

#include "nikonData.h"
#include "ptp.h"

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

#endif