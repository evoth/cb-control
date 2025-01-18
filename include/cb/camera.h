#ifndef CB_CONTROL_CAMERA_H
#define CB_CONTROL_CAMERA_H

#include <cb/event.h>

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

class Camera {
 public:
  virtual ~Camera() = default;

  virtual void connect() = 0;
  virtual void disconnect() = 0;

  virtual void capture() = 0;
  virtual void setProp(CameraProp prop, CameraPropValue value) = 0;
};

class EventCamera : public Camera, public EventEmitter<EventPacket> {};

#endif