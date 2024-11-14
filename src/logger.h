#ifndef CB_CONTROL_LOGGER_H
#define CB_CONTROL_LOGGER_H

#if defined(_WIN32)
#include <iostream>
#elif defined(ESP32)
#include <HardwareSerial.h>
#endif

class Logger {
 public:
  template <typename... Args>
  static void log(const char* format, Args... args) {
    char msg[256];
    snprintf(msg, sizeof(msg), format, args...);
#if defined(_WIN32)
    std::cout << msg << std::endl;
#elif defined(ESP32)
    Serial.println(msg);
#endif
  }
};

#endif