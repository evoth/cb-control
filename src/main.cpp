#include "factory.h"

#include <iostream>

int main() {
  const std::array<uint8_t, 16> guid(
      {7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7});

  std::unique_ptr<Camera> camera =
      PTPIPFactory(guid, "Tim", "192.168.4.7").create();

  if (!camera) {
    std::cerr << "Failed to create camera." << std::endl;
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