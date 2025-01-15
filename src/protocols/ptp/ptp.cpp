#include "ptp.h"

#if defined(ESP32)
#include <esp_pthread.h>
#endif

void PTP::openTransport() {
  if (!transport)
    throw Exception(ExceptionContext::PTPTransport, ExceptionType::IsNull);
  transport->open();
}

void PTP::closeTransport() {
  if (!transport)
    throw Exception(ExceptionContext::PTPTransport, ExceptionType::IsNull);
  transport->close();
  isSessionOpen = false;
}

bool PTP::isTransportOpen() {
  if (!transport)
    throw Exception(ExceptionContext::PTPTransport, ExceptionType::IsNull);
  return transport->isOpen();
}

void PTP::openSession() {
  std::lock_guard lock(sessionMutex);
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
  std::lock_guard lock(sessionMutex);
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

OperationResponseData PTP::transaction(bool dataPhase,
                                       bool sending,
                                       uint16_t operationCode,
                                       std::array<uint32_t, 5> params,
                                       std::vector<uint8_t> data) {
  std::lock_guard lock(transactionMutex);

  if (!transport)
    throw Exception(ExceptionContext::PTPTransport, ExceptionType::IsNull);

  if (!isTransportOpen())
    throw Exception(ExceptionContext::PTPTransport,
                    ExceptionType::NotConnected);

  OperationRequestData request(dataPhase, sending, operationCode,
                               getSessionId(), getTransactionId(), params,
                               data);

  OperationResponseData response = transport->transaction(request);
  if (response.responseCode != ResponseCode::OK)
    throw Exception(ExceptionContext::PTPIPTransaction,
                    ExceptionType::OperationFailure);

  return response;
}

// TODO: Avoid copying `data`?
OperationResponseData PTP::send(uint16_t operationCode,
                                std::array<uint32_t, 5> params,
                                std::vector<uint8_t> data) {
  return transaction(true, true, operationCode, params, data);
};

OperationResponseData PTP::recv(uint16_t operationCode,
                                std::array<uint32_t, 5> params) {
  return transaction(true, false, operationCode, params);
};

OperationResponseData PTP::mesg(uint16_t operationCode,
                                std::array<uint32_t, 5> params) {
  return transaction(false, false, operationCode, params);
};

void PTPCamera::connect() {
  openSession();
  pushEvent<ConnectEvent>(true);
}

void PTPCamera::disconnect() {
  closeSession();
  closeTransport();
  pushEvent<ConnectEvent>(false);
}

void PTPCamera::startEventThread() {
#if defined(ESP32)
  esp_pthread_cfg_t cfg = esp_pthread_get_default_config();
  cfg.stack_size = (4096);
  esp_pthread_set_cfg(&cfg);
#endif

  eventThread = std::jthread([this](std::stop_token stoken) {
    while (!stoken.stop_requested()) {
      try {
        if (!isTransportOpen()) {
          pushEvent<ConnectEvent>(false);
          break;
        }
        getEvents();
      } catch (Exception& e) {
        pushEvent(std::make_unique<ExceptionEvent>(e));
      }

      // TODO: Make this adjustable
      std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
  });
}

void PTPCamera::stopEventThread() {
  if (eventThread.joinable()) {
    eventThread.request_stop();
    eventThread.join();
  }
}

std::shared_ptr<DeviceInfo> PTPCamera::getCachedDI() {
  if (!cachedDI)
    cachedDI = getDeviceInfo();
  return cachedDI;
}

void PTPCamera::invalidateCachedDI() {
  cachedDI = nullptr;
}

bool PTPCamera::isOpSupported(uint16_t operationCode) {
  return getCachedDI()->isOpSupported(operationCode,
                                      static_cast<uint32_t>(vendorExtensionId));
}

bool PTPCamera::isPropSupported(uint16_t propertyCode) {
  return getCachedDI()->isPropSupported(
      propertyCode, static_cast<uint32_t>(vendorExtensionId));
}