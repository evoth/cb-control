#include <cb/exception.h>
#include <cb/protocols/xml.h>

namespace cb {

void XMLText::pack(Buffer& buffer, int& offset) {
  for (char& c : text)
    packer.pack(c, buffer, offset);
}

void XMLText::unpack(const Buffer& buffer,
                     int& offset,
                     std::optional<int> limitOffset) {
  int limit = getUnpackLimit(buffer.size(), limitOffset);
  char c;
  text.clear();
  while (offset < limit && buffer[offset] != '<') {
    packer.unpack(c, buffer, offset, limitOffset);
    text.push_back(c);
  }
}

const XMLElement XMLElement::notFound;

void XMLElement::pack(Buffer& buffer, int& offset) {
  namePacker.pack(name, buffer, offset);

  for (auto& [name, value] : attributes) {
    std::string temp = name;  // TODO: Remove this when I fix const correctness
    attributeNamePacker.pack(temp, buffer, offset);
    attributeValuePacker.pack(value, buffer, offset);
  }

  std::string close = "";
  closePacker.pack(close, buffer, offset);

  for (auto& child : children) {
    child->pack(buffer, offset);
  }

  endPacker.pack(name, buffer, offset);
}

void XMLElement::unpack(const Buffer& buffer,
                        int& offset,
                        std::optional<int> limitOffset) {
  int limit = getUnpackLimit(buffer.size(), limitOffset);

  children.clear();

  namePacker.unpack(name, buffer, offset, limitOffset);
  std::string temp;

  if (name.size() >= 1 && name[name.size() - 1] == '>') {
    if (name.size() >= 2 && name[name.size() - 2] == '/') {
      name.pop_back();
      temp = "/";
    }
  } else {
    attributes.clear();
    while (offset < limit) {
      if (buffer[offset] == '/' || buffer[offset] == '>')
        break;
      std::string name, value;
      attributeNamePacker.unpack(name, buffer, offset, limitOffset);
      attributeValuePacker.unpack(value, buffer, offset, limitOffset);
      attributes[name] = value;
    }

    closePacker.unpack(temp, buffer, offset, limitOffset);
  }
  name.pop_back();

  if (temp == "/")
    return;  // Self-closing tag

  while (offset < limit && !(offset + 1 < limit && buffer[offset] == '<' &&
                             buffer[offset + 1] == '/')) {
    if (buffer[offset] == '<')
      children.push_back(std::make_unique<XMLElement>());
    else
      children.push_back(std::make_unique<XMLText>());

    children.back()->unpack(buffer, offset, limitOffset);
  }

  endPacker.unpack(temp, buffer, offset, limitOffset);
}

XMLElement::operator std::string() const {
  if (children.empty())
    return "";

  if (auto textNode = dynamic_cast<XMLText*>(children[0].get()))
    return textNode->text;

  return "";
}

bool XMLElement::containsTag(std::string name) const {
  for (auto& child : children) {
    if (auto element = dynamic_cast<XMLElement*>(child.get())) {
      if (element->name == name)
        return true;
    }
  }
  return false;
}

const XMLElement& XMLElement::getTagByName(std::string name) const {
  for (auto& child : children) {
    if (auto element = dynamic_cast<XMLElement*>(child.get())) {
      if (element->name == name)
        return *element;
    }
  }
  return notFound;
}

void XMLDoc::pack(Buffer& buffer, int& offset) {
  if (!declaration.empty())
    Packet::pack(buffer, offset);
  XMLElement::pack(buffer, offset);
}

void XMLDoc::unpack(const Buffer& buffer,
                    int& offset,
                    std::optional<int> limitOffset) {
  Packet::unpack(buffer, offset, limitOffset);
  XMLElement::unpack(buffer, offset, limitOffset);
}

}  // namespace cb