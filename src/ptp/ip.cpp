#include "ip.h"

// TODO: Formal logging
#include <iostream>

void Socket::sendPacket(Packet& packet) {
  Buffer buffer = packet.pack();

  int targetBytes = buffer.size();
  int actualBytes = send(buffer);
  if (actualBytes < targetBytes)
    throw PTPTransportException(
        std::format("Socket unable to send packet ({}/{} bytes sent).",
                    actualBytes, targetBytes));
}

// TODO: Better timeout handling
void Socket::recvPacket(Buffer& buffer, unsigned int timeoutMs) {
  Packet packet;
  buffer.clear();

  int targetBytes = sizeof(packet.length);
  int actualBytes = recv(buffer, targetBytes, timeoutMs);
  if (actualBytes < targetBytes)
    throw PTPTransportException(
        std::format("Socket timed out while receiving packet length ({}/{} "
                    "bytes received).",
                    actualBytes, targetBytes));

  packet.unpack(buffer);
  targetBytes = packet.length - sizeof(packet.length);
  actualBytes = recv(buffer, targetBytes, timeoutMs);
  if (actualBytes < targetBytes)
    throw PTPTransportException(
        std::format("Socket timed out while receiving packet body ({}/{} "
                    "bytes received).",
                    actualBytes, targetBytes));
}

PTPIP::PTPIP(std::array<uint8_t, 16> clientGuid,
             const std::string& clientName,
             std::unique_ptr<Socket> commandSocket,
             std::unique_ptr<Socket> eventSocket,
             const std::string& ip,
             int port)
    : commandSocket(std::move(commandSocket)),
      eventSocket(std::move(eventSocket)) {
  if (!this->commandSocket)
    throw PTPTransportException("No command socket provided.");
  if (!this->eventSocket)
    throw PTPTransportException("No event socket provided.");

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
      throw PTPTransportException(
          std::format("Init Fail (reason code {:#04x})", initFail->reason));
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
      throw PTPTransportException(
          std::format("Init Fail (reason code {:#04x})", initFail->reason));
    throw PTPTransportException(
        "Unexpected packet type while initializing connection.");
  }
}

OperationResponseData PTPIP::transaction(const OperationRequestData& request) {
  std::cout << "PTPIP Operation Request "
            << std::format(
                   "(operationCode={:#04x}, transactionId={}, param1={:#04x}, "
                   "dataPhase={}, sending={})",
                   request.operationCode, request.transactionId,
                   request.params[0], request.dataPhase, request.sending)
            << std::endl;

  if (!isOpen())
    throw PTPTransportException("Transport is not open.");

  uint32_t dataPhaseInfo = (request.dataPhase && request.sending)
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
      std::cout << "> Operation Response "
                << std::format("(responseCode={:#04x})", opRes->responseCode)
                << std::endl;
      // TODO: Replace with warnings?
      if (dataPhaseInfo == DataPhaseInfo::DataOut && payload.size() > 0)
        throw PTPTransportException(
            "Unexpected Data-In phase during Data-Out transaction.");
      if (payload.size() != totalDataLength)
        throw PTPTransportException(
            std::format("Wrong amount of data in transaction data phase ({}/{} "
                        "bytes received).",
                        payload.size(), totalDataLength));
      return OperationResponseData(opRes->responseCode, opRes->params, payload);
    } else if (auto startData = Packet::unpackAs<StartData>(response)) {
      std::cout << "> Start Data "
                << std::format("(totalDataLength={})",
                               startData->totalDataLength)
                << std::endl;
      totalDataLength = startData->totalDataLength;
    } else if (auto data = Packet::unpackAs<Data>(response)) {
      std::cout << "> Data "
                << std::format("(payload.size()={})", data->payload.size())
                << std::endl;
      payload.insert(payload.end(), data->payload.begin(), data->payload.end());
    } else if (auto endData = Packet::unpackAs<EndData>(response)) {
      std::cout << "> End Data "
                << std::format("(payload.size()={})", endData->payload.size())
                << std::endl;
      payload.insert(payload.end(), endData->payload.begin(),
                     endData->payload.end());
    } else {
      throw PTPTransportException("Unexpected packet type during transaction.");
    }
  }
}