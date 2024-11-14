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

void WideString::unpack(Buffer& buffer, int& offset) {
  uint16_t wideChar;
  Primitive unpacker(wideChar);
  value.clear();
  while (offset < buffer.size()) {
    unpacker.unpack(buffer, offset);
    if (!wideChar)
      break;
    value.push_back(wideChar);
  }
}

void NestedPacket::pack(Buffer& buffer, int& offset) {
  value.pack(buffer, offset);
}

void NestedPacket::unpack(Buffer& buffer, int& offset) {
  value.unpack(buffer, offset);
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

void Packet::unpack(Buffer& buffer, int& offset) {
  for (std::unique_ptr<Field>& field : fields)
    field->unpack(buffer, offset);
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