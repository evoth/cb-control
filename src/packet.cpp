#include "packet.h"

void WideString::pack(Buffer& buffer, int& offset) {
  uint16_t wideChar;
  Primitive packer(wideChar);
  for (char& c : value) {
    wideChar = c;
    packer.pack(buffer, offset);
  }
  wideChar = 0;
  packer.pack(buffer, offset);
}

void WideString::unpack(Buffer& buffer,
                        int& offset,
                        std::optional<int> limitOffset) {
  int limit = Field::getUnpackLimit(buffer, limitOffset);
  uint16_t wideChar;
  Primitive unpacker(wideChar);
  value.clear();
  while (offset < limit) {
    unpacker.unpack(buffer, offset, limitOffset);
    if (!wideChar)
      break;
    value.push_back(wideChar);
  }
}

void NestedPacket::pack(Buffer& buffer, int& offset) {
  value.pack(buffer, offset);
}

void NestedPacket::unpack(Buffer& buffer,
                          int& offset,
                          std::optional<int> limitOffset) {
  value.unpack(buffer, offset, limitOffset);
}

void Packet::pack(Buffer& buffer, int& offset) {
  int startOffset = offset;
  for (std::unique_ptr<Field>& field : fields)
    field->pack(buffer, offset);

  lengthRef = offset - startOffset;

  offset = startOffset;
  for (std::unique_ptr<Field>& field : fields)
    field->pack(buffer, offset);
};

void Packet::unpack(Buffer& buffer,
                    int& offset,
                    std::optional<int> limitOffset) {
  int startOffset = offset;
  lengthRef = 0;
  for (std::unique_ptr<Field>& field : fields) {
    if (lengthRef > 0 && (!limitOffset.has_value() ||
                          startOffset + lengthRef < limitOffset.value()))
      limitOffset = startOffset + lengthRef;
    field->unpack(buffer, offset, limitOffset);
  }
};

Buffer Packet::pack() {
  Buffer buffer;
  int offset = 0;
  pack(buffer, offset);
  return buffer;
};

void Packet::unpack(Buffer& buffer) {
  int offset = 0;
  unpack(buffer, offset);
};