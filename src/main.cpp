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

class TestPacket3 : public Choice<Packet> {
 public:
  void choices() {
    choice<TestPacket>();
    choice<TestPacket2>();
  }
};

class TestPacketChoice : public Choice<TestPacket> {};

class TestPacket4 : public Packet {
 public:
  std::vector<std::unique_ptr<Packet>> vec;
  TestPacket4() : Packet(0x09) { choiceVector<TestPacket3>(vec); }
};

int main() {
  TestPacket4 tim;
  // tim.arr[1]->type = 0x05;
  tim.vec.push_back(std::make_unique<TestPacket2>());
  std::unique_ptr<TestPacket> oops = std::make_unique<TestPacket>();
  oops->arr[0] = 80;
  tim.vec.push_back(std::move(oops));
  Buffer buff = tim.pack();
  std::cout << std::endl;
  for (auto& e : buff) {
    std::cout << +e << std::endl;
  }

  TestPacket4 choiceTim;
  choiceTim.unpack(buff);
  if (TestPacket* newTim = dynamic_cast<TestPacket*>(choiceTim.vec[1].get())) {
    // newTim->arr[0] = 55;
    // newTim->unpack(buff);
    std::cout << std::endl
              << newTim->length << std::endl
              << newTim->type << std::endl
              << newTim->arr[0] << std::endl
              << newTim->arr[1] << std::endl;
  } else {
    std::cout << "Oops" << std::endl;
  }

  // TestPacket4 tim;
  // tim.vec[0]->arr[0] = 1000;
  // tim.vec[1]->arr[1] = 2000;

  // Buffer buff = tim.pack();
  // std::cout << std::endl;
  // for (auto& e : buff) {
  //   std::cout << +e << std::endl;
  // }

  // TestPacket4 newTim;

  // newTim.unpack(buff);

  // newTim.vec[0]->arr[0] = 55;
  // newTim.unpack(buff);
  // std::cout << std::endl
  //           << newTim.length << std::endl
  //           << newTim.type << std::endl
  //           << newTim.vec[0]->arr[0] << std::endl
  //           << newTim.vec[1]->arr[1] << std::endl;

  return 0;
}