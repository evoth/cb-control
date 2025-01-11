#ifndef CB_CONTROL_PACKET_H
#define CB_CONTROL_PACKET_H

#include <array>
#include <concepts>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

typedef std::vector<uint8_t> Buffer;
class Packet;

// TODO: Move this somewhere else?
int getUnpackLimit(int hardLimit, std::optional<int> limitOffset);

template <std::unsigned_integral T>
class ISpecialPropRef {
 public:
  virtual ~ISpecialPropRef() = default;

  virtual T get() const = 0;
  virtual void set(T newValue) = 0;
};

template <std::unsigned_integral T, std::unsigned_integral U>
class SpecialPropRef : public ISpecialPropRef<T> {
 public:
  SpecialPropRef(U& value) : value(value) {}

  T get() const override { return static_cast<T>(value); }

  void set(T newValue) override { value = static_cast<U>(newValue); }

 private:
  U& value;
};

template <std::unsigned_integral T>
class SpecialProp {
 public:
  T get() const {
    if (prop)
      return prop->get();
    return 0;
  }

  void set(T value) {
    if (prop)
      prop->set(value);
  }

  template <std::unsigned_integral U>
  void bind(U& ref) {
    prop = std::make_unique<SpecialPropRef<T, U>>(ref);
  }

  bool isBound() { return prop != nullptr; }

 private:
  std::unique_ptr<ISpecialPropRef<T>> prop;
};

template <typename T>
class Packer {
 public:
  virtual ~Packer() = default;

  // TODO: Really want to make value const here and enforce const correctness
  // where used (only reason I can't is Packet.pack() changes packet by design)
  virtual void pack(T& value, Buffer& buffer, int& offset) = 0;
  virtual void unpack(T& value,
                      const Buffer& buffer,
                      int& offset,
                      std::optional<int> limitOffset) = 0;
};

class IField {
 public:
  virtual ~IField() = default;

  virtual void pack(Buffer& buffer, int& offset) = 0;

  virtual void unpack(const Buffer& buffer,
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

  void unpack(const Buffer& buffer,
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
      uint8_t byte = (value >> i * 8) & 0xFF;
      if (offset < buffer.size())
        buffer[offset] = byte;
      else
        buffer.push_back(byte);
      offset++;
    }
  }

  void unpack(T& value,
              const Buffer& buffer,
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
  Vector(std::unique_ptr<Packer<T>> packer) : packer(std::move(packer)) {}
  Vector(std::unique_ptr<Packer<T>> packer, U& lengthRef)
      : packer(std::move(packer)) {
    _length.bind(lengthRef);
  }

  void pack(std::vector<T>& values, Buffer& buffer, int& offset) override {
    for (T& value : values)
      packer->pack(value, buffer, offset);

    _length.set(values.size());
  }

  void unpack(std::vector<T>& values,
              const Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override {
    int limit = getUnpackLimit(buffer.size(), limitOffset);
    values.clear();
    for (int i = 0; offset < limit && (!_length.isBound() || i < _length.get());
         i++) {
      values.push_back(T());
      packer->unpack(values.back(), buffer, offset, limitOffset);
    }
  }

 private:
  std::unique_ptr<Packer<T>> packer;
  SpecialProp<U> _length;
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
              const Buffer& buffer,
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
              const Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override;

 private:
  Primitive<uint16_t> packer;
};

class DelimitedString : public Packer<std::string> {
 public:
  DelimitedString(std::vector<std::string> start = {},
                  std::vector<std::string> end = {"\0"},
                  std::string trimChars = "",
                  bool packTrim = false,
                  bool keepEnd = false)
      : start(start),
        end(end),
        trimChars(trimChars),
        packTrim(packTrim),
        keepEnd(keepEnd) {}

  void pack(std::string& value, Buffer& buffer, int& offset) override;
  void unpack(std::string& value,
              const Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override;

 private:
  std::vector<std::string> start;
  std::vector<std::string> end;
  std::string trimChars;
  bool packTrim = false;
  bool keepEnd = false;
  Primitive<char> packer;
};

// TODO: Get rid of this since Packet can be its own field?
class NestedPacket : public Packer<Packet> {
 public:
  void pack(Packet& value, Buffer& buffer, int& offset) override;
  void unpack(Packet& value,
              const Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override;
};

template <typename T>
  requires std::derived_from<T, Packet>
class PacketBuffer : public Packer<Buffer> {
 public:
  PacketBuffer() : packer(std::make_unique<Primitive<uint8_t>>()) {}

  void pack(Buffer& value, Buffer& buffer, int& offset) override {
    packer.pack(value, buffer, offset);
  }

  void unpack(Buffer& value,
              const Buffer& buffer,
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
  Vector<uint8_t> packer;
};

// TODO: The overloading on field is kind of fun but hurts readability
class Packet : public IField {
 public:
  virtual ~Packet() = default;

  virtual void pack(Buffer& buffer, int& offset) override;
  virtual void unpack(const Buffer& buffer,
                      int& offset,
                      std::optional<int> limitOffset = std::nullopt) override;

  Buffer pack();
  void unpack(const Buffer& buffer);

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
  static std::unique_ptr<U> unpackAs(const Buffer& buffer) {
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

 protected:
  std::vector<std::unique_ptr<IField>> fields;

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

  // Wide string (encoded as 16 bits/char as in PTP)
  void field(std::string& value) {
    ADD_FIELD(std::string, WideString, , value);
  }

  // Delimited string
  void field(std::string& value,
             std::vector<std::string> start,
             std::vector<std::string> end,
             std::string trimChars = "",
             bool packTrim = false,
             bool keepEnd = false) {
    ADD_FIELD(std::string, DelimitedString,
              start COMMA() end COMMA() trimChars COMMA() packTrim COMMA()
                  keepEnd,
              value);
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

 private:
  SpecialProp<uint32_t> _length;
  SpecialProp<uint32_t> _type;
};

#endif