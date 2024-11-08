#include "ptp/canon.h"
#include "ptp/ip.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>
#include <iostream>

#define SOCKET_BUFF_SIZE 512

// TODO: Figure out error logging
class WindowsSocket : public Socket {
 public:
  WindowsSocket() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
      throw PTPTransportException("WSAStartup failed in socket constructor.");
  }

  virtual ~WindowsSocket() {
    std::cout << "Socket destructed" << std::endl;
    closesocket(clientSocket);
    WSACleanup();
  }

  bool connect(const std::string& ip, int port) override {
    close();

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
    std::cout << "Socket closed" << std::endl;
    int result = closesocket(clientSocket);
    clientSocket = INVALID_SOCKET;
    return result == 0;
  }

  bool isConnected() override { return clientSocket != INVALID_SOCKET; }

  int send(Buffer& buffer) override {
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
      if (result == SOCKET_ERROR) {
        std::cout << "OOPS: " << WSAGetLastError() << std::endl;
        return totalReceived;
      }

      for (int i = 0; i < result; i++) {
        buffer.push_back(buff[i]);
      }

      totalReceived += result;
      now = std::chrono::steady_clock::now();
    } while (totalReceived < length && now < endTime);

    return totalReceived;
  }

 private:
  SOCKET clientSocket;
  char buff[SOCKET_BUFF_SIZE];
};

int main() {
  const std::array<uint8_t, 16> guid(
      {7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7});

  std::cout << "Opening PTPIP transport..." << std::endl;
  std::unique_ptr<PTPIP> ptpip =
      std::make_unique<PTPIP>(guid, "Tim", std::make_unique<WindowsSocket>(),
                              std::make_unique<WindowsSocket>(), "192.168.4.7");
  std::cout << "PTPIP transport successfully opened!" << std::endl;

  std::cout << "Creating CanonPTP instance..." << std::endl;
  CanonPTP canon(std::move(ptpip));
  std::cout << "CanonPTP instance successfully created!" << std::endl;

  std::cout << "Opening PTP session..." << std::endl;
  canon.openSession();
  std::cout << "PTP session successfully opened!" << std::endl;

  std::cout << "Waiting for 5 seconds..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(5));

  std::cout << "Releasing shutter..." << std::endl;
  canon.releaseShutter();

  std::cout << "Waiting for 1 second..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(1));

  std::cout << "Releasing shutter..." << std::endl;
  canon.releaseShutter();

  std::cout << "Waiting for 5 seconds..." << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(5));

  return 0;
}