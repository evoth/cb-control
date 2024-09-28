#ifndef CB_CONTROL_PACKET_H
#define CB_CONTROL_PACKET_H

#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

typedef std::vector<unsigned char> Buffer;

// TODO: Make this less cursed
class IField {
 public:
  bool isLengthField;

  IField(bool isLengthField = false) : isLengthField(isLengthField) {}

  virtual int packField(Buffer& buffer, int offset) = 0;
  virtual int unpackField(Buffer& buffer, int offset) = 0;
  // The way setLength works is icky but it'll do for now
  virtual void setLength(size_t length) {};
};

template <typename T>
concept FieldType = std::is_base_of_v<IField, T>;

template <std::unsigned_integral T>
class Uint : public IField {
 public:
  Uint(T& value, bool isLengthField = false)
      : value(value), IField(isLengthField) {}

  int packField(Buffer& buffer, int offset) {
    for (int i = 0; i < sizeof(T); i++) {
      const unsigned char byte = (value >> i * 8) & 0xFF;
      if (offset + i < buffer.size())
        buffer[offset + i] = byte;
      else
        buffer.push_back(byte);
    }
    return sizeof(T);
  }

  int unpackField(Buffer& buffer, int offset) {
    value = 0;
    for (int i = 0; i < sizeof(T); i++) {
      // Current behavior will assume byte=0x00 if out of range
      if (offset + i < buffer.size())
        value |= (buffer[offset + i]) << i * 8;
    }
    return sizeof(T);
  }

  void setLength(size_t length) { value = length; }

 private:
  T& value;
};

// template <FieldType T, std::unsigned_integral U>
// class Array : public IField {
//  public:
//   Array(std::vector<std::unique_ptr<T>>& value, U& lengthValue)
//       : value(value), lengthValue(lengthValue) {}

//   int packField(Buffer& buffer, int offset) {
//     int startOffset = offset;
//     for (std::unique_ptr<T>& field : value) {
//       offset += field->packField(buffer, offset);
//     }
//     // Set lengthValue and pack it? Oh no this is getting complicated
//     return offset - startOffset;
//   }

//   int unpackField(Buffer& buffer, int offset) {
//     int startOffset = offset;
//     value.clear();
//     for (int i = 0; i < lengthValue; i++) {
//       value.push_back(std::make_unique<T>());
//       offset += value.back()->unpackField(buffer, offset);
//     }
//     return offset - startOffset;
//   }

//  private:
//   U& lengthValue;
//   std::vector<std::unique_ptr<T>>& value;
// };

class Packet : public IField {
 public:
  int packField(Buffer& buffer, int offset);
  int unpackField(Buffer& buffer, int offset);
  Buffer pack();
  int unpack(Buffer& buffer);

  // TODO: Make private again
  std::vector<std::unique_ptr<IField>> fields;

 protected:
  // Add field that packs/unpacks a uint type
  template <std::unsigned_integral T>
  void uint(T& value) {
    fields.push_back(std::make_unique<Uint<T>>(value));
  }

  // Special length type that will be populated with packet length when packing
  template <std::unsigned_integral T>
  void uintLength(T& value) {
    fields.push_back(std::make_unique<Uint<T>>(value, true));
  }

 private:
};

#endif