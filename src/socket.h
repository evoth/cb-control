#ifndef CB_CONTROL_SOCKET_H
#define CB_CONTROL_SOCKET_H

#include "exception.h"
#include "packet.h"

// TODO: Use noexcept?
class Socket {
 public:
  virtual ~Socket() = default;

 protected:
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

class TCPSocket : public virtual Socket {
 public:
  virtual bool connect(const std::string& ip,
                       int port) = 0;  // Should not throw exceptions
  virtual bool close() = 0;            // Should not throw exceptions
  virtual bool isConnected() = 0;      // Should not throw exceptions

  void sendPacket(Packet& packet) {
    Buffer buffer = packet.pack();
    if (send(buffer) < buffer.size()) {
      close();
      throw Exception(ExceptionContext::TCPSocket, ExceptionType::SendFailure);
    }
  }

  template <typename T>
    requires std::derived_from<T, Packet>
  void recvPacket(Buffer& buffer, unsigned int timeoutMs = 10000) {
    T lengthPacket;
    buffer.clear();

    int targetBytes = sizeof(lengthPacket.length);
    if (recv(buffer, timeoutMs, targetBytes) < targetBytes) {
      close();
      throw Exception(ExceptionContext::TCPSocket, ExceptionType::TimedOut);
    }

    lengthPacket.unpack(buffer);

    targetBytes = lengthPacket.length - targetBytes;
    if (recv(buffer, timeoutMs, targetBytes) < targetBytes) {
      close();
      throw Exception(ExceptionContext::TCPSocket, ExceptionType::TimedOut);
    }
  }
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