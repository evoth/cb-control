#include "http.h"
#include "exception.h"

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
    if (offset + 1 < limit && buffer[offset] == '\r' &&
        buffer[offset + 1] == '\n') {
      offset += 2;
      break;
    }

    std::string name, value;
    headerNamePacker.unpack(name, buffer, offset, limitOffset);
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

void HTTPMessage::send(TCPSocket& socket) {
  Buffer buffer = pack();
  if (socket.send(buffer) < buffer.size()) {
    socket.close();
    throw Exception(ExceptionContext::TCPSocket, ExceptionType::SendFailure);
  }
}

void HTTPMessage::recvAttempt(TCPSocket& socket,
                              Buffer& buffer,
                              unsigned int timeoutMs,
                              int targetBytes) {
  if (socket.recv(buffer, timeoutMs, targetBytes) < targetBytes) {
    socket.close();
    throw Exception(ExceptionContext::TCPSocket, ExceptionType::TimedOut);
  }
}

void HTTPMessage::recv(TCPSocket& socket,
                       Buffer& buffer,
                       unsigned int timeoutMs) {
  buffer.clear();

  const Buffer headerDelim = {'\r', '\n', '\r', '\n'};

  recvAttempt(socket, buffer, timeoutMs, headerDelim.size());
  if (buffer.size() < headerDelim.size())
    return;

  // Receive one byte at a time until end of header block
  while (!std::equal(buffer.end() - headerDelim.size(), buffer.end(),
                     headerDelim.begin())) {
    recvAttempt(socket, buffer, timeoutMs, 1);
  }
  unpack(buffer);

  if (contentLength > 0) {
    recvAttempt(socket, buffer, timeoutMs, contentLength);
    unpack(buffer);
  }
}