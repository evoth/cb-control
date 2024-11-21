#ifndef CB_CONTROL_PTP_IPDATA_H
#define CB_CONTROL_PTP_IPDATA_H

#include "../packet.h"

enum class DataPhaseInfo : uint32_t {
  Unknown = 0x00,
  DataIn = 0x01,
  DataOut = 0x02,
};

/* PTP/IP Packets */

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

  InitEventRequest(uint32_t connectionNum = 0)
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

  OperationRequest(uint32_t dataPhase = 0,
                   uint16_t operationCode = 0,
                   uint32_t transactionId = 0,
                   std::array<uint32_t, 5> params = {})
      : Packet(0x06),
        dataPhase(dataPhase),
        operationCode(operationCode),
        transactionId(transactionId),
        params(params) {
    field(this->dataPhase);
    field(this->operationCode);
    field(this->transactionId);
    field(this->params);  // Greedy; consumes to end of buffer when unpacking
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
    field(this->params);  // Greedy; consumes to end of buffer when unpacking
  }
};

class Event : public Packet {
 public:
  uint16_t eventCode = 0;
  uint32_t transactionId = 0;
  std::array<uint32_t, 3> params = {};

  Event() : Packet(0x08) {
    field(this->eventCode);
    field(this->transactionId);
    field(this->params);
  }
};

class StartData : public Packet {
 public:
  uint32_t transactionId = 0;
  uint64_t totalDataLength = 0;

  StartData(uint32_t transactionId = 0, uint64_t totalDataLength = 0)
      : Packet(0x09),
        transactionId(transactionId),
        totalDataLength(totalDataLength) {
    field(this->transactionId);
    field(this->totalDataLength);
  }
};

class Data : public Packet {
 public:
  uint32_t transactionId = 0;
  Buffer payload;

  Data() : Packet(0x0a) {
    field(this->transactionId);
    field(this->payload);  // Greedy; consumes to end of buffer when unpacking
  }
};

class Cancel : public Packet {
 public:
  uint32_t transactionId = 0;

  Cancel() : Packet(0x0b) { field(this->transactionId); }
};

class EndData : public Packet {
 public:
  uint32_t transactionId = 0;
  Buffer payload;

  EndData(uint32_t transactionId = 0, Buffer payload = {})
      : Packet(0x0c), transactionId(transactionId), payload(payload) {
    field(this->transactionId);
    field(this->payload);  // Greedy; consumes to end of buffer when unpacking
  }
};

class Ping : public Packet {
 public:
  Ping() : Packet(0x0d) {}
};

class Pong : public Packet {
 public:
  Pong() : Packet(0x0e) {}
};

#endif