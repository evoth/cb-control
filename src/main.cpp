#include "packet.h"

#include <iostream>

class TestPacket : public Packet {
 public:
  uint32_t vecLen = 0;
  std::vector<uint32_t> vec;
  std::array<uint32_t, 2> arr = {44, 45};
  TestPacket() : Packet(0x07) {
    field(vecLen);
    vec.push_back(23);
    vec.push_back(25);
    vector(vec, vecLen);
    array(arr);
  }
};

class TestPacket2 : public Packet {
 public:
  TestPacket2() : Packet(0x08) {}
};

class TestPacket3 : public Packet {
 public:
  TestPacket3() : Packet(0x0) {
    choice<TestPacket>();
    choice<TestPacket2>();
  }
};

int main() {
  TestPacket tim;
  // tim.arr[1]->type = 0x05;
  tim.arr[1] = 99;
  tim.type = 0x09;
  Buffer buff = tim.pack();
  std::cout << std::endl;
  for (auto& e : buff) {
    std::cout << +e << std::endl;
  }

  TestPacket3 choicePacket;
  std::unique_ptr<Packet> choiceTim = choicePacket.choose(buff);
  if (TestPacket* newTim = dynamic_cast<TestPacket*>(choiceTim.get())) {
    newTim->arr[0] = 55;
    newTim->unpack(buff);
    std::cout << std::endl
              << newTim->length << std::endl
              << newTim->type << std::endl
              << newTim->arr[0] << std::endl
              << newTim->arr[1] << std::endl;
  } else {
    std::cout << "Oops" << std::endl;
  }
  return 0;
}