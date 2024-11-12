#ifndef CB_CONTROL_CAMERA_H
#define CB_CONTROL_CAMERA_H

#include <exception>

// Sketch of camera class
// - Will be responsible for taking pictures, changing settings, querying
//   available settings, etc.
// - Should keep an internal cache of camera state and avoid redundant
//   communication with camera
// - Depending on protocol, should automatically determine camera brand and/or
//   model and delegate to subclasses as necessary

// TODO: Return types? Exception handling?
class Camera {
 public:
  virtual ~Camera() = default;

  virtual void connect() = 0;
  virtual void disconnect() = 0;

  virtual void releaseShutter() = 0;
};

#endif