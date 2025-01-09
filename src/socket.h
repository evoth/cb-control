#ifndef CB_CONTROL_SOCKET_H
#define CB_CONTROL_SOCKET_H

#include "packet.h"

// TODO: Use noexcept?
class Socket {
 public:
  virtual ~Socket() = default;

  virtual int send(const Buffer& buffer) = 0;  // Should not throw exceptions
  // Attempts to append `length` bytes to the buffer, waiting until enough bytes
  // have accumulated or until `timeoutMs` milliseconds have passed.
  // If `length` is left blank, this waits until socket is available or timeout
  // is exceeded, then reads until socket is exhausted.
  virtual int recv(Buffer& buffer,
                   unsigned int timeoutMs = 0,
                   std::optional<int> length =
                       std::nullopt) = 0;  // Should not throw exceptions
};

template <typename T>
  requires std::derived_from<T, Socket>
class Sendable {
 public:
  virtual void send(T& socket) = 0;
  virtual void recv(T& socket, Buffer& buffer, unsigned int timeoutMs) = 0;
};

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
};

#define SOCKET_BUFF_SIZE 512
#define BUFFERED_SOCKET_ERROR -1

class BufferedSocket : public virtual Socket {
 public:
  virtual ~BufferedSocket() = default;

 protected:
  int send(const Buffer& buffer) override;
  int recv(Buffer& buffer,
           unsigned int timeoutMs,
           std::optional<int> length) override;

  virtual int send(const char* buff, int length) = 0;
  virtual int recv(char* buff, int length) = 0;
  // Wait until socket available for reading, returning false on failure or
  // timeout. For a timeout of 0, return whether socket is immediately
  // available for reading.
  virtual bool wait(unsigned int timeoutMs) = 0;

 private:
  char buff[SOCKET_BUFF_SIZE];
};

#endif