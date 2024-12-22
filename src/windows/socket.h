#ifndef CB_CONTROL_WINDOWS_SOCKET_H
#define CB_CONTROL_WINDOWS_SOCKET_H

#include "../ptp/ip.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>
#include "../logger.h"

#define SOCKET_BUFF_SIZE 512

// TODO: Figure out error logging
class WindowsSocket : public Socket {
 public:
  WindowsSocket() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
      throw PTPTransportException("WSAStartup failed in socket constructor.");
  }

  ~WindowsSocket() {
    Logger::log("Socket destructed");
    closesocket(clientSocket);
    WSACleanup();
  }

  bool connect(const std::string& ip, int port) override {
    closesocket(clientSocket);  // TODO: is this needed?

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
      return false;

    BOOL bOptVal = TRUE;
    int bOptLen = sizeof(BOOL);

    if (setsockopt(clientSocket, SOL_SOCKET, SO_KEEPALIVE,
                   (const char*)&bOptVal, sizeof(bOptLen)) == SOCKET_ERROR) {
      close();
      return false;
    }

    if (setsockopt(clientSocket, IPPROTO_TCP, TCP_NODELAY,
                   (const char*)&bOptVal, sizeof(bOptLen)) == SOCKET_ERROR) {
      close();
      return false;
    }

    SOCKADDR_IN serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = inet_addr(ip.c_str());
    serverAddress.sin_port = htons(port);

    if (::connect(clientSocket, (SOCKADDR*)&serverAddress,
                  sizeof(serverAddress)) == SOCKET_ERROR) {
      close();
      return false;
    }

    return true;
  }

  bool close() override {
    Logger::log("Socket closed");
    int result = closesocket(clientSocket);
    clientSocket = INVALID_SOCKET;
    return result == 0;
  }

  bool isConnected() override { return clientSocket != INVALID_SOCKET; }

  int send(const Buffer& buffer) override {
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
        int result =
            ::send(clientSocket, buff + sentBytes, buffBytes - sentBytes, 0);

        // TODO: Close socket (or mark as closed) if appropriate
        if (result == SOCKET_ERROR)
          return totalSent + sentBytes;

        sentBytes += result;
      }

      totalSent += buffBytes;
    }

    return totalSent;
  }

  int recv(Buffer& buffer, int length, unsigned int timeoutMs) override {
    auto now = std::chrono::steady_clock::now();
    auto endTime = now + std::chrono::milliseconds(timeoutMs);

    fd_set readfds;
    FD_ZERO(&readfds);

    // We read blocks of up to SOCKET_BUFF_SIZE into the buffer until we reach
    // `length` bytes or timeout. Using a do/while loop so that we always try to
    // recv() at least once, regardless of timeout/length.
    int totalReceived = 0;
    do {
      auto timeoutDelta = endTime - now;
      timeval timeout;
      timeout.tv_sec =
          std::chrono::duration_cast<std::chrono::seconds>(timeoutDelta)
              .count();
      timeout.tv_usec =
          std::chrono::duration_cast<std::chrono::microseconds>(timeoutDelta)
              .count() %
          1000000;

      FD_ZERO(&readfds);
      FD_SET(clientSocket, &readfds);

      // TODO: More granular error handling
      int status = select(0, &readfds, NULL, NULL, &timeout);
      if (status == SOCKET_ERROR || status == 0 ||
          !FD_ISSET(clientSocket, &readfds))
        return totalReceived;

      int buffBytes = length - totalReceived;
      if (buffBytes > SOCKET_BUFF_SIZE)
        buffBytes = SOCKET_BUFF_SIZE;

      int result = ::recv(clientSocket, buff, buffBytes, 0);

      // TODO: Close socket (or mark as closed) if appropriate
      if (result == SOCKET_ERROR)
        return totalReceived;

      for (int i = 0; i < result; i++) {
        buffer.push_back(buff[i]);
      }

      totalReceived += result;
      now = std::chrono::steady_clock::now();
    } while (totalReceived < length && now < endTime);

    return totalReceived;
  }

 private:
  SOCKET clientSocket = INVALID_SOCKET;
  char buff[SOCKET_BUFF_SIZE];
};

#undef SOCKET_BUFF_SIZE

#endif