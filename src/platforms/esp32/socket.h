#ifndef CB_CONTROL_ESP32_SOCKET_H
#define CB_CONTROL_ESP32_SOCKET_H

#include "../../socket.h"

#include <WiFi.h>
#include <elapsedMillis.h>

// TODO: Figure out error logging
class ESP32Socket : public TCPSocket, BufferedSocket {
 public:
  ~ESP32Socket() { client.stop(); }

  bool connect(const std::string& ip, int port) override {
    return client.connect(ip.c_str(), port);
  }

  bool close() override {
    client.stop();
    return true;
  }

  bool isConnected() override { return client.connected(); }

 protected:
  int send(const char* buff, int length) override {
    return client.write(buff, length);
  }

  int recv(char* buff, int length) override {
    return client.read((uint8_t*)buff, length);
  }

  bool wait(unsigned int timeoutMs) override {
    if (timeoutMs == 0)
      return client.available();

    elapsedMillis elapsed;
    while (elapsed < timeoutMs) {
      if (client.available())
        return true;
      vTaskDelay(1);
    }
    return false;
  }

 private:
  WiFiClient client;
};

#endif