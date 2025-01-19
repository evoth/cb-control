#include <cb/logger.h>
#include <cb/platforms/socketImpl.h>
#include <cb/protocols/ssdp.h>
#include <cb/protocols/xml.h>

#include <thread>

int main() {
  using namespace cb;

  std::map<std::string, std::unique_ptr<CameraProxy>> cameras;
  std::array<uint8_t, 16> guid = {'C', 'a', 'p', 't', 'u', 'r', 'e', 'B',
                                  'e', 'a', 'm', 'P', 'T', 'P', 'I', 'P'};

  SSDPDiscovery ssdp(
      cameras, std::make_unique<UDPMulticastSocketImpl>(),
      std::make_unique<TCPSocketImpl>(),
      {"urn:schemas-canon-com:service:ICPO-SmartPhoneEOSSystemService:1"}, guid,
      "CaptureBeam");

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::unique_ptr<EventContainer> container = ssdp.popEvent();
    if (!container)
      continue;
    for (const Buffer& event : container->events) {
      if (auto addEvent = EventPacket::unpackAs<DiscoveryAddEvent>(event)) {
        Logger::log("===Discovery Add Event===");
        Logger::log("Camera ID: %s", container->id.c_str());
        Logger::log("IP address: %s", addEvent->connectionAddress.c_str());
        Logger::log("Serial number: %s", addEvent->serialNumber.c_str());
        Logger::log("Manufacturer: %s", addEvent->manufacturer.c_str());
        Logger::log("Model: %s", addEvent->model.c_str());
        Logger::log("Friendly name: %s", addEvent->name.c_str());
        Logger::log();
      } else if (EventPacket::unpackAs<DiscoveryRemoveEvent>(event)) {
        Logger::log("===Discovery Remove Event===");
        Logger::log("Camera ID: %s", container->id.c_str());
        Logger::log();
      }
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