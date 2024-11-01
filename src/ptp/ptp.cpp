#include "ptp.h"

OperationResponseData PTPExtension::send(OperationRequestData& request) {
  transport->send(request, getSessionId(), getTransactionId());
};

OperationResponseData PTPExtension::recv(OperationRequestData& request) {
  transport->recv(request, getSessionId(), getTransactionId());
};

OperationResponseData PTPExtension::mesg(OperationRequestData& request) {
  transport->mesg(request, getSessionId(), getTransactionId());
};

void PTPExtension::openSession() {
  if (isSessionOpen)
    return;
  sessionId++;
  transactionId = 1;
  OperationRequestData request(OperationCode::OpenSession, {sessionId});
  OperationResponseData response = mesg(request);
  if (response.responseCode != ResponseCode::OK)
    throw PTPOperationException(response.responseCode);
  isSessionOpen = true;
}

void PTPExtension::closeSession() {
  if (!isSessionOpen)
    return;
  OperationRequestData request(OperationCode::CloseSession);
  OperationResponseData response = mesg(request);
  if (response.responseCode != ResponseCode::OK)
    throw PTPOperationException(response.responseCode);
  isSessionOpen = false;
}