#include "http.h"
#include "logger.h"
#include "platforms/windows/socket.h"

int main() {
  WindowsTCPSocket socket;
  socket.connect("192.168.4.1", 80);

  HTTPRequest request("GET", "/");
  request.headers["Host"] = "192.168.4.1";
  request.headers["Connection"] = "close";
  request.send(socket);

  HTTPResponse response;
  response.recv(socket);

  socket.close();

  Logger::log(response, true);

  return 0;
}

#if defined(ESP32)
#include <WiFi.h>
#include <esp_pthread.h>
#include <thread>

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