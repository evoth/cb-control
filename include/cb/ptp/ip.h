#ifndef CB_CONTROL_PTP_IP_H
#define CB_CONTROL_PTP_IP_H

#include <cb/protocols/tcp.h>
#include <cb/ptp/ptp.h>

// TODO: Figure out where to catch and deal with exceptions
class PTPIP : public PTPTransport {
 public:
  PTPIP(std::unique_ptr<TCPSocket> commandSocket,
        std::unique_ptr<TCPSocket> eventSocket,
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
    if (!this->commandSocket || !this->eventSocket)
      throw Exception(ExceptionContext::Socket, ExceptionType::IsNull);
  }

  virtual ~PTPIP() { close(); }

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
  std::unique_ptr<TCPSocket> commandSocket;
  std::unique_ptr<TCPSocket> eventSocket;
  const std::array<uint8_t, 16> clientGuid;
  const std::string clientName;
  const std::string ip;
  const int port;

  std::array<uint8_t, 16> guid;
  std::string name;
};

#endif