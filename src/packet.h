#ifndef CB_CONTROL_PACKET_H
#define CB_CONTROL_PACKET_H

#include <array>
#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

typedef std::vector<unsigned char> Buffer;
class Packet;

// TODO: Move this somewhere else?
int getUnpackLimit(int hardLimit, std::optional<int> limitOffset);

class ISpecialPropRef {
 public:
  virtual uint32_t get() = 0;
  virtual void set(uint32_t newValue) = 0;
};

template <std::unsigned_integral T>
class SpecialPropRef : public ISpecialPropRef {
 public:
  SpecialPropRef(T& value) : value(value) {}

  uint32_t get() override { return static_cast<uint32_t>(value); }

  void set(uint32_t newValue) override { value = static_cast<T>(newValue); }

 private:
  T& value;
};

class SpecialProp {
 public:
  uint32_t get() {
    if (prop)
      return prop->get();
    return 0;
  }

  void set(uint32_t value) {
    if (prop)
      prop->set(value);
  }

  template <std::unsigned_integral T>
  void bind(T& ref) {
    prop = std::make_unique<SpecialPropRef<T>>(ref);
  }

 private:
  std::unique_ptr<ISpecialPropRef> prop;
};

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

template <std::integral T>
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
    int limit = getUnpackLimit(buffer.size(), limitOffset);
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

template <typename T, std::unsigned_integral U = uint32_t>
class Vector : public Packer<std::vector<T>> {
 public:
  Vector(std::unique_ptr<Packer<T>> packer)
      : packer(std::move(packer)), greedy(true) {}
  Vector(std::unique_ptr<Packer<T>> packer, U& lengthRef)
      : packer(std::move(packer)), greedy(false) {
    _length.bind(lengthRef);
  }

  void pack(std::vector<T>& values, Buffer& buffer, int& offset) override {
    for (T& value : values)
      packer->pack(value, buffer, offset);

    _length.set(values.size());
  }

  void unpack(std::vector<T>& values,
              Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override {
    int limit = getUnpackLimit(buffer.size(), limitOffset);
    values.clear();
    for (int i = 0; offset < limit && (greedy || i < _length.get()); i++) {
      values.push_back(T());
      packer->unpack(values.back(), buffer, offset, limitOffset);
    }
  }

 private:
  std::unique_ptr<Packer<T>> packer;
  SpecialProp _length;
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

// TODO: Get rid of this since Packet can be its own field?
class NestedPacket : public Packer<Packet> {
 public:
  void pack(Packet& value, Buffer& buffer, int& offset) override;
  void unpack(Packet& value,
              Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override;
};

template <typename T>
  requires std::derived_from<T, Packet>
class PacketBuffer : public Packer<Buffer> {
 public:
  PacketBuffer() : packer(std::make_unique<Primitive<unsigned char>>()) {}

  void pack(Buffer& value, Buffer& buffer, int& offset) override {
    packer.pack(value, buffer, offset);
  }

  void unpack(Buffer& value,
              Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override {
    int startOffset = offset;
    lengthPacket.unpack(buffer, offset, limitOffset);

    limitOffset =
        getUnpackLimit(startOffset + lengthPacket.getLength(), limitOffset);
    offset = startOffset;
    packer.unpack(value, buffer, offset, limitOffset);
  }

 private:
  T lengthPacket;
  Vector<unsigned char> packer;
};

// TODO: The overloading on field is kind of fun but hurts readability
class Packet : public IField {
 public:
  virtual void pack(Buffer& buffer, int& offset) override;
  virtual void unpack(Buffer& buffer,
                      int& offset,
                      std::optional<int> limitOffset = std::nullopt) override;

  Buffer pack();
  void unpack(Buffer& buffer);

  template <typename T>
    requires(std::derived_from<T, Packet>)
  std::unique_ptr<T> is() {
    std::unique_ptr<T> testPacket = std::make_unique<T>();
    if (_type.get() == testPacket->_type.get())
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

  uint32_t getLength() { return _length.get(); }
  uint32_t getType() { return _type.get(); }

#define COMMA() ,
#define ADD_FIELD(T_FIELD, T_PACKER, PACKER_ARG, VALUE) \
  fields.push_back(std::make_unique<Field<T_FIELD>>(    \
      std::make_unique<T_PACKER>(PACKER_ARG), VALUE))

  // Primitive (int for now)
  template <std::integral T>
  void field(T& value) {
    ADD_FIELD(T, Primitive<T>, , value);
  }

  // Non-greedy vector (length determined by lengthRef)
  template <std::integral T, std::unsigned_integral U>
  void field(std::vector<T>& values, U& lengthRef) {
    ADD_FIELD(std::vector<T>, Vector<T COMMA() U>,
              std::make_unique<Primitive<T>>() COMMA() lengthRef, values);
  }

  // Greedy vector (consumes until end of packet when unpacking)
  template <std::integral T>
  void field(std::vector<T>& values) {
    ADD_FIELD(std::vector<T>, Vector<T>, std::make_unique<Primitive<T>>(),
              values);
  }

  // Fixed-length array
  template <std::integral T, size_t N>
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

  // Buffer which automagically reads the length of a nested packet and only
  // consumes the respective number of bytes
  template <typename T>
    requires std::derived_from<T, Packet>
  void field(Buffer& value) {
    ADD_FIELD(Buffer, PacketBuffer<T>, , value);
  }

  // Vector of PacketBuffers
  template <typename T>
    requires std::derived_from<T, Packet>
  void field(std::vector<Buffer>& values) {
    ADD_FIELD(std::vector<Buffer>, Vector<Buffer>,
              std::make_unique<PacketBuffer<T>>(), values);
  }

  // Special length field which is automatically populated with the packet
  // length when unpacking
  template <std::unsigned_integral T>
  void lengthField(T& value) {
    field(value);
    _length.bind(value);
  }

  // Special type field which is used when conditionally unpacking
  template <std::unsigned_integral T>
  void typeField(T& value) {
    field(value);
    _type.bind(value);
  }

#undef ADD_FIELD
#undef COMMA

 protected:
  std::vector<std::unique_ptr<IField>> fields;

 private:
  SpecialProp _length;
  SpecialProp _type;
};

#endif