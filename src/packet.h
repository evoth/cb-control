#ifndef CB_CONTROL_PACKET_H
#define CB_CONTROL_PACKET_H

#include <array>
#include <concepts>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

// TODO: Replace templates with Concept auto where possible?
// TODO: Use requires clauses in more concise way

typedef std::vector<unsigned char> Buffer;
class Packet;

template <typename T, typename... Args>
concept ChoiceType =
    std::derived_from<T, Packet> && (std::derived_from<Args, T> && ...);

// template <typename T>
// concept PrimitiveType = std::unsigned_integral<T>;

// TODO: Static method to return value while unpacking without needing
// instance
class Field {
 public:
  Field() : lengthRef(length) {}
  Field(uint32_t& lengthRef) : lengthRef(lengthRef) {}

  virtual void pack(Buffer& buffer, int& offset) = 0;
  virtual void unpack(Buffer& buffer, int& offset) = 0;

 protected:
  uint32_t length = 0;
  uint32_t& lengthRef;
};

// TODO: Restrict template
template <std::unsigned_integral T>
class Primitive : public Field {
 public:
  Primitive(T& value) : value(value) {}

  void pack(Buffer& buffer, int& offset) override {
    packFieldHelper(value, buffer, offset);
  }

  void unpack(Buffer& buffer, int& offset) override {
    unpackFieldHelper(value, buffer, offset);
  }

