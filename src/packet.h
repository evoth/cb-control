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
template <typename T>
  requires(std::derived_from<T, Packet>)
class Choice;
template <typename T, typename U>
  requires(std::derived_from<T, Choice<U>>)
class ChoiceVector;

// TODO: Static method to return value while unpacking without needing
// instance
class IField {
 public:
  IField() : lengthRef(length) {}
  IField(uint32_t& lengthRef) : lengthRef(lengthRef) {}

  virtual void pack(Buffer& buffer, int& offset) = 0;
  virtual void unpack(Buffer& buffer, int& offset) = 0;

 protected:
  uint32_t length = 0;
  uint32_t& lengthRef;
};

// TODO: Restrict template
template <std::unsigned_integral T>
class Primitive : public IField {
 public:
  Primitive(T& value) : value(value) {}

  void pack(Buffer& buffer, int& offset) {
    packFieldHelper(value, buffer, offset);
  }

  void unpack(Buffer& buffer, int& offset) {
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
  void pack(Buffer& buffer, int& offset) {
    for (T& value : values)
      Primitive<T>(value).pack(buffer, offset);

    lengthRef = values.size();
  }

  void unpack(Buffer& buffer, int& offset) {
    values.clear();
    for (int i = 0; i < lengthRef; i++) {
      T value;
      Primitive<T>(value).unpack(buffer, offset);
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
  void pack(Buffer& buffer, int& offset) {
    for (T value : values)
      Primitive<T>(value).pack(buffer, offset);
  }

  void unpack(Buffer& buffer, int& offset) {
    for (int i = 0; i < N; i++)
      Primitive<T>(values[i]).unpack(buffer, offset);
  }

 private:
  std::array<T, N>& values;
};

// TODO: Redundant overloads?
class Packet : public IField {
 public:
  using IField::length;
  uint32_t type = 0;

  Packet(uint32_t defaultType = 0) : type(defaultType) {
    uint32Length();
    field(type);
  }

  void pack(Buffer& buffer, int& offset);
  void unpack(Buffer& buffer, int& offset);
  Buffer pack();
  void unpack(Buffer& buffer);

  virtual bool isEnd() { return false; }

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

  template <typename T, typename U>
    requires(std::derived_from<T, Choice<U>>)
  void choiceVector(std::vector<std::unique_ptr<U>>& values) {
    fields.push_back(std::make_unique<ChoiceVector<T, U>>(values));
  }

  void uint32Length() {
    fields.push_back(std::make_unique<Primitive<uint32_t>>(lengthRef));
  }

 private:
  std::vector<std::unique_ptr<IField>> fields;
};

// TODo: Figure out what I did and why
template <typename T>
  requires(std::derived_from<T, Packet>)
class Choice : public Packet {
 public:
  Choice(uint32_t defaultType = 0) : Packet(defaultType) {}

  std::unique_ptr<T> unpackPointer(Buffer& buffer, int& offset) {
    int startOffset = offset;
    std::unique_ptr<T> testPacket = std::make_unique<T>();
    testPacket->unpack(buffer, offset);
    choicesList.clear();
    choices();
    for (std::unique_ptr<T>& choicePacket : choicesList) {
      if (testPacket->type == choicePacket->type) {
        offset = startOffset;
        choicePacket->unpack(buffer, offset);
        return std::move(choicePacket);
      }
    }
    choicesList.clear();
    return testPacket;
  }

 protected:
  template <typename U>
    requires(std::derived_from<U, T>)
  void choice() {
    choicesList.push_back(std::make_unique<U>());
  }

  virtual void choices() {}

 private:
  std::vector<std::unique_ptr<T>> choicesList;
};

// TODO: Will have problems if respective packet doesn't have default
// constructor
// TODo: Figure out what I did and why
template <typename T, typename U>
  requires(std::derived_from<T, Choice<U>>)
class ChoiceVector : public IField {
 public:
  ChoiceVector(std::vector<std::unique_ptr<U>>& values) : values(values) {}

 protected:
  void pack(Buffer& buffer, int& offset) {
    for (std::unique_ptr<U>& value : values)
      value->pack(buffer, offset);
  }

  void unpack(Buffer& buffer, int& offset) {
    int startOffset = offset;
    values.clear();
    while (offset < buffer.size()) {
      startOffset = offset;
      T choicePacket;
      values.push_back(choicePacket.unpackPointer(buffer, offset));
      offset = startOffset + values.back()->length;
      if (values.back()->isEnd())
        break;
    }
  }

 private:
  std::vector<std::unique_ptr<U>>& values;
};

#endif