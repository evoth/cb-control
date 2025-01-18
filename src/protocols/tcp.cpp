#include <cb/protocols/tcp.h>

namespace cb {

int TCPPacket::send(TCPSocket& socket) {
  Buffer buffer = pack();
  return socket.sendAttempt(buffer);
}

int TCPPacket::recv(TCPSocket& socket, Buffer& buffer, unsigned int timeoutMs) {
  buffer.clear();

  socket.recvAttempt(buffer, timeoutMs, sizeof(uint32_t));
  unpack(buffer);

  socket.recvAttempt(buffer, timeoutMs, getLength() - sizeof(uint32_t));
  unpack(buffer);

  return buffer.size();
}

}