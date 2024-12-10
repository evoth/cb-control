#include "ptpData.h"

#include <algorithm>

void PTPString::pack(Buffer& buffer, int& offset) {
  if (string.length() > PTPString::MAX_CHARS)
    string = string.substr(0, PTPString::MAX_CHARS);
  numChars = string.length() + 1;
  Packet::pack(buffer, offset);
};

void PTPString::unpack(Buffer& buffer,
                       int& offset,
                       std::optional<int> limitOffset) {
  int limit = Field::getUnpackLimit(buffer, limitOffset);
  // Deal with "empty" string, which consists of a single 0x00 byte
  if (offset >= limit || buffer[offset] == 0x00) {
    numChars = 0;
    string.clear();
    offset++;
    return;
  }
  Packet::unpack(buffer, offset, limitOffset);
};

bool DeviceInfo::isOpSupported(uint16_t operationCode,
                               uint32_t vendorExtensionId) {
  if (vendorExtensionId != 0 && vendorExtensionId != this->vendorExtensionId)
    return false;
  return std::find(operationsSupported.array.begin(),
                   operationsSupported.array.end(),
                   operationCode) != operationsSupported.array.end();
}