#ifndef CB_CONTROL_CAMERA_H
#define CB_CONTROL_CAMERA_H

// Sketch of camera class
// - Will be responsible for taking pictures, changing settings, querying
//   available settings, etc.
// - Should keep an internal cache of camera state and avoid redundant
//   communication with camera
// - Depending on protocol, should automatically determine camera brand and/or
//   model and delegate to subclasses as necessary

// State
// - Will hold a DPD-like object for every property
//   - Hold DPD info + whether it is enabled and whether it is updated
// - The DPD object is created on first query then continues to track state
// - No values are "optional/nullable", only disabled or out-of-date
// - If out-of-date, query is made to camera upon access
// - If disabled, can't be set
// - Camera implementation will asynchronously update DPDs
//   - For PTP cameras, this is mostly done when processing events
// - To set prop, implementation function will be called without setting the DPD
//   - The DPD will be updated later by the implementation if successful

// Values
// - Numerical properties can be represented by pair<uint numer, uint denom>
// - Implementations will internally convert to/from these values

// TODO: Return types? Exception handling?

#include "event.h"

// Values are stored as (numerator, denominator)
typedef std::pair<uint32_t, uint32_t> CameraPropValue;

const CameraPropValue CPV_BULB(1, 0);

template <typename T, typename U>
class PairMap {
 public:
  PairMap(const std::vector<std::pair<T, U>>& map) : map(map) {}

  std::optional<T> findKey(const U& value) const {
    for (auto& [k, v] : map) {
      if (v == value)
        return k;
    }
    return std::nullopt;
  }

  std::optional<U> findValue(const T& key) const {
    for (auto& [k, v] : map) {
      if (k == key)
        return v;
    }
    return std::nullopt;
  }

 private:
  const std::vector<std::pair<T, U>> map;
};

enum class CameraProp {
  Aperture,
  ShutterSpeed,
  ISO,
  EVComp,
};

// struct CameraPropDesc {
//   CameraPropValue currentValue;
//   std::vector<CameraPropValue> supportedValues;
//   bool isEnabled = false;
//   bool isUpdated = false;
// };

// typedef std::map<CameraProp, std::unique_ptr<CameraPropDesc>> CameraPropMap;

// enum class CameraMode {
//   Unknown,
//   Manual,
//   Av,
//   Tv,
//   Program,
//   Auto,
// };

class Camera {
 public:
  virtual ~Camera() = default;

  virtual void connect() = 0;
  virtual void disconnect() = 0;

  virtual void capture() = 0;
  virtual void setProp(CameraProp prop, CameraPropValue value) = 0;

 protected:
  // CameraPropMap props;
  // CameraMode mode = CameraMode::Unknown;
};

class EventCamera : public Camera, public EventEmitter<EventPacket> {};

#endif