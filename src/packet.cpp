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
  std::vector<int> lengthIndices;

  for (std::unique_ptr<IField>& field : fields) {
    if (field->isLengthField)
      lengthIndices.push_back(offset);
    offset += field->packField(buffer, offset);
  }

  std::vector<int>::iterator it = lengthIndices.begin();

  for (std::unique_ptr<IField>& field : fields) {
    if (field->isLengthField) {
      field->setLength(offset - startOffset);
      field->packField(buffer, *it++);
    }
  }

  return offset - startOffset;
}

int Packet::unpackField(Buffer& buffer, int offset) {
  int startOffset = offset;
  for (std::unique_ptr<IField>& field : fields) {
    offset += field->unpackField(buffer, offset);
  }
  return offset - startOffset;
}