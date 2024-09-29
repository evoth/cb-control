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
  uint32_t length = 0;
  TestPacket2() {
    bind<TestPacket>();
    uint32Length();
    bind<TestPacket>();
  }

  TestPacket2(Buffer& buffer) : TestPacket2() { unpack(buffer); }
};

// class TestPacket3 : public Packet {
//  public:
//   uint32_t length = 0;
//   std::vector<std::unique_ptr<TestPacket>> tims;
//   uint32_t timeLength = 2;
//   TestPacket3() {
//     fields.push_back(std::make_unique<TestPacket>());
//     uintLength(length);
//     fields.push_back(
//         std::make_unique<Array<TestPacket, uint32_t>>(tims, timeLength));
//   }

//   TestPacket3(Buffer& buffer) : TestPacket3() { unpack(buffer); }
// };

int main() {
  TestPacket2 tim;
  Buffer buff = tim.pack();
  std::cout << std::endl;
  for (auto& e : buff) {
    std::cout << +e << std::endl;
  }

  TestPacket newTim(buff);
  std::cout << std::endl
            << newTim.length << std::endl
            << newTim.type << std::endl;
  return 0;
}