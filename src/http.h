#ifndef CB_CONTROL_HTTP_H
#define CB_CONTROL_HTTP_H

#include "tcp.h"

#include <map>

class HTTPMessage : public Packet, public Sendable<TCPSocket> {
 public:
  std::string httpVersion = "HTTP/1.1";
  std::map<std::string, std::string> headers;
  Buffer body;

  HTTPMessage()
      : headerNamePacker(": "),
        headerValuePacker("\r\n"),
        bodyField(std::make_unique<Vector<uint8_t, uint32_t>>(
                      std::make_unique<Primitive<uint8_t>>(),
                      contentLength),
                  body) {}

  using Packet::pack;
  using Packet::unpack;

  virtual void pack(Buffer& buffer, int& offset) override;
  virtual void unpack(const Buffer& buffer,
                      int& offset,
                      std::optional<int> limitOffset) override;

  void send(TCPSocket& socket) override;
  void recv(TCPSocket& socket,
            Buffer& buffer,
            unsigned int timeoutMs = 10000) override;
  void recv(TCPSocket& socket, unsigned int timeoutMs = 10000) {
    Buffer buffer;
    recv(socket, buffer, timeoutMs);
  }

 private:
  uint32_t contentLength = 0;
  uint32_t chunkSize = 8192;

  DelimitedString headerNamePacker;
  DelimitedString headerValuePacker;
  Field<Buffer> bodyField;

  void recvAttempt(TCPSocket& socket,
                   Buffer& buffer,
                   unsigned int timeoutMs,
                   int targetBytes);
};

class HTTPRequest : public HTTPMessage {
 public:
  std::string method;
  std::string target;

  HTTPRequest(std::string method = "", std::string target = "")
      : method(method), target(target) {
    field(this->method, " ");
    field(this->target, " ");
    field(this->httpVersion, "\r\n");
  }
};

class HTTPResponse : public HTTPMessage {
 public:
  std::string statusCode;
  std::string statusText;

  HTTPResponse(std::string statusCode = "", std::string statusText = "")
      : statusCode(statusCode), statusText(statusText) {
    field(this->httpVersion, " ");
    field(this->statusCode, " ");
    field(this->statusText, "\r\n");
  }
};

#endif