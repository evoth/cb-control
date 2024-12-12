#ifndef CB_CONTROL_PACKET_H
#define CB_CONTROL_PACKET_H

#include <array>
#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// TODO: Implement switches (based on variants) + vectors to support choice
//       packet vectors
// TODO: Implement typeRef system similar to lengthRef? How to support other
//       data types for lengthRef/typeRef without over-templating everything?
//       (std::variant?)

typedef std::vector<unsigned char> Buffer;
class Packet;

template <typename T, typename... Args>
concept ChoiceType =
    std::derived_from<T, Packet> && (std::derived_from<Args, T> && ...);

// TODO: Move this somewhere else?
int getUnpackLimit(Buffer& buffer, std::optional<int> limitOffset);

template <typename T>
class Packer {
 public:
  virtual void pack(T& value, Buffer& buffer, int& offset) = 0;
  virtual void unpack(T& value,
                      Buffer& buffer,
                      int& offset,
                      std::optional<int> limitOffset) = 0;
};

class IField {
 public:
  virtual void pack(Buffer& buffer, int& offset) = 0;

  virtual void unpack(Buffer& buffer,
                      int& offset,
                      std::optional<int> limitOffset) = 0;
};

template <typename T>
class Field : public IField {
 public:
  Field(std::unique_ptr<Packer<T>> packer, T& value)
      : packer(std::move(packer)), value(value) {}

  void pack(Buffer& buffer, int& offset) override {
    packer->pack(value, buffer, offset);
  }

  void unpack(Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override {
    packer->unpack(value, buffer, offset, limitOffset);
  }

 private:
  std::unique_ptr<Packer<T>> packer;
  T& value;
};

template <std::unsigned_integral T>
class Primitive : public Packer<T> {
 public:
  void pack(T& value, Buffer& buffer, int& offset) override {
    for (int i = 0; i < sizeof(T); i++) {
      unsigned char byte = (value >> i * 8) & 0xFF;
      if (offset < buffer.size())
        buffer[offset] = byte;
      else
        buffer.push_back(byte);
      offset++;
    }
  }

  void unpack(T& value,
              Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override {
    int limit = getUnpackLimit(buffer, limitOffset);
    value = 0;
    for (int i = 0; i < sizeof(T); i++) {
      // Current behavior will assume byte=0x00 if out of range
      if (offset >= limit)
        break;
      value |= buffer[offset] << i * 8;
      offset++;
    }
  }
};

template <typename T>
class Vector : public Packer<std::vector<T>> {
 public:
  Vector(std::unique_ptr<Packer<T>> packer)
      : packer(std::move(packer)), lengthRef(length), greedy(true) {}
  Vector(std::unique_ptr<Packer<T>> packer, uint32_t& lengthRef)
      : packer(std::move(packer)), lengthRef(lengthRef), greedy(false) {}

  void pack(std::vector<T>& values, Buffer& buffer, int& offset) override {
    for (T& value : values)
      packer->pack(value, buffer, offset);

    lengthRef = values.size();
  }

  void unpack(std::vector<T>& values,
              Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override {
    int limit = getUnpackLimit(buffer, limitOffset);
    values.clear();
    for (int i = 0; greedy ? (offset < limit) : (i < lengthRef); i++) {
      values.push_back(T());
      packer->unpack(values.back(), buffer, offset, limitOffset);
    }
  }

 private:
  std::unique_ptr<Packer<T>> packer;
  uint32_t length = 0;
  uint32_t& lengthRef;
  const bool greedy;
};

template <typename T, size_t N>
class Array : public Packer<std::array<T, N>> {
 public:
  Array(std::unique_ptr<Packer<T>> packer) : packer(std::move(packer)) {}

  void pack(std::array<T, N>& values, Buffer& buffer, int& offset) override {
    for (T& value : values)
      packer->pack(value, buffer, offset);
  }

  void unpack(std::array<T, N>& value,
              Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override {
    for (int i = 0; i < N; i++)
      packer->unpack(value[i], buffer, offset, limitOffset);
  }

 private:
  std::unique_ptr<Packer<T>> packer;
};

class WideString : public Packer<std::string> {
 public:
  void pack(std::string& value, Buffer& buffer, int& offset) override;
  void unpack(std::string& value,
              Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override;

 private:
  Primitive<uint16_t> packer;
};

class NestedPacket : public Packer<Packet> {
 public:
  void pack(Packet& value, Buffer& buffer, int& offset) override;
  void unpack(Packet& value,
              Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override;
};

template <typename T, typename... Args>
  requires ChoiceType<T, Args...>
class PacketChoice : public Packer<std::unique_ptr<T>> {
 public:
  void pack(std::unique_ptr<T>& value, Buffer& buffer, int& offset) override {
    if (value)
      value->pack(buffer, offset);
  }

