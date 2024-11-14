#ifndef CB_CONTROL_PTP_IP_H
#define CB_CONTROL_PTP_IP_H

#include "../logger.h"
#include "ptp.h"

// Ideally, these should not throw any exceptions
// If send/recv are called when not connected, they should return 0
class Socket {
 public:
  virtual ~Socket() = default;

  virtual bool connect(const std::string& ip, int port) = 0;
  virtual bool close() = 0;
  virtual bool isConnected() = 0;

  void sendPacket(Packet& packet);
  void recvPacket(Buffer& buffer, unsigned int timeoutMs = 10000);

 protected:
  virtual int send(Buffer& buffer) = 0;
  // Should attempt to append `length` bytes to the buffer, waiting until enough
  // bytes have accumulated or `timeoutMs` milliseconds have passed
  virtual int recv(Buffer& buffer, int length, unsigned int timeoutMs) = 0;
};

enum class DataPhaseInfo : uint32_t {
  Unknown = 0x00,
  DataIn = 0x01,
  DataOut = 0x02,
};

// TODO: Figure out where to catch and deal with exceptions
class PTPIP : public PTPTransport {
 public:
  PTPIP(std::unique_ptr<Socket> commandSocket,
        std::unique_ptr<Socket> eventSocket,
        std::array<uint8_t, 16> clientGuid,
        std::string clientName,
        std::string ip,
        int port = 15740)
      : commandSocket(std::move(commandSocket)),
        eventSocket(std::move(eventSocket)),
        clientGuid(clientGuid),
        clientName(clientName),
        ip(ip),
        port(port) {
    if (!this->commandSocket)
      throw PTPTransportException("No command socket provided.");
    if (!this->eventSocket)
      throw PTPTransportException("No event socket provided.");
  }

  virtual ~PTPIP() {
    Logger::log("PTPIP destructed");
    close();
  }

  void open() override;

  void close() override {
    commandSocket->close();
    eventSocket->close();
  };

  bool isOpen() override {
    return commandSocket->isConnected() && eventSocket->isConnected();
  };

  OperationResponseData transaction(
      const OperationRequestData& request) override;

 private:
  std::unique_ptr<Socket> commandSocket;
  std::unique_ptr<Socket> eventSocket;
  const std::array<uint8_t, 16> clientGuid;
  const std::string clientName;
  const std::string ip;
  const int port;

  std::array<uint8_t, 16> guid;
  std::string name;
};

#endif