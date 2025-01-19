#ifndef CB_CONTROL_SOCKET_H
#define CB_CONTROL_SOCKET_H

#include <cb/packet.h>

namespace cb {

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

  virtual bool close() = 0;  // Should not throw exceptions

  // TODO: Make these the main send/recv functions? But those are virtual?
  int sendAttempt(Buffer& buffer);
  int recvAttempt(Buffer& buffer, unsigned int timeoutMs, int targetBytes);
};

#define BUFFERED_SOCKET_ERROR -1

class BufferedSocket : public virtual Socket {
 public:
  BufferedSocket(int buffLen = 1024)
      : buffLen(buffLen), buff(new char[buffLen]) {}
  virtual ~BufferedSocket() { delete[] buff; }

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
  int buffLen = 0;
  char* buff;
};

template <typename T>
  requires std::derived_from<T, Socket>
class Sendable {
 public:
  virtual int send(T& socket) = 0;
  // Should put received packet in buffer and unpack
  virtual int recv(T& socket, Buffer& buffer, unsigned int timeoutMs) = 0;
};

}  // namespace cb

#endif