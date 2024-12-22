#include "ptp.h"

void PTP::openTransport() {
  Logger::log("Opening transport...");
  if (!transport)
    throw PTPTransportException("Transport is null.");
  transport->open();
}

void PTP::closeTransport() {
  if (!transport)
    throw PTPTransportException("Transport is null.");
  transport->close();
}

bool PTP::isTransportOpen() {
  if (!transport)
    throw PTPTransportException("Transport is null.");
  return transport->isOpen();
}

void PTP::openSession() {
  if (isSessionOpen)
    return;
  if (!isTransportOpen())
    openTransport();
  sessionId++;
  transactionId = 1;
  mesg(OperationCode::OpenSession, {sessionId});
  isSessionOpen = true;
}

void PTP::closeSession() {
  if (!isSessionOpen)
    return;
  mesg(OperationCode::CloseSession);
  isSessionOpen = false;
}

std::unique_ptr<DeviceInfo> PTP::getDeviceInfo() {
  Buffer data = recv(OperationCode::GetDeviceInfo).data;
  std::unique_ptr<DeviceInfo> deviceInfo = std::make_unique<DeviceInfo>();
  deviceInfo->unpack(data);
  return deviceInfo;
}

// TODO: Avoid copying `data`?
OperationResponseData PTP::send(uint16_t operationCode,
                                std::array<uint32_t, 5> params,
                                std::vector<unsigned char> data) {
  std::lock_guard lock(transactionMutex);

  if (!transport)
    throw PTPTransportException("Transport is null.");

  OperationRequestData request(true, true, operationCode, getSessionId(),
                               getTransactionId(), params, data);
  OperationResponseData response = transport->transaction(request);
  if (response.responseCode != ResponseCode::OK)
    throw PTPOperationException(response.responseCode);

  return response;
};

OperationResponseData PTP::recv(uint16_t operationCode,
                                std::array<uint32_t, 5> params) {
  std::lock_guard lock(transactionMutex);

  if (!transport)
    throw PTPTransportException("Transport is null.");

  OperationRequestData request(true, false, operationCode, getSessionId(),
                               getTransactionId(), params);
  OperationResponseData response = transport->transaction(request);
  if (response.responseCode != ResponseCode::OK)
    throw PTPOperationException(response.responseCode);

  return response;
};

OperationResponseData PTP::mesg(uint16_t operationCode,
                                std::array<uint32_t, 5> params) {
  std::lock_guard lock(transactionMutex);

  if (!transport)
    throw PTPTransportException("Transport is null.");

  OperationRequestData request(false, false, operationCode, getSessionId(),
                               getTransactionId(), params);
  OperationResponseData response = transport->transaction(request);
  if (response.responseCode != ResponseCode::OK)
    throw PTPOperationException(response.responseCode);

  return response;
};