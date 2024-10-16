#include "ptpip.h"

#include <iomanip>
#include <iostream>

int main() {
  InitCommandRequest initCommandRequest(
      {0x4d, 0xc4, 0x79, 0xe1, 0xc1, 0x0e, 0x46, 0x27, 0x9e, 0xe1, 0x1b, 0x2a,
       0xbc, 0xe2, 0xda, 0x29},
      "iPhone");
  Buffer buff = initCommandRequest.pack();
  std::cout << std::endl;
  int i = 0;
  for (auto& e : buff) {
    std::cout << std::hex << std::setfill('0') << std::setw(2) << +e
              << std::resetiosflags(std::ios::hex) << " ";
    i++;
    if (i % 16 == 8)
      std::cout << " ";
    if (i % 16 == 0)
      std::cout << std::endl;
  }
  std::cout << std::endl << std::endl;

  InitCommandRequest initCommandRequest2;
  initCommandRequest2.unpack(buff);
  std::cout << initCommandRequest2.name << std::endl;

  return 0;
}

// class TestPacket : public Packet {
//  public:
//   uint32_t vecLen = 0;
//   std::vector<uint32_t> vec;
//   std::array<uint32_t, 2> arr = {44, 45};
//   std::string s = "Hello Tim";
//   TestPacket() : Packet(0x07) {
//     field(vecLen);
//     field(s);
//     vec.push_back(23);
//     vec.push_back(25);
//     field(vec, vecLen);
//     field(arr);
//   }
// };

// class TestPacket2 : public Packet {
//  public:
//   TestPacket2() : Packet(0x08) {}
// };

// class TestPacket4 : public Packet {
//  public:
//   std::vector<std::unique_ptr<Packet>> vec;
//   TestPacket4() : Packet(0x09) { field<Packet, TestPacket, TestPacket2>(vec);
//   }
// };

// int main() {
//   TestPacket4 tim;
//   // tim.arr[1]->type = 0x05;
//   tim.vec.push_back(std::make_unique<TestPacket2>());
//   std::unique_ptr<TestPacket> oops = std::make_unique<TestPacket>();
//   oops->arr[0] = 80;
//   oops->s = "Tim";
//   tim.vec.push_back(std::move(oops));
//   Buffer buff = tim.pack();
//   std::cout << std::endl;
//   for (auto& e : buff) {
//     std::cout << +e << std::endl;
//   }

//   TestPacket4 choiceTim;
//   choiceTim.unpack(buff);
//   if (TestPacket* newTim = dynamic_cast<TestPacket*>(choiceTim.vec[1].get()))
//   {
//     // newTim->arr[0] = 55;
//     // newTim->unpack(buff);
//     std::cout << std::endl
//               << newTim->length << std::endl
//               << newTim->type << std::endl
//               << newTim->s << std::endl
//               << newTim->arr[0] << std::endl
//               << newTim->arr[1] << std::endl;
//   } else {
//     std::cout << "Oops" << std::endl;
//   }

//   // TestPacket4 tim;
//   // tim.vec[0]->arr[0] = 1000;
//   // tim.vec[1]->arr[1] = 2000;

//   // Buffer buff = tim.pack();
//   // std::cout << std::endl;
//   // for (auto& e : buff) {
//   //   std::cout << +e << std::endl;
//   // }

//   // TestPacket4 newTim;

//   // newTim.unpack(buff);

//   // newTim.vec[0]->arr[0] = 55;
//   // newTim.unpack(buff);
//   // std::cout << std::endl
//   //           << newTim.length << std::endl
//   //           << newTim.type << std::endl
//   //           << newTim.vec[0]->arr[0] << std::endl
//   //           << newTim.vec[1]->arr[1] << std::endl;

//   return 0;
// }