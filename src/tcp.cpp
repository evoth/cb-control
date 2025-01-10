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

  recvAttempt(socket, buffer, timeoutMs, sizeof(uint32_t));
  unpack(buffer);

  recvAttempt(socket, buffer, timeoutMs, getLength() - sizeof(uint32_t));
  unpack(buffer);
}

void TCPPacket::recvAttempt(TCPSocket& socket,
                            Buffer& buffer,
                            unsigned int timeoutMs,
                            int targetBytes) {
  if (socket.recv(buffer, timeoutMs, targetBytes) < targetBytes) {
    socket.close();
    throw Exception(ExceptionContext::TCPSocket, ExceptionType::TimedOut);
  }
}