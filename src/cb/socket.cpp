#include <cb/exception.h>
#include <cb/socket.h>

#include <chrono>

namespace cb {

int Socket::sendAttempt(Buffer& buffer) {
  int result = send(buffer);
  if (result < buffer.size()) {
    close();
    throw Exception(ExceptionContext::Socket, ExceptionType::SendFailure);
  }
  return result;
}

int Socket::recvAttempt(Buffer& buffer,
                        unsigned int timeoutMs,
                        int targetBytes) {
  int result = recv(buffer, timeoutMs, targetBytes);
  if (result < targetBytes) {
    close();
    throw Exception(ExceptionContext::Socket, ExceptionType::TimedOut);
  }
  return result;
}

int BufferedSocket::send(const Buffer& buffer) {
  int totalSent = 0;
  while (totalSent < buffer.size()) {
    int buffBytes = buffer.size() - totalSent;
    if (buffBytes > buffLen)
      buffBytes = buffLen;

    std::vector<uint8_t>::const_iterator begin = buffer.begin() + totalSent;
    std::copy(begin, begin + buffBytes, buff);

    int sentBytes = 0;
    while (sentBytes < buffBytes) {
      int result = send(buff + sentBytes, buffBytes - sentBytes);

      // TODO: Close socket (or mark as closed) if appropriate
      if (result == BUFFERED_SOCKET_ERROR)
        return totalSent + sentBytes;

      sentBytes += result;
    }

    totalSent += buffBytes;
  }

  return totalSent;
}

int BufferedSocket::recv(Buffer& buffer,
                         unsigned int timeoutMs,
                         std::optional<int> length) {
  // This might be premature optimization, but it should reduce heap
  // fragmentation for large packets
  if (length.has_value())
    buffer.reserve(buffer.size() + length.value());

  auto now = std::chrono::steady_clock::now();
  auto endTime = now + std::chrono::milliseconds(timeoutMs);

  // We read blocks of up to SOCKET_BUFF_SIZE into the buffer until we reach
  // `length` bytes or timeout. Using a do/while loop so that we always try to
  // recv() at least once, regardless of timeout/length.
  int totalReceived = 0;
  do {
    auto timeoutDelta = endTime - now;
    long long timeoutDeltaMs =
        std::chrono::duration_cast<std::chrono::milliseconds>(timeoutDelta)
            .count();
    if (timeoutDeltaMs < 0)
      timeoutDeltaMs = 0;

    if (!wait(timeoutDeltaMs))
      return totalReceived;

    int buffBytes = buffLen;
    if (length.has_value())
      buffBytes = length.value() - totalReceived;
    if (buffBytes > buffLen)
      buffBytes = buffLen;

    int result = recv(buff, buffBytes);

    // TODO: Close socket (or mark as closed) if appropriate
    if (result == BUFFERED_SOCKET_ERROR)
      return totalReceived;

    buffer.insert(buffer.end(), buff, buff + result);

    totalReceived += result;
    now = std::chrono::steady_clock::now();
  } while (now < endTime &&
           (!length.has_value() || totalReceived < length.value()));

  return totalReceived;
}

}