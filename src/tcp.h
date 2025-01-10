#ifndef CB_CONTROL_TCP_H
#define CB_CONTROL_TCP_H

#include "socket.h"

class TCPSocket : public virtual Socket {
 public:
  virtual bool connect(const std::string& ip,
                       int port) = 0;  // Should not throw exceptions
  virtual bool close() = 0;            // Should not throw exceptions
  virtual bool isConnected() = 0;      // Should not throw exceptions
};

class TCPPacket : public Packet, public Sendable<TCPSocket> {
 public:
  virtual void send(TCPSocket& socket) override;
  virtual void recv(TCPSocket& socket,
                    Buffer& buffer,
                    unsigned int timeoutMs = 10000) override;

 private:
  void recvAttempt(TCPSocket& socket,
                   Buffer& buffer,
                   unsigned int timeoutMs,
                   int targetBytes);
};

#endif