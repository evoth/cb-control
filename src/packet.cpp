#include "packet.h"

Buffer Packet::pack() {
  Buffer buffer;
  std::vector<int> lengthIndices;
  for (std::unique_ptr<IField>& field : fields) {
    if (field->isLengthField)
      lengthIndices.push_back(buffer.size());
    field->pack(buffer, buffer.size());
  }
  std::vector<int>::iterator it = lengthIndices.begin();
  for (std::unique_ptr<IField>& field : fields) {
    if (field->isLengthField) {
      field->setLength(buffer.size());
      field->pack(buffer, *it++);
    }
  }
  return buffer;
}

void Packet::unpack(Buffer& buffer) {
  int offset = 0;
  for (std::unique_ptr<IField>& field : fields) {
    offset += field->unpack(buffer, offset);
  }
}