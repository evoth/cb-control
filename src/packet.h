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
  IField() : lengthRef(length) {}
  IField(uint32_t& lengthRef) : lengthRef(lengthRef) {}

  virtual int packField(Buffer& buffer, int offset) = 0;
  virtual int unpackField(Buffer& buffer, int offset) = 0;

 protected:
  uint32_t length = 0;
  uint32_t& lengthRef;
};

template <typename T>
concept FieldType = std::is_base_of_v<IField, T> || std::unsigned_integral<T>;

// TODO: Restrict template
template <typename T>
class Field : public IField {
 public:
  Field(T& value) : value(value) {}

  int packField(Buffer& buffer, int offset) {
    return packFieldHelper(value, buffer, offset);
  }

  int unpackField(Buffer& buffer, int offset) {
    return unpackFieldHelper(value, buffer, offset);
  }

 protected:
  template <std::unsigned_integral U>
  int packFieldHelper(U& value, Buffer& buffer, int offset) {
    for (int i = 0; i < sizeof(U); i++) {
      const unsigned char byte = (value >> i * 8) & 0xFF;
      if (offset + i < buffer.size())
        buffer[offset + i] = byte;
      else
        buffer.push_back(byte);
    }
    return sizeof(U);
  }

  int packFieldHelper(IField& value, Buffer& buffer, int offset) {
    return value.packField(buffer, offset);
  }

  template <std::unsigned_integral U>
  int unpackFieldHelper(U& value, Buffer& buffer, int offset) {
    value = 0;
    for (int i = 0; i < sizeof(U); i++) {
      // Current behavior will assume byte=0x00 if out of range
      if (offset + i < buffer.size())
        value |= (buffer[offset + i]) << i * 8;
    }
    return sizeof(U);
  }

  int unpackFieldHelper(IField& value, Buffer& buffer, int offset) {
    return value.unpackField(buffer, offset);
  }

 private:
  T& value;
};

template <std::unsigned_integral T>
class Array : public IField {
 public:
  Array(std::vector<T>& values, uint32_t& lengthRef)
      : values(values), IField(lengthRef) {}

 protected:
  int packField(Buffer& buffer, int offset) {
    int startOffset = offset;
    for (T& value : values) {
      offset += Field<T>(value).packField(buffer, offset);
    }

    lengthRef = values.size();

    return offset - startOffset;
  }

  int unpackField(Buffer& buffer, int offset) {
    int startOffset = offset;

    values.clear();
    for (int i = 0; i < lengthRef; i++) {
      T value;
      offset += Field<T>(value).unpackField(buffer, offset);
      values.push_back(value);
    }

    return offset - startOffset;
  }

 private:
  std::vector<T>& values;
};

// The inheritance and constructor are probably bad ideas
class Packet : public Field<Packet> {
 public:
  Packet() : Field<Packet>(*this) {};

  int packField(Buffer& buffer, int offset);
  int unpackField(Buffer& buffer, int offset);
  Buffer pack();
  int unpack(Buffer& buffer);

 protected:
  template <FieldType T>
  void field(T& value) {
    fields.push_back(std::make_unique<Field<T>>(value));
  }

  template <FieldType T>
  void array(std::vector<T>& values, uint32_t& lengthRef) {
    fields.push_back(std::make_unique<Array<T>>(values, lengthRef));
  }

  void uint32Length() {
    fields.push_back(std::make_unique<Field<uint32_t>>(lengthRef));
  }

 private:
  std::vector<std::unique_ptr<IField>> fields;
};

#endif