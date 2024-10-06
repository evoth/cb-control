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

// TODO: Static method to return value while unpacking without needing instance
class IField {
 public:
  IField() : lengthRef(length) {}
  IField(uint32_t& lengthRef) : lengthRef(lengthRef) {}

  virtual void packField(Buffer& buffer, int& offset) = 0;
  virtual void unpackField(Buffer& buffer, int& offset) = 0;

 protected:
  uint32_t length = 0;
  uint32_t& lengthRef;
};

// TODO: Restrict template
template <std::unsigned_integral T>
class Primitive : public IField {
 public:
  Primitive(T& value) : value(value) {}

  void packField(Buffer& buffer, int& offset) {
    packFieldHelper(value, buffer, offset);
  }

  void unpackField(Buffer& buffer, int& offset) {
    unpackFieldHelper(value, buffer, offset);
  }

 protected:
  template <std::unsigned_integral U>
  void packFieldHelper(U& value, Buffer& buffer, int& offset) {
    for (int i = 0; i < sizeof(U); i++) {
      const unsigned char byte = (value >> i * 8) & 0xFF;
      if (offset + i < buffer.size())
        buffer[offset + i] = byte;
      else
        buffer.push_back(byte);
    }
    offset += sizeof(U);
  }

  template <std::unsigned_integral U>
  void unpackFieldHelper(U& value, Buffer& buffer, int& offset) {
    value = 0;
    for (int i = 0; i < sizeof(U); i++) {
      // Current behavior will assume byte=0x00 if out of range
      if (offset + i < buffer.size())
        value |= (buffer[offset + i]) << i * 8;
    }
    offset += sizeof(U);
  }

 private:
  T& value;
};

template <std::unsigned_integral T>
class Vector : public IField {
 public:
  Vector(std::vector<T>& values, uint32_t& lengthRef)
      : IField(lengthRef), values(values) {}

 protected:
  void packField(Buffer& buffer, int& offset) {
    for (T& value : values)
      Primitive<T>(value).packField(buffer, offset);

    lengthRef = values.size();
  }

  void unpackField(Buffer& buffer, int& offset) {
    values.clear();
    for (int i = 0; i < lengthRef; i++) {
      T value;
      Primitive<T>(value).unpackField(buffer, offset);
      values.push_back(value);
    }
  }

 private:
  std::vector<T>& values;
};

template <std::unsigned_integral T, size_t N>
class Array : public IField {
 public:
  Array(std::array<T, N>& values) : values(values) {}

 protected:
  void packField(Buffer& buffer, int& offset) {
    for (T value : values)
      Primitive<T>(value).packField(buffer, offset);
  }

  void unpackField(Buffer& buffer, int& offset) {
    for (int i = 0; i < N; i++)
      Primitive<T>(values[i]).unpackField(buffer, offset);
  }

 private:
  std::array<T, N>& values;
};

// TODO: Will have problems if respective packet doesn't have default
// constructor
// class PacketVector : public IField {
//  public:
//   PacketVector(std::vector<std::unique_ptr<Packet>>& values) : values(values)
//   {}

//  protected:
//   void packField(Buffer& buffer, int& offset) {
//     for (std::unique_ptr<Packet>& value : values)
//       value->packField(buffer, offset);
//   }

//   void unpackField(Buffer& buffer, int& offset) {
//     // int startOffset = offset;
//     // values.clear();
//     // while (offset < buffer.size()) {
//     //   values.push_back(std::make_unique<Packet>());
//     //   offset += values.back()->unpackField(buffer, offset);
//     //   if (values.back()->isEnd())
//     //     break;
//     // }
//   }

//  private:
//   std::vector<std::unique_ptr<Packet>>& values;
// };

// The inheritance and constructor are probably bad ideas
class Packet : public IField {
 public:
  using IField::length;
  uint32_t type = 0;

  Packet(uint32_t defaultType) : type(defaultType) {
    uint32Length();
    field(type);
  }

  void packField(Buffer& buffer, int& offset);
  void unpackField(Buffer& buffer, int& offset);
  Buffer pack();
  void unpack(Buffer& buffer);
  bool isEnd() { return false; }

  std::unique_ptr<Packet> choose(Buffer& buffer) {
    std::unique_ptr<Packet> testPacket = std::make_unique<Packet>(0);
    testPacket->unpack(buffer);
    for (std::unique_ptr<Packet>& choicePacket : choices) {
      if (testPacket->type == choicePacket->type) {
        choicePacket->unpack(buffer);
        return std::move(choicePacket);
      }
    }
    return testPacket;
  }

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

  // void packetVector(std::vector<std::unique_ptr<Packet>>& values) {
  //   fields.push_back(std::make_unique<PacketVector>(values));
  // }

  void uint32Length() {
    fields.push_back(std::make_unique<Primitive<uint32_t>>(lengthRef));
  }

  template <typename T>
    requires(std::derived_from<T, Packet>)
  void choice() {
    choices.push_back(std::make_unique<T>());
  }

 private:
  std::vector<std::unique_ptr<IField>> fields;
  std::vector<std::unique_ptr<Packet>> choices;
};

#endif