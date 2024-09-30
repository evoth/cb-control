#include "packet.h"

Buffer Packet::pack() {
  Buffer buffer;
  packField(buffer, 0);
  return buffer;
}

int Packet::unpack(Buffer& buffer) {
  return unpackField(buffer, 0);
}

int Packet::packField(Buffer& buffer, int offset) {
  int startOffset = offset;
  for (std::unique_ptr<IField>& field : fields)
    offset += field->packField(buffer, offset);

  lengthRef = offset - startOffset;

  offset = startOffset;
  for (std::unique_ptr<IField>& field : fields)
    offset += field->packField(buffer, offset);

  return offset - startOffset;
}

int Packet::unpackField(Buffer& buffer, int offset) {
  int startOffset = offset;
  for (std::unique_ptr<IField>& field : fields)
    offset += field->unpackField(buffer, offset);

  return offset - startOffset;
}