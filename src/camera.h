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

#include <memory>
#include <optional>
#include <utility>
#include <vector>

// Values are stored as (numerator, denominator)
typedef std::pair<uint32_t, uint32_t> CameraPropValue;

template <std::integral T>
class CameraPropMap {
 public:
  CameraPropMap(const std::vector<std::pair<T, CameraPropValue>>& map)
      : map(map) {}

  std::optional<T> findKey(CameraPropValue value) {
    for (auto& [k, v] : map) {
      if (v == value)
        return k;
    }
    return std::nullopt;
  }

  std::optional<CameraPropValue> findValue(T key) {
    for (auto& [k, v] : map) {
      if (k == key)
        return v;
    }
    return std::nullopt;
  }

 private:
  std::vector<std::pair<T, CameraPropValue>> map;
};

const CameraPropValue CPV_BULB(1, 0);

enum class CameraMode {
  Unknown,
  Manual,
  Av,
  Tv,
  Program,
  Auto,
};

struct CameraPropDesc {
  CameraPropValue currentValue;
  std::vector<CameraPropValue> supportedValues;
  bool isEnabled = false;
  bool isUpdated = false;
};

struct CameraProps {
  CameraMode mode = CameraMode::Unknown;
  CameraPropDesc aperture;
  CameraPropDesc shutterSpeed;
  CameraPropDesc iso;
  CameraPropDesc evComp;
};

class Camera {
 public:
  virtual ~Camera() = default;

  virtual void connect() = 0;
  virtual void disconnect() = 0;

  virtual void triggerCapture() = 0;

 protected:
  CameraProps props;
};

#endif