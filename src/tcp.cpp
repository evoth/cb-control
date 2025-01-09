#include "tcp.h"
#include "exception.h"

void TCPPacket::send(TCPSocket& socket) {
  Buffer buffer = pack();
  if (socket.send(buffer) < buffer.size()) {
    socket.close();
    throw Exception(ExceptionContext::TCPSocket, ExceptionType::SendFailure);
  }
}

void TCPPacket::recv(TCPSocket& socket,
                     Buffer& buffer,
                     unsigned int timeoutMs) {
  buffer.clear();

  int targetBytes = sizeof(uint32_t);
  if (socket.recv(buffer, timeoutMs, targetBytes) < targetBytes) {
    socket.close();
    throw Exception(ExceptionContext::TCPSocket, ExceptionType::TimedOut);
  }

  unpack(buffer);

  targetBytes = getLength() - targetBytes;
  if (socket.recv(buffer, timeoutMs, targetBytes) < targetBytes) {
    socket.close();
    throw Exception(ExceptionContext::TCPSocket, ExceptionType::TimedOut);
  }
}