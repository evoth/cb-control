#include "packet.h"

Buffer Packet::pack() {
  Buffer buffer;
  int offset = 0;
  packField(buffer, offset);
  return buffer;
}

void Packet::unpack(Buffer& buffer) {
  int offset = 0;
  unpackField(buffer, offset);
}

void Packet::packField(Buffer& buffer, int& offset) {
  int startOffset = offset;
  for (std::unique_ptr<IField>& field : fields)
    field->packField(buffer, offset);

  lengthRef = offset - startOffset;

  offset = startOffset;
  for (std::unique_ptr<IField>& field : fields)
    field->packField(buffer, offset);
}

void Packet::unpackField(Buffer& buffer, int& offset) {
  for (std::unique_ptr<IField>& field : fields)
    field->unpackField(buffer, offset);
}