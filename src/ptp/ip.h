#ifndef CB_CONTROL_PTP_IP_H
#define CB_CONTROL_PTP_IP_H

#include "../packet.h"
#include "ptp.h"

#include <queue>

// TODO: Figure out where to catch and deal with exceptions

class InitCommandRequest : public Packet {
 public:
  std::array<uint8_t, 16> guid = {};
  std::string name = "";
  uint32_t ptpVersion = 0x10000;

  InitCommandRequest(std::array<uint8_t, 16> guid = {}, std::string name = "")
      : Packet(0x01), guid(guid), name(name) {
    field(this->guid);
    field(this->name);
    field(this->ptpVersion);
  }
};

class InitCommandAck : public Packet {
 public:
  uint32_t connectionNum = 0;
  std::array<uint8_t, 16> guid = {};
  std::string name = "";
  uint32_t ptpVersion = 0x10000;

  InitCommandAck() : Packet(0x02) {
    field(this->connectionNum);
    field(this->guid);
    field(this->name);
    field(this->ptpVersion);
  }
};

class InitEventRequest : public Packet {
 public:
  uint32_t connectionNum = 0;

  InitEventRequest(uint32_t connectionNum)
      : Packet(0x03), connectionNum(connectionNum) {
    field(this->connectionNum);
  }
};

class InitEventAck : public Packet {
 public:
  InitEventAck() : Packet(0x04) {}
};

class InitFail : public Packet {
 public:
  uint32_t reason = 0;

  InitFail() : Packet(0x05) { field(this->reason); }
};

class OperationRequest : public Packet {
 public:
  uint32_t dataPhase = 0;
  uint16_t operationCode = 0;
  uint32_t transactionId = 0;
  std::array<uint32_t, 5> params = {};

  OperationRequest(uint32_t dataPhase,
                   uint16_t operationCode,
                   uint32_t transactionId,
                   std::array<uint32_t, 5> params)
      : Packet(0x06),
        dataPhase(dataPhase),
        operationCode(operationCode),
        transactionId(transactionId),
        params(params) {
    field(this->dataPhase);
    field(this->operationCode);
    field(this->transactionId);
    field(this->params);
  }
};

class OperationResponse : public Packet {
 public:
  uint16_t responseCode = 0;
  uint32_t transactionId = 0;
  std::array<uint32_t, 5> params = {};

  OperationResponse() : Packet(0x07) {
    field(this->responseCode);
    field(this->transactionId);
    field(this->params);
  }
};

// TODO: Event

class StartData : public Packet {
 public:
  uint32_t transactionId = 0;
  uint64_t totalDataLength = 0;

  StartData() : Packet(0x09) {
    field(this->transactionId);
    field(this->totalDataLength);
  }
};

// TODO: Data, Cancel

class EndData : public Packet {
 public:
  uint32_t transactionId = 0;
  uint64_t totalDataLength = 0;

  EndData() : Packet(0x0c) {
    field(this->transactionId);
    field(this->totalDataLength);
  }
};

// TODO: Ping, Pong

// Ideally, these should not throw any exceptions
// If send/recv are called when not connected, they should return 0
class Socket {
 public:
  virtual bool connect(std::string ip, int port) = 0;
  virtual bool close() = 0;
  virtual bool isConnected() = 0;
  virtual int send(Buffer& buffer) = 0;
  // Should attempt to append `length` bytes to the buffer, waiting until enough
  // bytes have accumulated or `timeoutMs` milliseconds have passed
  virtual int recv(Buffer& buffer, int length, int timeoutMs = 1000) = 0;
};

