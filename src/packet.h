#ifndef CB_CONTROL_PACKET_H
#define CB_CONTROL_PACKET_H

#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

typedef std::vector<unsigned char> Buffer;

class IField {
 public:
  bool isLengthField = false;
  virtual int pack(Buffer& buffer, int offset) = 0;
  virtual int unpack(Buffer& buffer, int offset) = 0;
  // The way setLength works is icky but it'll do for now
  virtual void setLength(size_t length) = 0;
};

template <class T>
class Field : public IField {
 public:
  Field(
      T& boundValue,
      std::function<int(T& value, Buffer& buffer, int offset)> packValue,
      std::function<int(T& value, Buffer& buffer, int offset)> unpackValue,
      std::function<void(T& value, size_t length)> setLengthValue =
          [](T& value, size_t length) {})
      : boundValue(boundValue),
        packValue(packValue),
        unpackValue(unpackValue),
        setLengthValue(setLengthValue) {}

  int pack(Buffer& buffer, int offset) {
    return packValue(boundValue, buffer, offset);
  }

  int unpack(Buffer& buffer, int offset) {
    return unpackValue(boundValue, buffer, offset);
  }

  void setLength(size_t length) { setLengthValue(boundValue, length); }

 private:
  T& boundValue;
  std::function<int(T& value, Buffer& buffer, int offset)> packValue;
  std::function<int(T& value, Buffer& buffer, int offset)> unpackValue;
  std::function<void(T& value, size_t length)> setLengthValue;
};

class Packet {
 public:
  Buffer pack();
  void unpack(Buffer& buffer);

 protected:
  // Add field that packs/unpacks a uint type
  template <std::unsigned_integral T>
  void uint(T& value) {
    fields.push_back(std::make_unique<Field<T>>(
        value,
        [](T& value, Buffer& buffer, int offset) {
          for (int i = 0; i < sizeof(T); i++) {
            const unsigned char byte = (value >> i * 8) & 0xFF;
            if (offset + i < buffer.size())
              buffer[offset + i] = byte;
            else
              buffer.push_back(byte);
          }
          return sizeof(T);
        },
        [](T& value, Buffer& buffer, int offset) {
          value = 0;
          for (int i = 0; i < sizeof(T); i++) {
            // Current behavior will assume byte=0x00 if out of range
            if (offset + i < buffer.size())
              value |= (buffer[offset + i]) << i * 8;
          }
          return sizeof(T);
        },
        [](T& value, size_t length) { value = length; }));
  }

  void uint16(uint16_t& value) { uint<uint16_t>(value); }
  void uint32(uint32_t& value) { uint<uint32_t>(value); }
  void uint64(uint64_t& value) { uint<uint64_t>(value); }

  // Special length type that will be populated with packet length when packing
  template <std::unsigned_integral T>
  void uintLength(T& value) {
    uint<T>(value);
    fields.back()->isLengthField = true;
  }

  void uint16Length(uint16_t& value) { uintLength<uint16_t>(value); }
  void uint32Length(uint32_t& value) { uintLength<uint32_t>(value); }
  void uint64Length(uint64_t& value) { uintLength<uint64_t>(value); }

 private:
  std::vector<std::unique_ptr<IField>> fields;
};

#endif