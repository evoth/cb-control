#include "packet.h"

int getUnpackLimit(int hardLimit, std::optional<int> limitOffset) {
  if (limitOffset.has_value() && limitOffset.value() < hardLimit)
    hardLimit = limitOffset.value();
  return hardLimit;
}

void WideString::pack(std::string& value, Buffer& buffer, int& offset) {
  uint16_t wideChar;
  for (char& c : value) {
    wideChar = c;
    packer.pack(wideChar, buffer, offset);
  }
  wideChar = 0;
  packer.pack(wideChar, buffer, offset);
}

void WideString::unpack(std::string& value,
                        Buffer& buffer,
                        int& offset,
                        std::optional<int> limitOffset) {
  int limit = getUnpackLimit(buffer.size(), limitOffset);
  uint16_t wideChar;
  value.clear();
  while (offset < limit) {
    packer.unpack(wideChar, buffer, offset, limitOffset);
    if (!wideChar)
      break;
    value.push_back(wideChar);
  }
}

void NestedPacket::pack(Packet& value, Buffer& buffer, int& offset) {
  value.pack(buffer, offset);
}

void NestedPacket::unpack(Packet& value,
                          Buffer& buffer,
                          int& offset,
                          std::optional<int> limitOffset) {
  value.unpack(buffer, offset, limitOffset);
}

void Packet::pack(Buffer& buffer, int& offset) {
  int startOffset = offset;
  for (std::unique_ptr<IField>& field : fields)
    field->pack(buffer, offset);

  _length.set(offset - startOffset);

  offset = startOffset;
  for (std::unique_ptr<IField>& field : fields)
    field->pack(buffer, offset);
};

void Packet::unpack(Buffer& buffer,
                    int& offset,
                    std::optional<int> limitOffset) {
  int startOffset = offset;
  _length.set(0);
  for (std::unique_ptr<IField>& field : fields) {
    if (_length.get() > 0)
      limitOffset = getUnpackLimit(startOffset + _length.get(), limitOffset);
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