// TODO: Move stuff into implementation file
class PTPIP : public PTPTransport {
 public:
  PTPIP(std::array<uint8_t, 16> clientGuid,
        std::string clientName,
        std::unique_ptr<Socket> commandSocket,
        std::unique_ptr<Socket> eventSocket,
        std::string ip,
        int port = 15740)
      : commandSocket(std::move(commandSocket)),
        eventSocket(std::move(eventSocket)) {
    if (!commandSocket)
      throw PTPTransportException("No command socket provided.");
    if (!eventSocket)
      throw PTPTransportException("No event socket provided.");

    if (!commandSocket->connect(ip, port))
      throw PTPTransportException("Unable to connect command socket.");

    Buffer response;

    InitCommandRequest initCmdReq(clientGuid, clientName);
    sendPacket(commandSocket, initCmdReq);
    recvPacket(commandSocket, response);
    auto initCmdAck = Packet::unpackAs<InitCommandAck>(response);

    if (!initCmdAck) {
      // TODO: Move to helper function to avoid duplication?
      if (auto initFail = Packet::unpackAs<InitFail>(response))
        throw PTPTransportException(
            std::format("Init Fail (reason code {:#04x})", initFail->reason)
                .c_str());
      throw PTPTransportException("Unexpected packet type.");
    }

    guid = initCmdAck->guid;
    name = initCmdAck->name;

    if (!eventSocket->connect(ip, port))
      throw PTPTransportException("Unable to connect event socket.");

    InitEventRequest initEvtReq(initCmdAck->connectionNum);
    sendPacket(eventSocket, initEvtReq);
    recvPacket(eventSocket, response);
    auto initEvtAck = Packet::unpackAs<InitEventAck>(response);

    if (!initEvtAck) {
      // TODO: Move to helper function to avoid duplication?
      if (auto initFail = Packet::unpackAs<InitFail>(response))
        throw PTPTransportException(
            std::format("Init Fail (reason code {:#04x})", initFail->reason)
                .c_str());
      throw PTPTransportException("Unexpected packet type.");
    }
  }

  ~PTPIP() {
    commandSocket->close();
    eventSocket->close();
  }

  bool isOpen() override {
    return commandSocket->isConnected() && eventSocket->isConnected();
  };

  OperationResponseData send(OperationRequestData& request,
                             uint32_t sessionId,
                             uint32_t transactionId) override {
    // TODO:
    // if (!isOpen())
    //   throw PTPTransportException("Transport is not open.");
  }

  OperationResponseData recv(OperationRequestData& request,
                             uint32_t sessionId,
                             uint32_t transactionId) override {
    // TODO:
  }

  OperationResponseData mesg(OperationRequestData& request,
                             uint32_t sessionId,
                             uint32_t transactionId) override {
    // TODO:
  }

 private:
  std::unique_ptr<Socket> commandSocket;
  std::unique_ptr<Socket> eventSocket;
  std::array<uint8_t, 16> guid;
  std::string name;

  void sendPacket(std::unique_ptr<Socket>& socket, Packet& packet) {
    Buffer buffer = packet.pack();

    int targetBytes = buffer.size();
    int actualBytes = socket->send(buffer);
    if (actualBytes < targetBytes)
      throw PTPTransportException(
          std::format("Socket unable to send packet ({}/{} bytes sent).",
                      actualBytes, targetBytes)
              .c_str());
  }

  void recvPacket(std::unique_ptr<Socket>& socket, Buffer& buffer) {
    Packet packet;
    buffer.clear();

    int targetBytes = sizeof(packet.length);
    int actualBytes = socket->recv(buffer, targetBytes);
    if (actualBytes < targetBytes)
      throw PTPTransportException(
          std::format("Socket timed out while receiving packet length ({}/{} "
                      "bytes received).",
                      actualBytes, targetBytes)
              .c_str());

    packet.unpack(buffer);
    targetBytes = packet.length - targetBytes;
    actualBytes = socket->recv(buffer, targetBytes);
    if (actualBytes < targetBytes)
      throw PTPTransportException(
          std::format("Socket timed out while receiving packet body ({}/{} "
                      "bytes received).",
                      actualBytes, targetBytes)
              .c_str());
  }
};

#endif
