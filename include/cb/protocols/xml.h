#ifndef CB_CONTROL_XML_H
#define CB_CONTROL_XML_H

#include <cb/packet.h>

#include <map>

#define XML_WHITESPACE " \r\n\t"

class XMLNode : public Packet {
 public:
  virtual void pack(Buffer& buffer, int& offset) override = 0;
  virtual void unpack(const Buffer& buffer,
                      int& offset,
                      std::optional<int> limitOffset) override = 0;
};

class XMLText : public XMLNode {
 public:
  std::string text;

  void pack(Buffer& buffer, int& offset) override;
  void unpack(const Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override;

 private:
  Primitive<char> packer;
};

class XMLElement : public XMLNode {
 public:
  std::string name;
  std::map<std::string, std::string> attributes;
  std::vector<std::unique_ptr<XMLNode>> children;

  XMLElement()
      : namePacker({"<"},
                   {" ", "\r", "\n", "\t", ">", "/>"},
                   XML_WHITESPACE,
                   false,
                   true),
        attributeNamePacker({}, {"="}, XML_WHITESPACE),
        attributeValuePacker({"\"", "'"}, {"\"", "'"}, XML_WHITESPACE, true),
        closePacker({}, {">"}, XML_WHITESPACE),
        endPacker({"</"}, {">"}, XML_WHITESPACE) {}

  virtual void pack(Buffer& buffer, int& offset) override;
  virtual void unpack(const Buffer& buffer,
                      int& offset,
                      std::optional<int> limitOffset) override;

  operator std::string() const;

  bool containsTag(std::string name) const;
  XMLElement& getTagByName(std::string name) const;
  XMLElement& operator[](std::string name) const { return getTagByName(name); }

 private:
  Primitive<char> charPacker;
  DelimitedString namePacker;
  DelimitedString attributeNamePacker;
  DelimitedString attributeValuePacker;
  DelimitedString closePacker;
  DelimitedString endPacker;
};

class XMLDocument : public XMLElement {
 public:
  std::string declaration;

  using Packet::pack;
  using Packet::unpack;

  XMLDocument() { field(this->declaration, {"<?xml"}, {"?>"}, XML_WHITESPACE); }

  void pack(Buffer& buffer, int& offset) override;
  void unpack(const Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override;
};

#endif