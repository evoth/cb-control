#ifndef CB_CONTROL_PTP_IP_H
#define CB_CONTROL_PTP_IP_H

#include "../packet.h"
#include "ptp.h"

// TODO: Figure out where to catch and deal with exceptions

class InitCommandRequest : public Packet {
 public:
  std::array<uint8_t, 16> guid = {0};
  std::string name = "";
  uint32_t ptpVersion = 0x10000;

  InitCommandRequest(std::array<uint8_t, 16> guid = {0}, std::string name = "")
      : Packet(0x01), guid(guid), name(name) {
    field(this->guid);
    field(this->name);
    field(this->ptpVersion);
  }
};

class InitCommandAck : public Packet {
 public:
  uint32_t connectionNum = 0;
  std::array<uint8_t, 16> guid = {0};
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

  InitEventRequest() : Packet(0x03) { field(this->connectionNum); }
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
  std::array<uint32_t, 5> params = {0};

  OperationRequest() : Packet(0x06) {
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
  std::array<uint32_t, 5> params = {0};

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

class Socket {
 public:
  virtual bool connect(std::string ip, int port) = 0;
  virtual bool close() = 0;
  virtual bool isConnected() = 0;
  virtual int send(Buffer& buffer) = 0;
  virtual int recv(Buffer& buffer) = 0;
};

template <typename TSocket>
  requires(std::derived_from<TSocket, Socket>)
class PTPIP : public PTPTransport {
 public:
  PTPIP(std::string ip, int port = 15740) {
    // TODO:
  }
  ~PTPIP() {
    // TODO:
  }

  bool isOpen() override {

  };

  OperationResponseData send(OperationRequestData& request,
                             uint32_t sessionId,
                             uint32_t transactionId) override {
    // TODO:
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
};

#endif