  void unpack(std::unique_ptr<T>& value,
              Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override {
    int startOffset = offset;
    std::unique_ptr<T> basePacket = std::make_unique<T>();
    basePacket->unpack(buffer, offset, limitOffset);

    std::unique_ptr<T> result;
    ((result = std::move(basePacket->template is<Args>())) || ...);

    if (!result) {
      value = std::move(basePacket);
      return;
    }

    offset = startOffset;
    result->unpack(buffer, offset, limitOffset);
    value = std::move(result);
  }
};

// TODO: The overloading on field is kind of fun but hurts readability
class Packet {
 public:
  virtual void pack(Buffer& buffer, int& offset);
  virtual void unpack(Buffer& buffer,
                      int& offset,
                      std::optional<int> limitOffset = std::nullopt);

  Buffer pack();
  void unpack(Buffer& buffer);

  virtual uint32_t getType() { return 0; }

  template <typename T>
    requires(std::derived_from<T, Packet>)
  std::unique_ptr<T> is() {
    std::unique_ptr<T> testPacket = std::make_unique<T>();
    if (getType() == testPacket->getType())
      return testPacket;
    return nullptr;
  }

  template <typename T, typename U>
    requires(std::derived_from<T, Packet> && std::derived_from<U, Packet>)
  static std::unique_ptr<U> unpackAs(Buffer& buffer) {
    T testPacket;
    testPacket.unpack(buffer);
    if (std::unique_ptr<U> packet = testPacket.template is<U>()) {
      packet->unpack(buffer);
      return packet;
    }
    return nullptr;
  }

  template <typename T, typename U>
  static T* cast(std::unique_ptr<U>& ptr) {
    return dynamic_cast<T*>(ptr.get());
  }

 protected:
#define COMMA() ,
#define ADD_FIELD(T_FIELD, T_PACKER, PACKER_ARG, VALUE) \
  fields.push_back(std::make_unique<Field<T_FIELD>>(    \
      std::make_unique<T_PACKER>(PACKER_ARG), VALUE))

  // Primitive (only unsigned int for now)
  template <std::unsigned_integral T>
  void field(T& value) {
    ADD_FIELD(T, Primitive<T>, , value);
  }

  // Non-greedy vector (length determined by lengthRef)
  template <std::unsigned_integral T>
  void field(std::vector<T>& values, uint32_t& lengthRef) {
    ADD_FIELD(std::vector<T>, Vector<T>,
              std::make_unique<Primitive<T>>() COMMA() lengthRef, values);
  }

  // Greedy vector (consumes until end of buffer when unpacking)
  template <std::unsigned_integral T>
  void field(std::vector<T>& values) {
    ADD_FIELD(std::vector<T>, Vector<T>, std::make_unique<Primitive<T>>(),
              values);
  }

  // Fixed-length array
  template <std::unsigned_integral T, size_t N>
  void field(std::array<T, N>& values) {
    ADD_FIELD(std::array<T COMMA() N>, Array<T COMMA() N>,
              std::make_unique<Primitive<T>>(), values);
  }

  // String (encoded as 16 bits/char as in PTP)
  void field(std::string& value) {
    ADD_FIELD(std::string, WideString, , value);
  }

  // Nested packet (packed/unpacked in place)
  void field(Packet& value) { ADD_FIELD(Packet, NestedPacket, , value); }

  // Choice of packets which share a base type, which is dynamically unpacked
  // into an object instance based on getType()
  template <typename T, typename... Args>
    requires ChoiceType<T, Args...>
  void field(std::unique_ptr<T>& value) {
    ADD_FIELD(std::unique_ptr<T>, PacketChoice<T COMMA() Args...>, , value);
  }

  // Special length field which is automatically populated with the packet
  // length when unpacking
  void field() {
    fields.push_back(std::make_unique<Field<uint32_t>>(
        std::make_unique<Primitive<uint32_t>>(), length));
  }

#undef ADD_FIELD
#undef COMMA

 private:
  uint32_t length = 0;
  std::vector<std::unique_ptr<IField>> fields;
};

#endif