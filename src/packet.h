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
class Vector : public IField {
 public:
  Vector(std::vector<T>& values, uint32_t& lengthRef)
      : IField(lengthRef), values(values) {}

 protected:
  void pack(Buffer& buffer, int& offset) override {
    for (T& value : values)
      Primitive<T>(value).pack(buffer, offset);

    lengthRef = values.size();
  }

  void unpack(Buffer& buffer, int& offset) override {
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

class String : public IField {
 public:
  String(std::string& value) : value(value) {}

 protected:
  void pack(Buffer& buffer, int& offset) override {
    uint16_t wideChar;
    Primitive packer = Primitive(wideChar);
    for (char& c : value) {
      wideChar = c;
      packer.pack(buffer, offset);
    }
    wideChar = 0;
    packer.pack(buffer, offset);
  }

  void unpack(Buffer& buffer, int& offset) override {
    uint16_t wideChar;
    Primitive unpacker = Primitive(wideChar);
    value.clear();
    while (offset < buffer.size()) {
      unpacker.unpack(buffer, offset);
      if (!wideChar)
        break;
      value.push_back(wideChar);
    }
  }

 private:
  std::string& value;
};

// TODO: Will have problems if respective packet doesn't have default
// constructor
// TODo: Figure out better way or simplify
template <typename T, typename... Args>
  requires ChoiceType<T, Args...>
class ChoiceVector : public IField {
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

// TODO: The overloading on field is kind of fun but hurts readability
class Packet : public IField {
 public:
  using IField::length;

  Packet(uint32_t defaultType = 0) : type(defaultType) {
    field();
    field(type);
  }

  void pack(Buffer& buffer, int& offset) override {
    int startOffset = offset;
    for (std::unique_ptr<IField>& field : fields)
      field->pack(buffer, offset);

    lengthRef = offset - startOffset;

    offset = startOffset;
    for (std::unique_ptr<IField>& field : fields)
      field->pack(buffer, offset);
  };

  void unpack(Buffer& buffer, int& offset) override {
    for (std::unique_ptr<IField>& field : fields)
      field->unpack(buffer, offset);
  };

  Buffer pack() {
    Buffer buffer;
    int offset = 0;
    pack(buffer, offset);
    return buffer;
  };

  void unpack(Buffer& buffer) {
    int offset = 0;
    unpack(buffer, offset);
  };

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
    Packet testPacket;
    testPacket.unpack(buffer);
    if (std::unique_ptr<T> packet = testPacket.is<T>()) {
      packet->unpack(buffer);
      return packet;
    }
    return nullptr;
  }

 protected:
  uint32_t type = 0;

  template <std::unsigned_integral T>
  void field(T& value) {
    fields.push_back(std::make_unique<Primitive<T>>(value));
  }

  template <std::unsigned_integral T>
  void field(std::vector<T>& values, uint32_t& lengthRef) {
    fields.push_back(std::make_unique<Vector<T>>(values, lengthRef));
  }

  template <std::unsigned_integral T, size_t N>
  void field(std::array<T, N>& values) {
    fields.push_back(std::make_unique<Array<T, N>>(values));
  }

  void field(std::string& value) {
    fields.push_back(std::make_unique<String>(value));
  }

  template <typename T, typename... Args>
    requires ChoiceType<T, Args...>
  void field(std::vector<std::unique_ptr<T>>& values) {
    fields.push_back(std::make_unique<ChoiceVector<T, Args...>>(values));
  }

  // template <typename... Args>
  // void fields(Args... args) {
  //   (field(args), ...);
  // }

  void field() {
    fields.push_back(std::make_unique<Primitive<uint32_t>>(lengthRef));
  }

 private:
  std::vector<std::unique_ptr<IField>> fields;
};

#endif