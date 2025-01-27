#include <cb/logger.h>
#include <cb/platforms/socketImpl.h>
#include <cb/protocols/ssdp.h>
#include <cb/protocols/xml.h>

#include <thread>

int main() {
  using namespace cb;

  SSDPDiscovery ssdp(
      std::make_unique<UDPSocketImpl>(), std::make_unique<UDPSocketImpl>(),
      std::make_unique<TCPSocketImpl>(),
      {"urn:schemas-canon-com:service:ICPO-SmartPhoneEOSSystemService:1"},
      {'C', 'a', 'p', 't', 'u', 'r', 'e', 'B', 'e', 'a', 'm', 'P', 'T', 'P',
       'I', 'P'},
      "CaptureBeam");

  ssdp.onException([](const Exception& exception) {
    Logger::log("SSDP exception (context=%d, type=%d)",
                static_cast<int>(exception.context),
                static_cast<int>(exception.type));
  });

  ssdp.onEvent<DiscoveryAddEvent>(
      [&ssdp](const std::unique_ptr<DiscoveryAddEvent>& addEvent) {
        auto camera = ssdp.createCamera(addEvent);

        camera->onException([](const Exception& exception) {
          Logger::log("Camera exception (context=%d, type=%d)",
                      static_cast<int>(exception.context),
                      static_cast<int>(exception.type));
        });

        Logger::log("Connecting to camera...");
        camera->connect();
        std::this_thread::sleep_for(std::chrono::seconds(5));

        camera->setProp(CameraProp::Aperture, {56, 10});
        camera->setProp(CameraProp::ShutterSpeed, {1, 100});
        camera->setProp(CameraProp::ISO, {400, 1});
        camera->capture();
        std::this_thread::sleep_for(std::chrono::seconds(1));

        camera->setProp(CameraProp::Aperture, {80, 10});
        camera->setProp(CameraProp::ShutterSpeed, {1, 1000});
        camera->setProp(CameraProp::ISO, {100, 1});
        camera->capture();
        std::this_thread::sleep_for(std::chrono::seconds(5));
      });

  while (true)
    ssdp.dispatchEvent();

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