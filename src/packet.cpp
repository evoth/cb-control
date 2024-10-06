#include "packet.h"

Buffer Packet::pack() {
  Buffer buffer;
  int offset = 0;
  pack(buffer, offset);
  return buffer;
}

void Packet::unpack(Buffer& buffer) {
  int offset = 0;
  unpack(buffer, offset);
}

void Packet::pack(Buffer& buffer, int& offset) {
  int startOffset = offset;
  for (std::unique_ptr<IField>& field : fields)
    field->pack(buffer, offset);

  lengthRef = offset - startOffset;

  offset = startOffset;
  for (std::unique_ptr<IField>& field : fields)
    field->pack(buffer, offset);
}

void Packet::unpack(Buffer& buffer, int& offset) {
  for (std::unique_ptr<IField>& field : fields)
    field->unpack(buffer, offset);
}