 protected:
  template <std::unsigned_integral U>
  void packFieldHelper(U& value, Buffer& buffer, int& offset) {
    for (int i = 0; i < sizeof(U); i++) {
      unsigned char byte = (value >> i * 8) & 0xFF;
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
class Vector : public Field {
 public:
  Vector(std::vector<T>& values) : values(values), greedy(true) {}
  Vector(std::vector<T>& values, uint32_t& lengthRef)
      : Field(lengthRef), values(values), greedy(false) {}

 protected:
  void pack(Buffer& buffer, int& offset) override {
    for (T& value : values)
      Primitive<T>(value).pack(buffer, offset);

    lengthRef = values.size();
  }

  void unpack(Buffer& buffer, int& offset) override {
    values.clear();
    for (int i = 0; greedy ? (offset < buffer.size()) : (i < lengthRef); i++) {
      T value;
      Primitive<T>(value).unpack(buffer, offset);
      values.push_back(value);
    }
  }

 private:
  std::vector<T>& values;
  bool greedy;
};

template <std::unsigned_integral T, size_t N>
class Array : public Field {
 public:
  Array(std::array<T, N>& values) : values(values) {}

 protected:
  void pack(Buffer& buffer, int& offset) override {
    for (T& value : values)
      Primitive<T>(value).pack(buffer, offset);
  }

  void unpack(Buffer& buffer, int& offset) override {
    for (int i = 0; i < N; i++)
      Primitive<T>(values[i]).unpack(buffer, offset);
  }

 private:
  std::array<T, N>& values;
};

class WideString : public Field {
 public:
  WideString(std::string& value) : value(value) {}

 protected:
  void pack(Buffer& buffer, int& offset) override;
  void unpack(Buffer& buffer, int& offset) override;

 private:
  std::string& value;
};

// TODO: Will have problems if respective packet doesn't have default
// constructor
// TODo: Figure out better way or simplify
template <typename T, typename... Args>
  requires ChoiceType<T, Args...>
class ChoiceVector : public Field {
 public:
  ChoiceVector(std::vector<std::unique_ptr<T>>& values) : values(values) {}

 protected:
  void pack(Buffer& buffer, int& offset) override {
    for (std::unique_ptr<T>& value : values)
      value->pack(buffer, offset);
  }

  void unpack(Buffer& buffer, int& offset) override {
    int startOffset = offset;
    values.clear();
    while (offset < buffer.size()) {
      startOffset = offset;
      values.push_back(choose(buffer, offset));
      offset = startOffset + values.back()->length;
      if (values.back()->isEnd())
        break;
    }
  }

 private:
  std::vector<std::unique_ptr<T>>& values;

  std::unique_ptr<T> choose(Buffer& buffer, int& offset) {
    int startOffset = offset;
    std::unique_ptr<T> testPacket = std::make_unique<T>();
    testPacket->unpack(buffer, offset);

    std::vector<std::unique_ptr<T>> choicesList;
    (choice<Args>(choicesList), ...);
    for (std::unique_ptr<T>& choicePacket : choicesList) {
      if (testPacket->type == choicePacket->type) {
        offset = startOffset;
        choicePacket->unpack(buffer, offset);
        return std::move(choicePacket);
      }
    }
    return testPacket;
  }

  template <typename U>
    requires(std::derived_from<U, T>)
  void choice(std::vector<std::unique_ptr<T>>& choicesList) {
    choicesList.push_back(std::make_unique<U>());
  }
};

class NestedPacket : public Field {
 public:
  NestedPacket(Packet& value) : value(value) {}

 protected:
  void pack(Buffer& buffer, int& offset) override;
  void unpack(Buffer& buffer, int& offset) override;

 private:
  Packet& value;
};

// TODO: The overloading on field is kind of fun but hurts readability
class Packet : public Field {
 public:
  using Field::length;

  // If type is specified, default length and type fields are added
  Packet(uint32_t type) : type(type) {
    field();
    field(this->type);
  }

  Packet() : type(0) {}

  virtual void pack(Buffer& buffer, int& offset) override;
  virtual void unpack(Buffer& buffer, int& offset) override;

  Buffer pack();
  void unpack(Buffer& buffer);

  virtual bool isEnd() { return false; }

  template <typename T>
    requires(std::derived_from<T, Packet>)
  std::unique_ptr<T> is() {
    std::unique_ptr<T> testPacket = std::make_unique<T>();
    if (type == testPacket->type)
      return testPacket;
    return nullptr;
  }

  template <typename T>
    requires(std::derived_from<T, Packet>)
  static std::unique_ptr<T> unpackAs(Buffer& buffer) {
    Packet testPacket(0);
    testPacket.unpack(buffer);
    if (std::unique_ptr<T> packet = testPacket.is<T>()) {
      packet->unpack(buffer);
      return packet;
    }
    return nullptr;
  }

 protected:
  uint32_t type = 0;

  // Primitive (only unsigned int for now)
  template <std::unsigned_integral T>
  void field(T& value) {
    fields.push_back(std::make_unique<Primitive<T>>(value));
  }

  // Non-greedy vector (length determined by lengthRef)
  template <std::unsigned_integral T>
  void field(std::vector<T>& values, uint32_t& lengthRef) {
    fields.push_back(std::make_unique<Vector<T>>(values, lengthRef));
  }

  // Greedy vector (consumes until end of buffer when unpacking)
  template <std::unsigned_integral T>
  void field(std::vector<T>& values) {
    fields.push_back(std::make_unique<Vector<T>>(values));
  }

  // Fixed-length array
  template <std::unsigned_integral T, size_t N>
  void field(std::array<T, N>& values) {
    fields.push_back(std::make_unique<Array<T, N>>(values));
  }

  // String (encoded as 16 bits/char as in PTP)
  void field(std::string& value) {
    fields.push_back(std::make_unique<WideString>(value));
  }

  // Nested packet (packed/unpacked in place)
  void field(Packet& value) {
    fields.push_back(std::make_unique<NestedPacket>(value));
  }

  // Vector of packets which share a base type, which are dynamically unpacked
  // into respective object instances based on default value of the type field
  template <typename T, typename... Args>
    requires ChoiceType<T, Args...>
  void field(std::vector<std::unique_ptr<T>>& values) {
    fields.push_back(std::make_unique<ChoiceVector<T, Args...>>(values));
  }

  // Special length field which is automatically populated with the packet
  // length when unpacking
  void field() {
    fields.push_back(std::make_unique<Primitive<uint32_t>>(lengthRef));
  }

 private:
  std::vector<std::unique_ptr<Field>> fields;
};

#endif