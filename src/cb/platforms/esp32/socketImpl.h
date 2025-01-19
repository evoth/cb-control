#ifndef CB_CONTROL_ESP32_SOCKET_H
#define CB_CONTROL_ESP32_SOCKET_H

#include <cb/protocols/tcp.h>

#include <WiFi.h>
#include <elapsedMillis.h>

namespace cb {

// TODO: Figure out error logging
class TCPSocketImpl : public TCPSocket, BufferedSocket {
 public:
  ~TCPSocketImpl() { client.stop(); }

  bool connect(const std::string& ip, int port) override {
    // TODO: Set socket options
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

}  // namespace cb

#endif