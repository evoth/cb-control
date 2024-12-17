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
#include <utility>
#include <vector>

typedef std::pair<uint32_t, uint32_t> CameraPropValue;

const CameraPropValue CPV_BULB(1, 0);

enum class CameraMode {
  Unknown,
  Manual,
  Av,
  Tv,
  Program,
  Auto,
};

class CameraPropDesc {
 public:
  CameraPropValue currentValue;
  std::vector<CameraPropValue> supportedValues;
  bool isEnabled = false;
  bool isUpdated = false;

  CameraPropDesc(CameraPropValue currentValue,
                 std::vector<CameraPropValue> supportedValues,
                 bool isEnabled,
                 bool isUpdated)
      : currentValue(currentValue),
        supportedValues(supportedValues),
        isEnabled(isEnabled),
        isUpdated(isUpdated) {}
};

struct CameraProps {
  CameraMode mode = CameraMode::Unknown;
  std::unique_ptr<CameraPropDesc> aperture;
  std::unique_ptr<CameraPropDesc> shutterSpeed;
  std::unique_ptr<CameraPropDesc> iso;
  std::unique_ptr<CameraPropDesc> evComp;
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