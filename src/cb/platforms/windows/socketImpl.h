#ifndef CB_CONTROL_WINDOWS_SOCKET_H
#define CB_CONTROL_WINDOWS_SOCKET_H

#include <cb/exception.h>
#include <cb/protocols/tcp.h>
#include <cb/protocols/udp.h>

#include <winsock2.h>
#include <ws2tcpip.h>

namespace cb {

class WindowsSocket : public virtual BufferedSocket {
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

 protected:
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

  SOCKET clientSocket = INVALID_SOCKET;
  fd_set readfds;
};

// TODO: Figure out error logging
class TCPSocketImpl : public TCPSocket, WindowsSocket {
 public:
  bool connect(const std::string& ip, int port) override {
    close();  // TODO: is this needed?

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
    bool failure = closesocket(clientSocket) == SOCKET_ERROR;
    clientSocket = INVALID_SOCKET;
    return failure;
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
};

// TODO: Send/listen on all available interfaces ("multi-homed" control point)
class UDPMulticastSocketImpl : public UDPMulticastSocket, WindowsSocket {
 public:
  UDPMulticastSocketImpl() : BufferedSocket(1460) {}

  bool begin(const std::string& ip, int port) override {
    remoteIp = ip;
    remotePort = port;

    close();  // TODO: is this needed?

    clientSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (clientSocket == INVALID_SOCKET)
      return false;

    SOCKADDR_IN serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddress.sin_port = htons(remotePort);

    if (bind(clientSocket, (SOCKADDR*)&serverAddress, sizeof(serverAddress)) ==
        SOCKET_ERROR) {
      close();
      return false;
    }

    IP_MREQ imr;
    imr.imr_multiaddr.s_addr = inet_addr(remoteIp.c_str());
    imr.imr_interface.s_addr = htonl(INADDR_ANY);

    if (setsockopt(clientSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   (const char*)&imr, sizeof(imr)) == SOCKET_ERROR) {
      close();
      return false;
    }

    return true;
  }

  bool close() override {
    IP_MREQ imr;
    imr.imr_multiaddr.s_addr = inet_addr(remoteIp.c_str());
    imr.imr_interface.s_addr = htonl(INADDR_ANY);

    bool failure = setsockopt(clientSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP,
                              (const char*)&imr, sizeof(imr)) == SOCKET_ERROR;

    failure |= closesocket(clientSocket) == SOCKET_ERROR;
    clientSocket = INVALID_SOCKET;
    return failure;
  }

  std::string getRemoteIp() const override { return remoteIp; }

  int getRemotePort() const override { return remotePort; }

 protected:
  int send(const char* buff, int length) override {
    if (remoteIp.empty() || remotePort == 0)
      return BUFFERED_SOCKET_ERROR;

    SOCKADDR_IN recvAddress;
    recvAddress.sin_family = AF_INET;
    recvAddress.sin_addr.s_addr = inet_addr(remoteIp.c_str());
    recvAddress.sin_port = htons(remotePort);

    int result = sendto(clientSocket, buff, length, 0, (SOCKADDR*)&recvAddress,
                        sizeof(recvAddress));

    if (result == SOCKET_ERROR)
      return BUFFERED_SOCKET_ERROR;
    return result;
  }

  int recv(char* buff, int length) override {
    SOCKADDR_IN sendAddress;
    int sendAddressSize = sizeof(sendAddress);

    int result = recvfrom(clientSocket, buff, length, 0,
                          (SOCKADDR*)&sendAddress, &sendAddressSize);

    char ipString[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &sendAddress.sin_addr, ipString, sizeof(ipString));
    remoteIp = ipString;
    remotePort = ntohs(sendAddress.sin_port);

    if (result == SOCKET_ERROR)
      return BUFFERED_SOCKET_ERROR;
    return result;
  }

 private:
  std::string remoteIp;
  int remotePort = 0;
};

}  // namespace cb

#endif