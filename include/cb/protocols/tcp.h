#ifndef CB_CONTROL_TCP_H
#define CB_CONTROL_TCP_H

#include <cb/socket.h>

// TODO: Interface for setting options like KEEPALIVE and NODELAY
class TCPSocket : public virtual Socket {
 public:
  virtual bool connect(const std::string& ip,
                       int port) = 0;  // Should not throw exceptions
  virtual bool isConnected() = 0;      // Should not throw exceptions
};

class TCPPacket : public Packet, public Sendable<TCPSocket> {
 public:
  virtual int send(TCPSocket& socket) override;
  virtual int recv(TCPSocket& socket,
                   Buffer& buffer,
                   unsigned int timeoutMs = 10000) override;
};

#endif