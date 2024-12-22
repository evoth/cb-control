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
  int limit = getUnpackLimit(buffer.size(), limitOffset);
  // Deal with "empty" string, which consists of a single 0x00 byte
  if (offset >= limit || buffer[offset] == 0x00) {
    numChars = 0;
    string.clear();
    if (offset < limit)
      offset++;
    return;
  }
  Packet::unpack(buffer, offset, limitOffset);
};

bool DeviceInfo::isOpSupported(uint16_t operationCode,
                               uint32_t vendorExtensionId) {
  if (vendorExtensionId != 0 && vendorExtensionId != this->vendorExtensionId)
    return false;
  return std::find(operationsSupported.begin(), operationsSupported.end(),
                   operationCode) != operationsSupported.end();
}

// TODO: Reduce duplication?
bool DeviceInfo::isPropSupported(uint16_t propertyCode,
                                 uint32_t vendorExtensionId) {
  if (vendorExtensionId != 0 && vendorExtensionId != this->vendorExtensionId)
    return false;
  return std::find(devicePropertiesSupported.begin(),
                   devicePropertiesSupported.end(),
                   propertyCode) != devicePropertiesSupported.end();
}

// 128-bit data types not currently supported
const std::map<std::type_index, uint16_t> DataTypeMap = {
    {typeid(int8_t), DataType::INT8},
    {typeid(uint8_t), DataType::UINT8},
    {typeid(int16_t), DataType::INT16},
    {typeid(uint16_t), DataType::UINT16},
    {typeid(int32_t), DataType::INT32},
    {typeid(uint32_t), DataType::UINT32},
    {typeid(int64_t), DataType::INT64},
    {typeid(uint64_t), DataType::UINT64},
    {typeid(std::vector<int8_t>), DataType::AINT8},
    {typeid(std::vector<uint8_t>), DataType::AUINT8},
    {typeid(std::vector<int16_t>), DataType::AINT16},
    {typeid(std::vector<uint16_t>), DataType::AUINT16},
    {typeid(std::vector<int32_t>), DataType::AINT32},
    {typeid(std::vector<uint32_t>), DataType::AUINT32},
    {typeid(std::vector<int64_t>), DataType::AINT64},
    {typeid(std::vector<uint64_t>), DataType::AUINT64},
    {typeid(std::string), DataType::STR},
};