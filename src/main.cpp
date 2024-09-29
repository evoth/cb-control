#include "packet.h"

#include <iostream>

class TestPacket : public Packet {
 public:
  using Packet::length;
  uint32_t type = 0x07;

  TestPacket() {
    uint32Length();
    bind(type);
  }

  TestPacket(Buffer& buffer) : TestPacket() { unpack(buffer); }
};

class TestPacket2 : public Packet {
 public:
  using Packet::length;
  TestPacket p1;
  TestPacket p2;
  TestPacket2() {
    bind<TestPacket>(p1);
    uint32Length();
    bind<TestPacket>(p2);
  }

  TestPacket2(Buffer& buffer) : TestPacket2() { unpack(buffer); }
};

int main() {
  TestPacket2 tim;
  tim.p2.type = 0x05;
  Buffer buff = tim.pack();
  std::cout << std::endl;
  for (auto& e : buff) {
    std::cout << +e << std::endl;
  }

  TestPacket2 newTim;
  newTim.unpack(buff);
  std::cout << std::endl
            << newTim.length << std::endl
            << newTim.p1.length << std::endl
            << newTim.p2.length << std::endl
            << newTim.p1.type << std::endl
            << newTim.p2.type << std::endl;
  return 0;
}