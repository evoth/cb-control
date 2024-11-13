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

  for (int i = 0; i < 3; i++) {
    camera->connect();
    std::this_thread::sleep_for(std::chrono::seconds(5));

    camera->releaseShutter();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    camera->releaseShutter();
    std::this_thread::sleep_for(std::chrono::seconds(5));

    camera->disconnect();
    std::this_thread::sleep_for(std::chrono::seconds(5));
  }

  return 0;
}

// int main() {
//   InitCommandRequest initCommandRequest(
//       {0x4d, 0xc4, 0x79, 0xe1, 0xc1, 0x0e, 0x46, 0x27, 0x9e, 0xe1, 0x1b,
//       0x2a,
//        0xbc, 0xe2, 0xda, 0x29},
//       "iPhone");
//   Buffer buff = initCommandRequest.pack();
//   int i = 0;
//   for (auto& e : buff) {
//     std::stringstream ss;
//     ss << std::hex << std::setfill('0') << std::setw(2) << +e
//        << std::resetiosflags(std::ios::hex) << " ";
//     Serial.print(ss.str().c_str());
//     i++;
//     if (i % 16 == 8)
//       Serial.print(" ");
//     if (i % 16 == 0)
//       Serial.println();
//   }
//   Serial.println();
//   Serial.println();

//   InitCommandRequest initCommandRequest2;
//   initCommandRequest2.unpack(buff);
//   Serial.println(initCommandRequest2.name.c_str());

//   return 0;
// }

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