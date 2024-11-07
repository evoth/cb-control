#include "ptp.h"

// TODO: Avoid copying `data`?
OperationResponseData PTPExtension::send(uint16_t operationCode,
                                         std::array<uint32_t, 5> params,
                                         std::vector<unsigned char> data) {
  std::lock_guard lock(transactionMutex);

  OperationRequestData request(operationCode, true, true, params, data);
  OperationResponseData response =
      transport->transaction(request, getSessionId(), getTransactionId());
  if (response.responseCode != ResponseCode::OK)
    throw PTPOperationException(response.responseCode);

  return response;
};

OperationResponseData PTPExtension::recv(uint16_t operationCode,
                                         std::array<uint32_t, 5> params) {
  std::lock_guard lock(transactionMutex);

  OperationRequestData request(operationCode, true, false, params);
  OperationResponseData response =
      transport->transaction(request, getSessionId(), getTransactionId());
  if (response.responseCode != ResponseCode::OK)
    throw PTPOperationException(response.responseCode);

  return response;
};

OperationResponseData PTPExtension::mesg(uint16_t operationCode,
                                         std::array<uint32_t, 5> params) {
  std::lock_guard lock(transactionMutex);

  OperationRequestData request(operationCode, false, false, params);
  OperationResponseData response =
      transport->transaction(request, getSessionId(), getTransactionId());
  if (response.responseCode != ResponseCode::OK)
    throw PTPOperationException(response.responseCode);

  return response;
};

void PTPExtension::openSession() {
  if (isSessionOpen)
    return;
  sessionId++;
  transactionId = 1;
  mesg(OperationCode::OpenSession, {sessionId});
  isSessionOpen = true;
}

void PTPExtension::closeSession() {
  if (!isSessionOpen)
    return;
  mesg(OperationCode::CloseSession);
  isSessionOpen = false;
}