#include "ip.h"

PTPIP::PTPIP(std::array<uint8_t, 16> clientGuid,
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
    throw PTPTransportException(
        "Unexpected packet type while initializing connection.");
  }

  this->guid = initCmdAck->guid;
  this->name = initCmdAck->name;

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
    throw PTPTransportException(
        "Unexpected packet type while initializing connection.");
  }
}

void PTPIP::sendPacket(std::unique_ptr<Socket>& socket, Packet& packet) {
  Buffer buffer = packet.pack();

  int targetBytes = buffer.size();
  int actualBytes = socket->send(buffer);
  if (actualBytes < targetBytes)
    throw PTPTransportException(
        std::format("Socket unable to send packet ({}/{} bytes sent).",
                    actualBytes, targetBytes)
            .c_str());
}

void PTPIP::recvPacket(std::unique_ptr<Socket>& socket, Buffer& buffer) {
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
  targetBytes = packet.length - sizeof(packet.length);
  actualBytes = socket->recv(buffer, targetBytes);
  if (actualBytes < targetBytes)
    throw PTPTransportException(
        std::format("Socket timed out while receiving packet body ({}/{} "
                    "bytes received).",
                    actualBytes, targetBytes)
            .c_str());
}

OperationResponseData PTPIP::transaction(OperationRequestData& request,
                                         uint32_t sessionId,
                                         uint32_t transactionId,
                                         DataPhaseInfo dataPhaseInfo) {
  if (!isOpen())
    throw PTPTransportException("Transport is not open.");

  OperationRequest opReq(static_cast<uint32_t>(dataPhaseInfo),
                         request.operationCode, transactionId, request.params);
  sendPacket(commandSocket, opReq);

  if (dataPhaseInfo == DataPhaseInfo::DataOut) {
    StartData startData(transactionId, request.data.size());
    sendPacket(commandSocket, startData);

    EndData endData(transactionId, request.data);
    sendPacket(commandSocket, endData);
  }

  Buffer payload;
  uint64_t totalDataLength = 0;
  while (true) {
    Buffer response;
    recvPacket(commandSocket, response);

    // TODO: Validate transactionId?
    if (auto opRes = Packet::unpackAs<OperationResponse>(response)) {
      // TODO: Replace with warnings?
      if (dataPhaseInfo == DataPhaseInfo::DataOut && payload.size() > 0)
        throw PTPTransportException(
            "Unexpected Data-In phase during Data-Out transaction.");
      if (payload.size() != totalDataLength)
        throw PTPTransportException(
            std::format("Wrong amount of data in transaction data phase ({}/{} "
                        "bytes received).",
                        payload.size(), totalDataLength)
                .c_str());
      return OperationResponseData(opRes->responseCode, opRes->params, payload);
    } else if (auto startData = Packet::unpackAs<StartData>(response)) {
      totalDataLength = startData->totalDataLength;
    } else if (auto data = Packet::unpackAs<Data>(response)) {
      payload.insert(payload.end(), data->payload.begin(), data->payload.end());
    } else if (auto endData = Packet::unpackAs<EndData>(response)) {
      payload.insert(payload.end(), endData->payload.begin(),
                     endData->payload.end());
    } else {
      throw PTPTransportException("Unexpected packet type during transaction.");
    }
  }
}

OperationResponseData PTPIP::send(OperationRequestData& request,
                                  uint32_t sessionId,
                                  uint32_t transactionId) {
  return transaction(request, sessionId, transactionId, DataPhaseInfo::DataOut);
}

OperationResponseData PTPIP::recv(OperationRequestData& request,
                                  uint32_t sessionId,
                                  uint32_t transactionId) {
  return transaction(request, sessionId, transactionId, DataPhaseInfo::DataIn);
}

OperationResponseData PTPIP::mesg(OperationRequestData& request,
                                  uint32_t sessionId,
                                  uint32_t transactionId) {
  return transaction(request, sessionId, transactionId, DataPhaseInfo::DataIn);
}