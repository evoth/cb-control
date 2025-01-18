#include <cb/exception.h>
#include <cb/protocols/http.h>

#include <algorithm>

// TODO: Implement chunked encoding and automatically use it when header present
void HTTPMessage::pack(Buffer& buffer, int& offset) {
  Packet::pack(buffer, offset);

  contentLength = body.size();
  if (contentLength > 0)
    headers["Content-Length"] = std::to_string(contentLength);

  std::string temp;
  for (auto& [name, value] : headers) {
    temp = name;  // TODO: Remove this when I fix const correctness
    headerNamePacker.pack(temp, buffer, offset);
    headerValuePacker.pack(value, buffer, offset);
  }
  temp = "";
  headerValuePacker.pack(temp, buffer, offset);

  if (!body.empty())
    bodyField.pack(buffer, offset);
}

void HTTPMessage::unpack(const Buffer& buffer,
                         int& offset,
                         std::optional<int> limitOffset) {
  Packet::unpack(buffer, offset, limitOffset);

  int limit = getUnpackLimit(buffer.size(), limitOffset);
  headers.clear();
  while (offset < limit) {
    std::string name, value;
    headerNamePacker.unpack(name, buffer, offset, limitOffset);
    if (name.empty())
      break;
    headerValuePacker.unpack(value, buffer, offset, limitOffset);
    headers[name] = value;
  }

  contentLength = 0;
  body.clear();
  if (headers.contains("Content-Length")) {
    contentLength = std::stoul(headers["Content-Length"]);
    bodyField.unpack(buffer, offset, limitOffset);
  }
}

int HTTPMessage::send(TCPSocket& socket) {
  Buffer buffer = pack();
  return socket.sendAttempt(buffer);
}

int HTTPMessage::recv(TCPSocket& socket,
                      Buffer& buffer,
                      unsigned int timeoutMs) {
  buffer.clear();

  const Buffer headerDelim = {'\r', '\n', '\r', '\n'};

  socket.recvAttempt(buffer, timeoutMs, headerDelim.size());
  if (buffer.size() < headerDelim.size()) {
    buffer.clear();
    return 0;
  }

  // Receive one byte at a time until end of header block
  while (!std::equal(buffer.end() - headerDelim.size(), buffer.end(),
                     headerDelim.begin())) {
    socket.recvAttempt(buffer, timeoutMs, 1);
  }
  unpack(buffer);

  if (contentLength > 0) {
    socket.recvAttempt(buffer, timeoutMs, contentLength);
    unpack(buffer);
  }

  return buffer.size();
}

int HTTPMessage::send(UDPMulticastSocket& socket) {
  Buffer buffer = pack();
  return socket.sendAttempt(buffer);
}

int HTTPMessage::recv(UDPMulticastSocket& socket,
                      Buffer& buffer,
                      unsigned int timeoutMs) {
  buffer.clear();
  socket.recv(buffer, timeoutMs);
  unpack(buffer);
  return buffer.size();
}