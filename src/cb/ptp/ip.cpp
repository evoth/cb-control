#include <cb/ptp/ip.h>
#include <cb/ptp/ipData.h>

#include <cb/logger.h>

namespace cb {

// TODO: Formal logging

void PTPIP::open() {
  if (isOpen())
    return;

  if (!commandSocket->connect(ip, port))
    throw Exception(ExceptionContext::PTPIPConnect,
                    ExceptionType::ConnectFailure);

  Buffer response;

  InitCommandRequest(clientGuid, clientName).send(*commandSocket);
  IPPacket().recv(*commandSocket, response, 60000);
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

  InitEventRequest(initCmdAck->connectionNum).send(*eventSocket);
  IPPacket().recv(*eventSocket, response);
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
  OperationRequest(static_cast<uint32_t>(dataPhaseInfo), request.operationCode,
                   request.transactionId, request.params)
      .send(*commandSocket);

  if (dataPhaseInfo == DataPhaseInfo::DataOut) {
    StartData(request.transactionId, request.data.size()).send(*commandSocket);

    // TODO: Avoid copying request.data?
    EndData(request.transactionId, request.data).send(*commandSocket);
  }

  Buffer payload;
  uint64_t totalDataLength = 0;
  while (true) {
    Buffer response;
    IPPacket().recv(*commandSocket, response);

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

}