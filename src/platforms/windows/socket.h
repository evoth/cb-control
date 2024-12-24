#ifndef CB_CONTROL_WINDOWS_SOCKET_H
#define CB_CONTROL_WINDOWS_SOCKET_H

#include "../../socket.h"

#include <winsock2.h>
#include <ws2tcpip.h>

// TODO: Figure out error logging
class WindowsSocket : public BufferedSocket {
 public:
  WindowsSocket() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
      throw Exception(ExceptionContext::Socket, ExceptionType::InitFailure);
  }

  ~WindowsSocket() {
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
    int result = closesocket(clientSocket);
    clientSocket = INVALID_SOCKET;
    return result == 0;
  }

  bool isConnected() override { return clientSocket != INVALID_SOCKET; }

 protected:
  int send(const char* buff, int length) override {
    int result = ::send(clientSocket, buff, length, 0);
    if (result == SOCKET_ERROR)
      return BUFFERED_SOCKET_ERROR;
    return result;
  }

  int recv(char* buff, int length) override {
    int result = ::recv(clientSocket, buff, length, 0);
    if (result == SOCKET_ERROR)
      return BUFFERED_SOCKET_ERROR;
    return result;
  }

  bool wait(unsigned int timeoutMs) override {
    timeval timeout;
    timeout.tv_sec = timeoutMs / 1000;
    timeout.tv_usec = (timeoutMs % 1000) * 1000;

    FD_ZERO(&readfds);
    FD_SET(clientSocket, &readfds);

    // TODO: More granular error handling
    int status = select(0, &readfds, NULL, NULL, &timeout);
    if (status == SOCKET_ERROR || status == 0 ||
        !FD_ISSET(clientSocket, &readfds))
      return false;

    return true;
  }

 private:
  SOCKET clientSocket = INVALID_SOCKET;
  fd_set readfds;
};

#endif