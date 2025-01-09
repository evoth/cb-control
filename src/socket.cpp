#include "socket.h"

#include <chrono>

int BufferedSocket::send(const Buffer& buffer) {
  // Send in blocks of SOCKET_BUFF_SIZE bytes
  int totalSent = 0;
  while (totalSent < buffer.size()) {
    // Read block from `buffer` (byte vector) into `buff` (char array)
    int buffBytes = 0;
    while (buffBytes < SOCKET_BUFF_SIZE &&
           totalSent + buffBytes < buffer.size()) {
      buff[buffBytes] = buffer[totalSent + buffBytes];
      buffBytes++;
    }

    // Repeatedly send() until current block has been fully sent
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

    int buffBytes = SOCKET_BUFF_SIZE;
    if (length.has_value())
      buffBytes = length.value() - totalReceived;
    if (buffBytes > SOCKET_BUFF_SIZE)
      buffBytes = SOCKET_BUFF_SIZE;

    int result = recv(buff, buffBytes);

    // TODO: Close socket (or mark as closed) if appropriate
    if (result == BUFFERED_SOCKET_ERROR)
      return totalReceived;

    for (int i = 0; i < result; i++) {
      buffer.push_back(buff[i]);
    }

    totalReceived += result;
    now = std::chrono::steady_clock::now();
    // This (combined with <= in the while conditional)
    // is evil but does exactly what I want
    if (!length.has_value())
      endTime = now;
  } while (now <= endTime && totalReceived < length.value());

  return totalReceived;
}