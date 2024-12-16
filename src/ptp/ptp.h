#ifndef CB_CONTROL_PTP_PTP_H
#define CB_CONTROL_PTP_PTP_H

#include "../camera.h"
#include "../logger.h"
#include "ptpData.h"

#include <mutex>
#include <utility>

struct EventData {
  const uint16_t eventCode;
  const std::array<uint32_t, 3> params;

  EventData(uint16_t eventCode, std::array<uint32_t, 3> params)
      : eventCode(eventCode), params(params) {}
};

class PTPTransport {
 public:
  // Destructor should close
  virtual ~PTPTransport() = default;
  // TODO: Deal with events (event queue, pollEvents(), etc.)

  virtual void open() = 0;
  virtual void close() = 0;
  virtual bool isOpen() = 0;

  virtual OperationResponseData transaction(
      const OperationRequestData& request) = 0;
};

class PTP {
 public:
  PTP(std::unique_ptr<PTPTransport> transport)
      : transport(std::move(transport)) {}
  PTP(PTP&& o) noexcept
      : transport(std::move(o.transport)),
        sessionId(std::exchange(o.sessionId, 0)),
        transactionId(std::exchange(o.transactionId, 0)),
        isSessionOpen(std::exchange(o.isSessionOpen, false)) {};

  virtual ~PTP() {
    Logger::log("PTP destructed");
    try {
      closeSession();
    } catch (const std::exception& e) {
      // TODO: Handle exception
    }
  }

  void openTransport();
  void closeTransport();
  bool isTransportOpen();

  virtual void openSession();
  virtual void closeSession();
  virtual DeviceInfo getDeviceInfo();

  template <typename T>
  std::unique_ptr<DevicePropDesc<T>> getDevicePropDesc(
      uint32_t devicePropCode) {
    Buffer data = recv(OperationCode::GetDevicePropDesc, {devicePropCode}).data;
    if (auto devicePropDesc =
            Packet::unpackAs<DevicePropDesc<T>, DevicePropDesc<T>>(data))
      return devicePropDesc;
    throw PTPException(
        "Unable to unpack DevicePropDesc; wrong data type provided.");
  }

 protected:
  std::unique_ptr<PTPTransport> transport;

  OperationResponseData send(uint16_t operationCode,
                             std::array<uint32_t, 5> params = {},
                             std::vector<unsigned char> data = {});
  OperationResponseData recv(uint16_t operationCode,
                             std::array<uint32_t, 5> params = {});
  OperationResponseData mesg(uint16_t operationCode,
                             std::array<uint32_t, 5> params = {});

 private:
  uint32_t sessionId = 0;
  uint32_t transactionId = 0;
  bool isSessionOpen = false;
  std::mutex transactionMutex;

  uint32_t getSessionId() { return isSessionOpen ? sessionId : 0; }
  uint32_t getTransactionId() { return isSessionOpen ? transactionId++ : 0; }
};

class PTPCamera : protected PTP, public Camera {
 public:
  PTPCamera(PTP&& ptp, VendorExtensionId vendorExtensionId)
      : PTP(std::move(ptp)), vendorExtensionId(vendorExtensionId) {}

  void connect() override {
    openTransport();
    openSession();
  }

  void disconnect() override {
    closeSession();
    closeTransport();
  }

 protected:
  const VendorExtensionId vendorExtensionId;

  std::shared_ptr<DeviceInfo> getCachedDI() {
    if (!cachedDI)
      cachedDI = std::make_shared<DeviceInfo>(getDeviceInfo());
    return cachedDI;
  }

  void invalidateCachedDI() { cachedDI = nullptr; }

  bool isOpSupported(uint16_t operationCode) {
    return getCachedDI()->isOpSupported(
        operationCode, static_cast<uint32_t>(vendorExtensionId));
  }

 private:
  std::shared_ptr<DeviceInfo> cachedDI;
};

#endif