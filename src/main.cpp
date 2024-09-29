#include "packet.h"

#include <iostream>

class TestPacket : public Packet {
 public:
  using Packet::length;
  uint32_t type = 0x07;
  uint32_t vecLen = 0;
  std::vector<uint32_t> vec;

  TestPacket() {
    uint32Length();
    field(type);
    field(vecLen);
    vec.push_back(23);
    vec.push_back(25);
    array(vec, vecLen);
  }

  TestPacket(Buffer& buffer) : TestPacket() { unpack(buffer); }
};

class TestPacket2 : public Packet {
 public:
  using Packet::length;
  TestPacket p1;
  TestPacket p2;
  TestPacket2() {
    field<TestPacket>(p1);
    uint32Length();
    field<TestPacket>(p2);
  }

  TestPacket2(Buffer& buffer) : TestPacket2() { unpack(buffer); }
};

int main() {
  TestPacket2 tim;
  tim.p2.type = 0x05;
  tim.p2.vec[1] = 99;
  Buffer buff = tim.pack();
  std::cout << std::endl;
  for (auto& e : buff) {
    std::cout << +e << std::endl;
  }

  TestPacket2 newTim;
  newTim.p1.vec[0] = 55;
  newTim.unpack(buff);
  std::cout << std::endl
            << newTim.length << std::endl
            << newTim.p1.length << std::endl
            << newTim.p2.length << std::endl
            << newTim.p1.type << std::endl
            << newTim.p2.type << std::endl
            << newTim.p1.vec[0] << std::endl;
  return 0;
}