#ifndef CB_CONTROL_PTP_IPDATA_H
#define CB_CONTROL_PTP_IPDATA_H

#include "../packet.h"

enum class DataPhaseInfo : uint32_t {
  Unknown = 0x00,
  DataIn = 0x01,
  DataOut = 0x02,
};

/* PTP/IP Packets */

class IpPacket : public Packet {
 public:
  uint32_t packetType = 0;

  IpPacket(uint32_t packetType = 0) : packetType(packetType) {
    field();
    field(this->packetType);
  }

  template <typename T>
    requires(std::derived_from<T, Packet>)
  static std::unique_ptr<T> unpackAs(Buffer& buffer) {
    return Packet::unpackAs<IpPacket, T>(buffer);
  }

  uint32_t getType() override { return packetType; }
};

class InitCommandRequest : public IpPacket {
 public:
  std::array<uint8_t, 16> guid = {};
  std::string name = "";
  uint32_t ptpVersion = 0x10000;

  InitCommandRequest(std::array<uint8_t, 16> guid = {}, std::string name = "")
      : IpPacket(0x01), guid(guid), name(name) {
    field(this->guid);
    field(this->name);
    field(this->ptpVersion);
  }
};

class InitCommandAck : public IpPacket {
 public:
  uint32_t connectionNum = 0;
  std::array<uint8_t, 16> guid = {};
  std::string name = "";
  uint32_t ptpVersion = 0x10000;

  InitCommandAck() : IpPacket(0x02) {
    field(this->connectionNum);
    field(this->guid);
    field(this->name);
    field(this->ptpVersion);
  }
};

class InitEventRequest : public IpPacket {
 public:
  uint32_t connectionNum = 0;

  InitEventRequest(uint32_t connectionNum = 0)
      : IpPacket(0x03), connectionNum(connectionNum) {
    field(this->connectionNum);
  }
};

class InitEventAck : public IpPacket {
 public:
  InitEventAck() : IpPacket(0x04) {}
};

class InitFail : public IpPacket {
 public:
  uint32_t reason = 0;

  InitFail() : IpPacket(0x05) { field(this->reason); }
};

class OperationRequest : public IpPacket {
 public:
  uint32_t dataPhase = 0;
  uint16_t operationCode = 0;
  uint32_t transactionId = 0;
  std::array<uint32_t, 5> params = {};

  OperationRequest(uint32_t dataPhase = 0,
                   uint16_t operationCode = 0,
                   uint32_t transactionId = 0,
                   std::array<uint32_t, 5> params = {})
      : IpPacket(0x06),
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

class OperationResponse : public IpPacket {
 public:
  uint16_t responseCode = 0;
  uint32_t transactionId = 0;
  std::array<uint32_t, 5> params = {};

  OperationResponse() : IpPacket(0x07) {
    field(this->responseCode);
    field(this->transactionId);
    field(this->params);
  }
};

class Event : public IpPacket {
 public:
  uint16_t eventCode = 0;
  uint32_t transactionId = 0;
  std::array<uint32_t, 3> params = {};

  Event() : IpPacket(0x08) {
    field(this->eventCode);
    field(this->transactionId);
    field(this->params);
  }
};

class StartData : public IpPacket {
 public:
  uint32_t transactionId = 0;
  uint64_t totalDataLength = 0;

  StartData(uint32_t transactionId = 0, uint64_t totalDataLength = 0)
      : IpPacket(0x09),
        transactionId(transactionId),
        totalDataLength(totalDataLength) {
    field(this->transactionId);
    field(this->totalDataLength);
  }
};

class Data : public IpPacket {
 public:
  uint32_t transactionId = 0;
  Buffer payload;

  Data() : IpPacket(0x0a) {
    field(this->transactionId);
    field(this->payload);
  }
};

class Cancel : public IpPacket {
 public:
  uint32_t transactionId = 0;

  Cancel() : IpPacket(0x0b) { field(this->transactionId); }
};

class EndData : public IpPacket {
 public:
  uint32_t transactionId = 0;
  Buffer payload;

  EndData(uint32_t transactionId = 0, Buffer payload = {})
      : IpPacket(0x0c), transactionId(transactionId), payload(payload) {
    field(this->transactionId);
    field(this->payload);
  }
};

class Ping : public IpPacket {
 public:
  Ping() : IpPacket(0x0d) {}
};

class Pong : public IpPacket {
 public:
  Pong() : IpPacket(0x0e) {}
};

#endif