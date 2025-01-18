#ifndef CB_CONTROL_PTP_IPDATA_H
#define CB_CONTROL_PTP_IPDATA_H

#include "../protocols/tcp.h"

enum class DataPhaseInfo : uint32_t {
  Unknown = 0x00,
  DataIn = 0x01,
  DataOut = 0x02,
};

/* PTP/IP Packets */

class IPPacket : public TCPPacket {
 public:
  uint32_t length = 0;
  uint32_t packetType = 0;

  IPPacket(uint32_t packetType) : packetType(packetType) {
    lengthField(this->length);
    typeField(this->packetType);
  }
  IPPacket() : IPPacket(0) {}

  template <typename T>
    requires(std::derived_from<T, IPPacket>)
  static std::unique_ptr<T> unpackAs(const Buffer& buffer) {
    return Packet::unpackAs<IPPacket, T>(buffer);
  }
};

class InitCommandRequest : public IPPacket {
 public:
  std::array<uint8_t, 16> guid = {};
  std::string name;
  uint32_t ptpVersion = 0x10000;

  InitCommandRequest(std::array<uint8_t, 16> guid, std::string name)
      : IPPacket(0x01), guid(guid), name(name) {
    field(this->guid);
    field(this->name);
    field(this->ptpVersion);
  }
  InitCommandRequest() : InitCommandRequest({}, "") {}
};

class InitCommandAck : public IPPacket {
 public:
  uint32_t connectionNum = 0;
  std::array<uint8_t, 16> guid = {};
  std::string name;
  uint32_t ptpVersion = 0x10000;

  InitCommandAck() : IPPacket(0x02) {
    field(this->connectionNum);
    field(this->guid);
    field(this->name);
    field(this->ptpVersion);
  }
};

class InitEventRequest : public IPPacket {
 public:
  uint32_t connectionNum = 0;

  InitEventRequest(uint32_t connectionNum)
      : IPPacket(0x03), connectionNum(connectionNum) {
    field(this->connectionNum);
  }
  InitEventRequest() : InitEventRequest(0) {}
};

class InitEventAck : public IPPacket {
 public:
  InitEventAck() : IPPacket(0x04) {}
};

class InitFail : public IPPacket {
 public:
  uint32_t reason = 0;

  InitFail() : IPPacket(0x05) { field(this->reason); }
};

class OperationRequest : public IPPacket {
 public:
  uint32_t dataPhase = 0;
  uint16_t operationCode = 0;
  uint32_t transactionId = 0;
  std::array<uint32_t, 5> params = {};

  OperationRequest(uint32_t dataPhase,
                   uint16_t operationCode,
                   uint32_t transactionId,
                   std::array<uint32_t, 5> params)
      : IPPacket(0x06),
        dataPhase(dataPhase),
        operationCode(operationCode),
        transactionId(transactionId),
        params(params) {
    field(this->dataPhase);
    field(this->operationCode);
    field(this->transactionId);
    field(this->params);
  }
  OperationRequest() : OperationRequest(0, 0, 0, {}) {}
};

class OperationResponse : public IPPacket {
 public:
  uint16_t responseCode = 0;
  uint32_t transactionId = 0;
  std::array<uint32_t, 5> params = {};

  OperationResponse() : IPPacket(0x07) {
    field(this->responseCode);
    field(this->transactionId);
    field(this->params);
  }
};

class Event : public IPPacket {
 public:
  uint16_t eventCode = 0;
  uint32_t transactionId = 0;
  std::array<uint32_t, 3> params = {};

  Event() : IPPacket(0x08) {
    field(this->eventCode);
    field(this->transactionId);
    field(this->params);
  }
};

class StartData : public IPPacket {
 public:
  uint32_t transactionId = 0;
  uint64_t totalDataLength = 0;

  StartData(uint32_t transactionId, uint64_t totalDataLength)
      : IPPacket(0x09),
        transactionId(transactionId),
        totalDataLength(totalDataLength) {
    field(this->transactionId);
    field(this->totalDataLength);
  }
  StartData() : StartData(0, 0) {}
};

class Data : public IPPacket {
 public:
  uint32_t transactionId = 0;
  Buffer payload;

  Data() : IPPacket(0x0a) {
    field(this->transactionId);
    field(this->payload);
  }
};

class Cancel : public IPPacket {
 public:
  uint32_t transactionId = 0;

  Cancel() : IPPacket(0x0b) { field(this->transactionId); }
};

class EndData : public IPPacket {
 public:
  uint32_t transactionId = 0;
  Buffer payload;

  // TODO: Use move semantics for payload
  EndData(uint32_t transactionId, Buffer payload)
      : IPPacket(0x0c), transactionId(transactionId), payload(payload) {
    field(this->transactionId);
    field(this->payload);
  }
  EndData() : EndData(0, {}) {}
};

class Ping : public IPPacket {
 public:
  Ping() : IPPacket(0x0d) {}
};

class Pong : public IPPacket {
 public:
  Pong() : IPPacket(0x0e) {}
};

#endif