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

 protected:
  void openSession() override;
  void closeSession() override;
  DeviceInfo getDeviceInfo() override;

 private:
  std::jthread nikonEventThread;
};

#endif