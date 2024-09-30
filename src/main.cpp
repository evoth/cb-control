#include "packet.h"

#include <iostream>

class TestPacket : public Packet {
 public:
  using Packet::length;
  uint32_t type = 0x07;
  uint32_t vecLen = 0;
  std::vector<uint32_t> vec;
  std::array<uint32_t, 2> arr = {44, 45};

  TestPacket() {
    uint32Length();
    field(type);
    field(vecLen);
    vec.push_back(23);
    vec.push_back(25);
    vector(vec, vecLen);
    array(arr);
  }

  TestPacket(Buffer& buffer) : TestPacket() { unpack(buffer); }

  bool isEnd() { return type == 0; }
};

class TestPacket2 : public Packet {
 public:
  using Packet::length;
  std::vector<std::unique_ptr<TestPacket>> arr;
  TestPacket2() {
    arr.push_back(std::make_unique<TestPacket>());
    arr.push_back(std::make_unique<TestPacket>());
    arr.back()->type = 0;
    packetVector(arr);
    uint32Length();
  }

  TestPacket2(Buffer& buffer) : TestPacket2() { unpack(buffer); }
};

int main() {
  TestPacket2 tim;
  // tim.arr[1]->type = 0x05;
  tim.arr[1]->vec[1] = 99;
  Buffer buff = tim.pack();
  std::cout << std::endl;
  for (auto& e : buff) {
    std::cout << +e << std::endl;
  }

  TestPacket2 newTim;
  newTim.arr[0]->vec[0] = 55;
  newTim.arr[0]->arr[0] = 55;
  newTim.unpack(buff);
  std::cout << std::endl
            << newTim.length << std::endl
            << newTim.arr[0]->length << std::endl
            << newTim.arr[1]->length << std::endl
            << newTim.arr[1]->vec[1] << std::endl;
  return 0;
}