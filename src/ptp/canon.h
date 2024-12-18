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

  void checkEvents();
  void updateDesc(uint32_t propertyCode,
                  CameraPropDesc& desc,
                  CameraPropMap<uint32_t> map);
  void updateProp(uint32_t propertyCode, uint32_t value);
  void updateProp(uint32_t propertyCode,
                  uint32_t value,
                  CameraPropDesc& desc,
                  CameraPropMap<uint32_t> map);

  template <std::unsigned_integral T>
  void eosSetDeviceProp(uint32_t devicePropertyCode, T value) {
    send(CanonOperationCode::EOSSetDevicePropValueEx, {},
         EOSDeviceProp<T>(devicePropertyCode, value).pack());
  }
};

#endif