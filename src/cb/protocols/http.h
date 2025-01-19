#ifndef CB_CONTROL_HTTP_H
#define CB_CONTROL_HTTP_H

#include <cb/protocols/tcp.h>
#include <cb/protocols/udp.h>

#include <map>

namespace cb {

class HTTPMessage : public Packet,
                    public Sendable<TCPSocket>,
                    public Sendable<UDPMulticastSocket> {
 public:
  std::string httpVersion = "HTTP/1.1";
  std::map<std::string, std::string> headers;
  Buffer body;

  HTTPMessage()
      : headerNamePacker({}, {":", "\r\n"}, " \t", true),
        headerValuePacker({}, {"\r\n"}, " \t"),
        bodyField(std::make_unique<Vector<uint8_t, uint32_t>>(
                      std::make_unique<Primitive<uint8_t>>(),
                      contentLength),
                  body) {}

  using Packet::pack;
  using Packet::unpack;

  void pack(Buffer& buffer, int& offset) override;
  void unpack(const Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override;

  int send(TCPSocket& socket) override;
  int recv(TCPSocket& socket,
           Buffer& buffer,
           unsigned int timeoutMs = 10000) override;
  int recv(TCPSocket& socket, unsigned int timeoutMs = 10000) {
    Buffer buffer;
    return recv(socket, buffer, timeoutMs);
  }

  int send(UDPMulticastSocket& socket) override;
  int recv(UDPMulticastSocket& socket,
           Buffer& buffer,
           unsigned int timeoutMs = 10000) override;
  int recv(UDPMulticastSocket& socket, unsigned int timeoutMs = 10000) {
    Buffer buffer;
    return recv(socket, buffer, timeoutMs);
  }

 private:
  uint32_t contentLength = 0;
  uint32_t chunkSize = 8192;

  DelimitedString headerNamePacker;
  DelimitedString headerValuePacker;
  Field<Buffer> bodyField;
};

class HTTPRequest : public HTTPMessage {
 public:
  std::string method;
  std::string target;

  HTTPRequest(std::string method, std::string target)
      : method(method), target(target) {
    field(this->method, {}, {" "});
    field(this->target, {}, {" "});
    field(this->httpVersion, {}, {"\r\n"});
  }
  HTTPRequest() : HTTPRequest("", "") {}
};

class HTTPResponse : public HTTPMessage {
 public:
  std::string statusCode;
  std::string statusText;

  HTTPResponse(std::string statusCode, std::string statusText)
      : statusCode(statusCode), statusText(statusText) {
    field(this->httpVersion, {}, {" "});
    field(this->statusCode, {}, {" "});
    field(this->statusText, {}, {"\r\n"});
  }
  HTTPResponse() : HTTPResponse("", "") {}
};

// This is a very fragile way to parse URL strings, but good enough for now
class URL : public Packet {
 public:
  std::string protocol;
  std::string hostname;
  std::string port;
  std::string path;
  std::string query;
  std::string fragment;

  URL() {
    field(this->protocol, {}, {"://"});
    field(this->hostname, {}, {":", "/"}, "", false, true, false, false);
    field(this->port, {":"}, {"/"}, "", false, false, false, false);
    field(this->path, {}, {"?", "#"}, "", false, false, false, false);
    field(this->query, {"?"}, {"#"}, "", false, false, false, false);
    field(this->fragment, {"#"}, {}, "", false, false, false, false);
  }

  URL(std::string s) : URL() { unpackString(s); }

  std::unique_ptr<HTTPResponse> request(std::unique_ptr<TCPSocket>& socket);
};

}  // namespace cb

#endif