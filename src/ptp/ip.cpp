#include "ip.h"
#include "ipData.h"

#include "../logger.h"

// TODO: Formal logging

void PTPIP::open() {
  if (isOpen())
    return;

  if (!commandSocket->connect(ip, port))
    throw Exception(ExceptionContext::PTPIPConnect,
                    ExceptionType::ConnectFailure);

  Buffer response;

  InitCommandRequest initCmdReq(clientGuid, clientName);
  commandSocket->sendPacket(initCmdReq);
  commandSocket->recvPacket<IPPacket>(response, 60000);
  auto initCmdAck = IPPacket::unpackAs<InitCommandAck>(response);

  if (!initCmdAck) {
    // TODO: Move to helper function to avoid duplication?
    if (auto initFail = IPPacket::unpackAs<InitFail>(response))
      throw Exception(ExceptionContext::PTPIPConnect,
                      ExceptionType::InitFailure);
    throw Exception(ExceptionContext::PTPIPConnect,
                    ExceptionType::UnexpectedPacket);
  }

  guid = initCmdAck->guid;
  name = initCmdAck->name;

  if (!eventSocket->connect(ip, port))
    throw Exception(ExceptionContext::PTPIPConnect,
                    ExceptionType::ConnectFailure);

  InitEventRequest initEvtReq(initCmdAck->connectionNum);
  eventSocket->sendPacket(initEvtReq);
  eventSocket->recvPacket<IPPacket>(response);
  auto initEvtAck = IPPacket::unpackAs<InitEventAck>(response);

  if (!initEvtAck) {
    // TODO: Move to helper function to avoid duplication?
    if (auto initFail = IPPacket::unpackAs<InitFail>(response))
      throw Exception(ExceptionContext::PTPIPConnect,
                      ExceptionType::InitFailure);
    throw Exception(ExceptionContext::PTPIPConnect,
                    ExceptionType::UnexpectedPacket);
  }
}

OperationResponseData PTPIP::transaction(const OperationRequestData& request) {
  Logger::log(
      "PTPIP Operation Request (operationCode=0x%04x, transactionId=%d, "
      "param1=0x%04x, dataPhase=%d, sending=%d)",
      request.operationCode, request.transactionId, request.params[0],
      request.dataPhase, request.sending);

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
    commandSocket->recvPacket<IPPacket>(response);

    // TODO: Validate transactionId?
    if (auto opRes = IPPacket::unpackAs<OperationResponse>(response)) {
      Logger::log("> Operation Response (responseCode=0x%04x)",
                  opRes->responseCode);
      if (payload.size() != totalDataLength)
        throw Exception(ExceptionContext::PTPIPTransaction,
                        ExceptionType::WrongDataLength);
      return OperationResponseData(opRes->responseCode, opRes->params, payload);
    } else if (auto startData = IPPacket::unpackAs<StartData>(response)) {
      Logger::log("> Start Data (totalDataLength=%d)",
                  startData->totalDataLength);
      totalDataLength = startData->totalDataLength;
    } else if (auto data = IPPacket::unpackAs<Data>(response)) {
      Logger::log("> Data (payload.size()=%d)", data->payload.size());
      payload.insert(payload.end(), data->payload.begin(), data->payload.end());
    } else if (auto endData = IPPacket::unpackAs<EndData>(response)) {
      Logger::log("> End Data (payload.size()=%d)", endData->payload.size());
      payload.insert(payload.end(), endData->payload.begin(),
                     endData->payload.end());
    } else {
      throw Exception(ExceptionContext::PTPIPTransaction,
                      ExceptionType::UnexpectedPacket);
    }
  }
}