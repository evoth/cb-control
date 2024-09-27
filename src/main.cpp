#include "packet.h"

#include <iostream>

class TestPacket : public Packet {
 public:
  uint32_t length = 0;
  uint32_t type = 0x07;

  TestPacket() {
    uint32Length(length);
    uint32(type);
  }

  TestPacket(Buffer& buffer) : TestPacket() { unpack(buffer); }
};

int main() {
  TestPacket tim;
  Buffer buff = tim.pack();
  std::cout << std::endl;
  for (auto& e : buff) {
    std::cout << +e << std::endl;
  }
  tim.length = 1235;
  tim.type = 555;
  tim.unpack(buff);
  std::cout << std::endl << tim.length << std::endl << tim.type << std::endl;

  TestPacket newTim(buff);
  std::cout << std::endl
            << newTim.length << std::endl
            << newTim.type << std::endl;
  return 0;
}