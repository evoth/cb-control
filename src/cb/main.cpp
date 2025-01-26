#include <cb/logger.h>
#include <cb/platforms/socketImpl.h>
#include <cb/protocols/ssdp.h>
#include <cb/protocols/xml.h>

#include <thread>

int main() {
  using namespace cb;

  std::map<std::string, std::unique_ptr<CameraProxy>> cameras;
  const std::array<uint8_t, 16> guid = {'C', 'a', 'p', 't', 'u', 'r', 'e', 'B',
                                        'e', 'a', 'm', 'P', 'T', 'P', 'I', 'P'};

  SSDPDiscovery ssdp(
      cameras, std::make_unique<UDPSocketImpl>(),
      std::make_unique<UDPSocketImpl>(), std::make_unique<TCPSocketImpl>(),
      {"urn:schemas-canon-com:service:ICPO-SmartPhoneEOSSystemService:1"}, guid,
      "CaptureBeam");

  std::unique_ptr<CameraProxy> camera;

  for (int i = 0; i < 2; i++) {
    while (!camera) {
      std::unique_ptr<EventContainer> container = ssdp.popEvent();
      if (!container)
        continue;
      for (const Buffer& event : container->events) {
        if (auto addEvent = EventPacket::unpackAs<DiscoveryAddEvent>(event)) {
          camera = std::move(cameras[container->id]);
          cameras.erase(
              container->id);  // TODO: Make way to remove advertisement as well
        }
      }
    }

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

    camera.reset();
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