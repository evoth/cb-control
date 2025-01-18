#ifndef CB_CONTROL_LOGGER_H
#define CB_CONTROL_LOGGER_H

#include <cb/packet.h>

#if defined(_WIN32)
#include <iostream>
#elif defined(ESP32)
#include <HardwareSerial.h>
#endif

class Logger {
 public:
  template <typename... Args>
  static void log(bool newLine, const char* format, Args... args) {
    char msg[256];
    snprintf(msg, sizeof(msg), format, args...);
#if defined(_WIN32)
    std::cout << msg;
    if (newLine)
      std::cout << std::endl;
#elif defined(ESP32)
    if (newLine)
      Serial.println(msg);
    else
      Serial.print(msg);
#endif
  }

  template <typename... Args>
  static void log(const char* format = "", Args... args) {
    log(true, format, args...);
  }

  static void log(Packet& packet, bool asString = false) {
    log(packet.pack(), asString);
  }

  static void log(const Buffer& buff, bool asString = false) {
    if (asString) {
      for (auto& c : buff)
        log(false, "%c", c);
    } else {
      int i = 0;
      for (auto& c : buff) {
        log(false, "%02x ", +c);
        i++;
        if (i % 16 == 8)
          log(false, " ");
        if (i % 16 == 0)
          log();
      }
    }
    log();
  }
};

#endif