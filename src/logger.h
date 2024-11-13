#ifndef CB_CONTROL_LOGGER_H
#define CB_CONTROL_LOGGER_H

#if defined(_WIN32)
#include <iostream>
#elif defined(ESP32)
#include <HardwareSerial.h>
#endif

class Logger {
 public:
  static void log(const char* msg) {
#if defined(_WIN32)
    std::cout << msg << std::endl;
#elif defined(ESP32)
    Serial.println(msg);
#endif
  }
};

#endif