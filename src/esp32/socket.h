#ifndef CB_CONTROL_ESP32_SOCKET_H
#define CB_CONTROL_ESP32_SOCKET_H

#include "../ptp/ip.h"

#include <WiFi.h>
#include <elapsedMillis.h>

// TODO: Figure out error logging
class ESP32Socket : public Socket {
 public:
  virtual ~ESP32Socket() { client.stop(); }

  bool connect(const std::string& ip, int port) override {
    return client.connect(ip.c_str(), port);
  }

  bool close() override {
    client.stop();
    return true;
  }

  bool isConnected() override { return client.connected(); }

  // TODO: Use a buffer (ESP32 WiFi.h supports that, unlike pure Arduino)
  int send(Buffer& buffer) override {
    int totalSent = 0;
    while (totalSent < buffer.size() && client.write(buffer[totalSent])) {
      totalSent++;
    }
    return totalSent;
  }

  // TODO: Use a buffer (ESP32 WiFi.h supports that, unlike pure Arduino)
  int recv(Buffer& buffer, int length, unsigned int timeoutMs) override {
    elapsedMillis elapsed;

    int totalReceived = 0;
    while (totalReceived < length && elapsed < timeoutMs) {
      if (client.available()) {
        buffer.push_back(client.read());
        totalReceived++;
      }
      vTaskDelay(1);
    }

    return totalReceived;
  }

 private:
  WiFiClient client;
};

#endif