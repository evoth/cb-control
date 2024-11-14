#ifndef CB_CONTROL_PTP_PTP_H
#define CB_CONTROL_PTP_PTP_H

#include "../camera.h"
#include "../logger.h"
#include "ptpData.h"

#include <array>
#include <cstdint>
#include <exception>
// #include <format>
#include <iomanip>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

// TODO: Figure out where to catch and deal with exceptions (probably within
// PTP class)

// TODO: Split into multiple headers

class PTPOperationException : public std::exception {
 public:
  const int errorCode;

  PTPOperationException(uint32_t responseCode) : errorCode(responseCode) {
    snprintf(msg, sizeof(msg), "PTP Operation Error: 0x%04x", responseCode);
  }

  const char* what() const noexcept override { return msg; }

 private:
  char msg[32];
};

class PTPTransportException : public std::exception {
 public:
  template <typename... Args>
  PTPTransportException(const char* format, Args... args) {
    snprintf(msg, sizeof(msg), format, args...);
  }

  const char* what() const noexcept override { return msg; }

 private:
  char msg[256];
};

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
  DeviceInfo getDeviceInfo();

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
  PTPCamera(std::unique_ptr<PTPTransport> transport)
      : PTP(std::move(transport)) {}

  void connect() override {
    openTransport();
    openSession();
  }

  void disconnect() override {
    closeSession();
    closeTransport();
  }

  void getDeviceInfo() override {
    DeviceInfo deviceInfo = PTP::getDeviceInfo();
    Logger::log("VendorExtensionID: 0x%04x", deviceInfo.vendorExtensionId);
    Logger::log("VendorExtensionVersion: 0x%02x",
                deviceInfo.vendorExtensionVersion);
    Logger::log("VendorExtensionDesc: %s",
                deviceInfo.vendorExtensionDesc.string.c_str());
    Logger::log("Manufacturer: %s", deviceInfo.manufacturer.string.c_str());
    Logger::log("Model: %s", deviceInfo.model.string.c_str());
    Logger::log("DeviceVersion: %s", deviceInfo.deviceVersion.string.c_str());
    Logger::log("SerialNumber: %s", deviceInfo.serialNumber.string.c_str());
  }
};

#endif