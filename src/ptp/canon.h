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
  void setProp(CameraProp prop, CameraPropValue value) override;

 protected:
  void openSession() override;
  void closeSession() override;
  std::unique_ptr<DeviceInfo> getDeviceInfo() override;

 private:
  std::jthread eosEventThread;

  bool isEos();
  bool isEosM();

  void checkEvents();

  template <std::unsigned_integral T>
  void eosSetDeviceProp(uint32_t devicePropertyCode, T value) {
    send(CanonOperationCode::EOSSetDevicePropValueEx, {},
         EOSDeviceProp<T>(devicePropertyCode, value).pack());
  }
};

#endif