#include "ip.h"
#include "ipData.h"

// TODO: Formal logging

void Socket::sendPacket(Packet& packet) {
  Buffer buffer = packet.pack();
  if (send(buffer) < buffer.size())
    throw PTPTransportException("Socket unable to send packet.");
}

// TODO: Better timeout handling
void Socket::recvPacket(Buffer& buffer, unsigned int timeoutMs) {
  Packet packet(0);
  buffer.clear();

  int targetBytes = sizeof(packet.length);
  if (recv(buffer, targetBytes, timeoutMs) < targetBytes)
    throw PTPTransportException(
        "Socket timed out while receiving length of next packet.");

  packet.unpack(buffer);
  targetBytes = packet.length - sizeof(packet.length);
  if (recv(buffer, targetBytes, timeoutMs) < targetBytes)
    throw PTPTransportException(
        "Socket timed out while receiving packet body.");
}

void PTPIP::open() {
  if (isOpen())
    return;

  if (!this->commandSocket->connect(ip, port))
    throw PTPTransportException("Unable to connect command socket.");

  Buffer response;

  InitCommandRequest initCmdReq(clientGuid, clientName);
  this->commandSocket->sendPacket(initCmdReq);
  this->commandSocket->recvPacket(response, 60000);
  auto initCmdAck = Packet::unpackAs<InitCommandAck>(response);

  if (!initCmdAck) {
    // TODO: Move to helper function to avoid duplication?
    if (auto initFail = Packet::unpackAs<InitFail>(response))
      throw PTPTransportException("Init Fail (reason code 0x%04x)",
                                  initFail->reason);
    throw PTPTransportException(
        "Unexpected packet type while initializing connection.");
  }

  this->guid = initCmdAck->guid;
  this->name = initCmdAck->name;

  if (!this->eventSocket->connect(ip, port))
    throw PTPTransportException("Unable to connect event socket.");

  InitEventRequest initEvtReq(initCmdAck->connectionNum);
  this->eventSocket->sendPacket(initEvtReq);
  this->eventSocket->recvPacket(response);
  auto initEvtAck = Packet::unpackAs<InitEventAck>(response);

  if (!initEvtAck) {
    // TODO: Move to helper function to avoid duplication?
    if (auto initFail = Packet::unpackAs<InitFail>(response))
      throw PTPTransportException("Init Fail (reason code 0x%04x)",
                                  initFail->reason);
    throw PTPTransportException(
        "Unexpected packet type while initializing connection.");
  }
}

OperationResponseData PTPIP::transaction(const OperationRequestData& request) {
  Logger::log(
      "PTPIP Operation Request (operationCode=0x%04x, transactionId=%d, "
      "param1=0x%04x, dataPhase=%d, sending=%d)",
      request.operationCode, request.transactionId, request.params[0],
      request.dataPhase, request.sending);

  if (!isOpen())
    throw PTPTransportException("Transport is not open.");

  DataPhaseInfo dataPhaseInfo = (request.dataPhase && request.sending)
                                    ? DataPhaseInfo::DataOut
                                    : DataPhaseInfo::DataIn;
  OperationRequest opReq(static_cast<uint32_t>(dataPhaseInfo),
                         request.operationCode, request.transactionId,
                         request.params);
  commandSocket->sendPacket(opReq);

  if (dataPhaseInfo == DataPhaseInfo::DataOut) {
    StartData startData(request.transactionId, request.data.size());
    commandSocket->sendPacket(startData);

    // TODO: Avoid copying request.data?
    EndData endData(request.transactionId, request.data);
    commandSocket->sendPacket(endData);
  }

  Buffer payload;
  uint64_t totalDataLength = 0;
  while (true) {
    Buffer response;
    commandSocket->recvPacket(response);

    // TODO: Validate transactionId?
    if (auto opRes = Packet::unpackAs<OperationResponse>(response)) {
      Logger::log("> Operation Response (responseCode=0x%04x)",
                  opRes->responseCode);
      // TODO: Replace with warnings?
      if (dataPhaseInfo == DataPhaseInfo::DataOut && payload.size() > 0)
        throw PTPTransportException(
            "Unexpected Data-In phase during Data-Out transaction.");
      if (payload.size() != totalDataLength)
        throw PTPTransportException(
            "Wrong amount of data in transaction data phase.");
      return OperationResponseData(opRes->responseCode, opRes->params, payload);
    } else if (auto startData = Packet::unpackAs<StartData>(response)) {
      Logger::log("> Start Data (totalDataLength=%d)",
                  startData->totalDataLength);
      totalDataLength = startData->totalDataLength;
    } else if (auto data = Packet::unpackAs<Data>(response)) {
      Logger::log("> Data (payload.size()=%d)", data->payload.size());
      payload.insert(payload.end(), data->payload.begin(), data->payload.end());
    } else if (auto endData = Packet::unpackAs<EndData>(response)) {
      Logger::log("> End Data (payload.size()=%d)", endData->payload.size());
      payload.insert(payload.end(), endData->payload.begin(),
                     endData->payload.end());
    } else {
      throw PTPTransportException("Unexpected packet type during transaction.");
    }
  }
}