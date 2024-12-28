#ifndef CB_CONTROL_PTP_PTP_H
#define CB_CONTROL_PTP_PTP_H

#include "../camera.h"
#include "ptpData.h"

#include <mutex>
#include <thread>
#include <utility>

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
        isSessionOpen(std::exchange(o.isSessionOpen, false)),
        sessionId(std::exchange(o.sessionId, 0)),
        transactionId(std::exchange(o.transactionId, 0)) {};

  virtual ~PTP() {
    try {
      closeSession();
    } catch (const std::exception& e) {
      // TODO: Handle exception
    }
  }

  virtual void openTransport();
  virtual void closeTransport();
  bool isTransportOpen();

  virtual void openSession();
  virtual void closeSession();

  virtual std::unique_ptr<DeviceInfo> getDeviceInfo();

  template <typename T>
  std::unique_ptr<DevicePropDesc<T>> getDevicePropDesc(
      uint32_t devicePropCode) {
    Buffer data = recv(OperationCode::GetDevicePropDesc, {devicePropCode}).data;
    if (auto devicePropDesc =
            Packet::unpackAs<DevicePropDesc<T>, DevicePropDesc<T>>(data))
      return devicePropDesc;
    return Exception(ExceptionContext::PTPDevicePropDesc,
                     ExceptionType::WrongType);
  }

 protected:
  std::unique_ptr<PTPTransport> transport;
  bool isSessionOpen = false;

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
  std::mutex transactionMutex;

  uint32_t getSessionId() { return isSessionOpen ? sessionId : 0; }
  uint32_t getTransactionId() { return isSessionOpen ? transactionId++ : 0; }
};

class PTPCamera : protected PTP, public Camera {
 public:
  PTPCamera(PTP&& ptp, VendorExtensionId vendorExtensionId)
      : PTP(std::move(ptp)), vendorExtensionId(vendorExtensionId) {}

  void connect() override;
  void disconnect() override;
  bool isConnected() override;

 protected:
  const VendorExtensionId vendorExtensionId;

  void closeTransport() override;

  virtual void checkEvents() = 0;
  void startEventThread();
  void stopEventThread();

  std::shared_ptr<DeviceInfo> getCachedDI();
  void invalidateCachedDI();
  bool isOpSupported(uint16_t operationCode);
  bool isPropSupported(uint16_t propertyCode);

 private:
  std::jthread eventThread;
  std::shared_ptr<DeviceInfo> cachedDI;
};

#endif