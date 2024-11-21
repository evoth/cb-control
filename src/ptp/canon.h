#ifndef CB_CONTROL_PTP_CANON_H
#define CB_CONTROL_PTP_CANON_H

#include "canonData.h"
#include "ptp.h"

#include <thread>

// TODO: Break out CanonPTPCamera into classes based on series/behavior (e.g.
// EOS, EOS M, Powershot, etc.)

class CanonPTPCamera : public PTPCamera {
 public:
  CanonPTPCamera(PTP&& ptp)
      : PTPCamera(std::move(ptp), VendorExtensionId::Canon) {}

  void triggerCapture() override;

 protected:
  void openSession() override;
  void closeSession() override;
  DeviceInfo getDeviceInfo() override;

 private:
  std::jthread eosEventThread;

  bool isEos();
  bool isEosM();

  // TODO: Support other datatypes using template?
  void setEosDeviceProp(uint32_t devicePropertyCode, uint32_t value);
};

#endif