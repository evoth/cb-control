#include "http.h"
#include "logger.h"
#include "platforms/windows/socket.h"

#include <thread>

int main() {
  WindowsUDPMulticastSocket socket;
  socket.begin("239.255.255.250", 1900);

  HTTPRequest req;
  std::string addr;

  while (true) {
    if (req.recv(socket, 0)) {
      addr = socket.getRemoteIp();
      Logger::log("Received from %s:", addr.c_str());
      Logger::log(req, true);
    }
  }

  return 0;
}

#if defined(ESP32)
#include <WiFi.h>
#include <esp_pthread.h>

void setup() {
  Serial.begin(115200);
  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFi.begin("ESP8266_AP", "defgecd7");

  while (WiFi.status() != WL_CONNECTED) {
    Serial.print('.');
    delay(1000);
  }
  Serial.print("\nIP address: ");
  Serial.println(WiFi.localIP());

  delay(10000);

  esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
  cfg.stack_size = (4096);
  esp_pthread_set_cfg(&cfg);

  std::thread mainThread(main);
  mainThread.join();
}

void loop() {}
#endif