#include "factory.h"

#include <thread>

int main() {
  const std::array<uint8_t, 16> guid(
      {7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7});

  std::unique_ptr<Camera> camera =
      PTPIPFactory(guid, "Tim", "192.168.4.7").create();

  if (!camera) {
    return 1;
  }

  camera->connect();
  std::this_thread::sleep_for(std::chrono::seconds(5));

  camera->setProp(CameraProp::Aperture, CameraPropValue(56, 10));
  camera->setProp(CameraProp::ShutterSpeed, CameraPropValue(1, 100));
  camera->setProp(CameraProp::ISO, CameraPropValue(400, 1));
  camera->triggerCapture();
  std::this_thread::sleep_for(std::chrono::seconds(1));

  camera->setProp(CameraProp::Aperture, CameraPropValue(80, 10));
  camera->setProp(CameraProp::ShutterSpeed, CameraPropValue(1, 1000));
  camera->setProp(CameraProp::ISO, CameraPropValue(100, 1));
  camera->triggerCapture();
  std::this_thread::sleep_for(std::chrono::seconds(5));

  camera->disconnect();
  std::this_thread::sleep_for(std::chrono::seconds(5));

  return 0;
}

#if defined(ESP32)
#include <WiFi.h>
#include <esp_pthread.h>

void setup() {
  Serial.begin(115200);
  Serial.println();

  Serial.print("Starting soft-AP... ");
  WiFi.softAP("ESP8266_AP", "defgecd7");

  Serial.print("Soft-AP IP address: ");
  Serial.println(WiFi.softAPIP());

  esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
  cfg.stack_size = (4096);
  esp_pthread_set_cfg(&cfg);

  std::thread mainThread(main);
  mainThread.join();
}

void loop() {}
#endif