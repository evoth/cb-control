#ifndef CB_CONTROL_PACKET_H
#define CB_CONTROL_PACKET_H

#include <array>
#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

typedef std::vector<unsigned char> Buffer;
class Packet;

class IField {
 public:
  IField() : lengthRef(length) {}
  IField(uint32_t& lengthRef) : lengthRef(lengthRef) {}

  virtual int packField(Buffer& buffer, int offset) = 0;
  virtual int unpackField(Buffer& buffer, int offset) = 0;

 protected:
  uint32_t length = 0;
  uint32_t& lengthRef;
  // static(?) getType()
};

// TODO: Restrict template
template <std::unsigned_integral T>
class Primitive : public IField {
 public:
  Primitive(T& value) : value(value) {}

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

 private:
  T& value;
};

template <std::unsigned_integral T>
class Vector : public IField {
 public:
  Vector(std::vector<T>& values, uint32_t& lengthRef)
      : values(values), IField(lengthRef) {}

 protected:
  int packField(Buffer& buffer, int offset) {
    int startOffset = offset;
    for (T& value : values)
      offset += Primitive<T>(value).packField(buffer, offset);

    lengthRef = values.size();

    return offset - startOffset;
  }

  int unpackField(Buffer& buffer, int offset) {
    int startOffset = offset;
    values.clear();
    for (int i = 0; i < lengthRef; i++) {
      T value;
      offset += Primitive<T>(value).unpackField(buffer, offset);
      values.push_back(value);
    }

    return offset - startOffset;
  }

 private:
  std::vector<T>& values;
};

template <std::unsigned_integral T, size_t N>
class Array : public IField {
 public:
  Array(std::array<T, N>& values) : values(values) {}

 protected:
  int packField(Buffer& buffer, int offset) {
    int startOffset = offset;
    for (T value : values)
      offset += Primitive<T>(value).packField(buffer, offset);

    return offset - startOffset;
  }

  int unpackField(Buffer& buffer, int offset) {
    int startOffset = offset;
    for (int i = 0; i < N; i++)
      offset += Primitive<T>(values[i]).unpackField(buffer, offset);

    return offset - startOffset;
  }

 private:
  std::array<T, N>& values;
};

// TODO: Will have problems if respective packet doesn't have default
// constructor
template <typename T>
  requires(std::derived_from<T, Packet>)
class PacketVector : public IField {
 public:
  PacketVector(std::vector<std::unique_ptr<T>>& values) : values(values) {}

 protected:
  int packField(Buffer& buffer, int offset) {
    int startOffset = offset;
    for (std::unique_ptr<T>& value : values)
      offset += value->packField(buffer, offset);

    return offset - startOffset;
  }

  int unpackField(Buffer& buffer, int offset) {
    int startOffset = offset;
    values.clear();
    while (offset < buffer.size()) {
      values.push_back(std::make_unique<T>());
      offset += values.back()->unpackField(buffer, offset);
      if (values.back()->isEnd())
        break;
    }

    return offset - startOffset;
  }

 private:
  std::vector<std::unique_ptr<T>>& values;
};

// The inheritance and constructor are probably bad ideas
class Packet : public IField {
 public:
  int packField(Buffer& buffer, int offset);
  int unpackField(Buffer& buffer, int offset);
  Buffer pack();
  int unpack(Buffer& buffer);
  bool isEnd() { return false; }

 protected:
  template <std::unsigned_integral T>
  void field(T& value) {
    fields.push_back(std::make_unique<Primitive<T>>(value));
  }

  template <std::unsigned_integral T>
  void vector(std::vector<T>& values, uint32_t& lengthRef) {
    fields.push_back(std::make_unique<Vector<T>>(values, lengthRef));
  }

  template <std::unsigned_integral T, size_t N>
  void array(std::array<T, N>& values) {
    fields.push_back(std::make_unique<Array<T, N>>(values));
  }

  template <typename T>
    requires(std::derived_from<T, Packet>)
  void packetVector(std::vector<std::unique_ptr<T>>& values) {
    fields.push_back(std::make_unique<PacketVector<T>>(values));
  }

  void uint32Length() {
    fields.push_back(std::make_unique<Primitive<uint32_t>>(lengthRef));
  }

 private:
  std::vector<std::unique_ptr<IField>> fields;
};

#endif