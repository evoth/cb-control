#include <cb/packet.h>

#include <iterator>

namespace cb {

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
                        const Buffer& buffer,
                        int& offset,
                        std::optional<int> limitOffset) {
  int limit = getUnpackLimit(buffer.size(), limitOffset);
  uint16_t wideChar;
  value.clear();
  while (offset < limit) {
    packer.unpack(wideChar, buffer, offset, limitOffset);
    if (wideChar == '\0')
      break;
    value.push_back(wideChar);
  }
}

void DelimitedString::pack(std::string& value, Buffer& buffer, int& offset) {
  if (value.empty() && !packEmpty)
    return;
  if (!start.empty())
    for (char& c : start[0])
      packer.pack(c, buffer, offset);
  for (char& c : value)
    packer.pack(c, buffer, offset);
  if (!end.empty() && seekEnd)
    for (char& c : end[0])
      packer.pack(c, buffer, offset);
  if (!trimChars.empty() && packTrim)
    packer.pack(trimChars[0], buffer, offset);
}

void DelimitedString::unpack(std::string& value,
                             const Buffer& buffer,
                             int& offset,
                             std::optional<int> limitOffset) {
  int limit = getUnpackLimit(buffer.size(), limitOffset);
  char c;
  value.clear();

  // TODO: Use unpack here instead of raw access
  while (offset < limit && trimChars.find(buffer[offset]) != std::string::npos)
    offset++;

  if (!start.empty()) {
    int startOffset = offset;
    std::string startTest;
    bool foundStart = false;
    bool testedAll = false;
    // Keep adding characters until we have exhausted all start options
    while (offset < limit && !testedAll) {
      testedAll = true;
      for (std::string& s : start) {
        if (startTest.length() < s.length())
          testedAll = false;
        if (startTest == s) {
          foundStart = true;
          break;
        }
      }
      if (foundStart)
        break;

      packer.unpack(c, buffer, offset, limitOffset);
      // We also trim characters within start string
      if (trimChars.find(c) == std::string::npos)
        startTest.push_back(c);
    }
    if (!foundStart) {
      offset = startOffset;
      return;
    }
  }

  while (offset < limit) {
    packer.unpack(c, buffer, offset, limitOffset);
    value.push_back(c);

    for (std::string& e : end) {
      auto valueIt = value.rbegin();
      auto eIt = e.rbegin();

      // Check equivalence of endings sans trimmed characters
      while (valueIt != value.rend() && eIt != e.rend()) {
        if (*valueIt != *eIt && trimChars.find(*valueIt) != std::string::npos) {
          valueIt++;
          continue;
        }
        if (*valueIt != *eIt)
          break;
        valueIt++;
        eIt++;
      }

      if (eIt == e.rend()) {
        while (valueIt != value.rend() &&
               trimChars.find(*valueIt) != std::string::npos)
          valueIt++;
        int endDist = std::distance(value.rbegin(), valueIt);

        if (!keepEnd)
          value.erase(value.size() - endDist);

        if (seekEnd) {
          // TODO: Use unpack here instead of raw access
          while (offset < limit &&
                 trimChars.find(buffer[offset]) != std::string::npos)
            offset++;
        } else {
          offset -= endDist;
        }

        return;
      }
    }
  }
}

void NestedPacket::pack(Packet& value, Buffer& buffer, int& offset) {
  value.pack(buffer, offset);
}

void NestedPacket::unpack(Packet& value,
                          const Buffer& buffer,
                          int& offset,
                          std::optional<int> limitOffset) {
  value.unpack(buffer, offset, limitOffset);
}

void Packet::pack(Buffer& buffer, int& offset) {
  int startOffset = offset;
  for (std::unique_ptr<IField>& field : fields)
    field->pack(buffer, offset);

  if (_length.isBound()) {
    _length.set(offset - startOffset);

    offset = startOffset;
    for (std::unique_ptr<IField>& field : fields)
      field->pack(buffer, offset);
  }
};

void Packet::unpack(const Buffer& buffer,
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

void Packet::unpack(const Buffer& buffer) {
  int offset = 0;
  unpack(buffer, offset);
};

}  // namespace cb