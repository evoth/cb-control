#ifndef CB_CONTROL_PTP_PTP_H
#define CB_CONTROL_PTP_PTP_H

#include <array>
#include <cstdint>
#include <exception>
#include <format>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// TODO: Figure out where to catch and deal with exceptions (probably within
// PTPExtension class)

class PTPOperationException : public std::exception {
 public:
  const int errorCode;

  PTPOperationException(uint32_t responseCode)
      : errorCode(responseCode),
        message(
            std::format("PTP Operation Error: {:#04x}", responseCode).c_str()) {
  }

  const char* what() const noexcept override { return message; }

 private:
  const char* message;
};

class PTPTransportException : public std::exception {
 public:
  PTPTransportException(const char* message)
      : message(std::format("PTP Transport Error: {}", message).c_str()) {}

  const char* what() const noexcept override { return message; }

 private:
  const char* message;
};

struct EventData {
  const uint16_t eventCode;
  const std::array<uint32_t, 3> params;

  EventData(uint16_t eventCode, std::array<uint32_t, 3> params)
      : eventCode(eventCode), params(params) {}
};

struct OperationRequestData {
  const uint16_t operationCode;
  const std::array<uint32_t, 5> params;
  const std::vector<unsigned char> data;

  OperationRequestData(uint16_t operationCode,
                       std::array<uint32_t, 5> params = {},
                       std::vector<unsigned char> data = {})
      : operationCode(operationCode), params(params), data(std::move(data)) {}
};

struct OperationResponseData {
  const uint16_t responseCode;
  const std::array<uint32_t, 5> params;
  const std::vector<unsigned char> data;
  // const std::vector<std::unique_ptr<EventData>> events;

  OperationResponseData(uint16_t responseCode,
                        std::array<uint32_t, 5> params = {},
                        std::vector<unsigned char> data = {})
      : responseCode(responseCode), params(params), data(std::move(data)) {}
};

class PTPTransport {
 public:
  // Constructor should open transport
  // Destructor should close transport
  // TODO: What to do if transport closes early?
  // TODO: Deal with events (event queue, pollEvents(), etc.)

  virtual bool isOpen() = 0;

  virtual OperationResponseData send(const OperationRequestData& request,
                                     uint32_t sessionId,
                                     uint32_t transactionId) = 0;
  virtual OperationResponseData recv(const OperationRequestData& request,
                                     uint32_t sessionId,
                                     uint32_t transactionId) = 0;
  virtual OperationResponseData mesg(const OperationRequestData& request,
                                     uint32_t sessionId,
                                     uint32_t transactionId) = 0;
};

class PTPExtension {
 public:
  PTPExtension(std::unique_ptr<PTPTransport> transport)
      : transport(std::move(transport)) {
    if (!transport)
      throw PTPTransportException("No transport provided.");
  }

  ~PTPExtension() {
    try {
      closeSession();
    } catch (const std::exception& e) {
      // TODO: Handle exception
    }
  }

  virtual void openSession();
  virtual void closeSession();

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

/* PTP Enums */
// TODO: Figure out a cleaner way to do this? An OperationCode should be easily
// comparable against uint16_t and passed as uint16_t to functions because
// different PTPExtensions will have vendor-specific operation codes

namespace OperationCode {
enum OperationCode : uint16_t {
  Undefined = 0x1000,
  GetDeviceInfo = 0x1001,
  OpenSession = 0x1002,
  CloseSession = 0x1003,
  GetStorageIDs = 0x1004,
  GetStorageInfo = 0x1005,
  GetNumObjects = 0x1006,
  GetObjectHandles = 0x1007,
  GetObjectInfo = 0x1008,
  GetObject = 0x1009,
  GetThumb = 0x100A,
  DeleteObject = 0x100B,
  SendObjectInfo = 0x100C,
  SendObject = 0x100D,
  InitiateCapture = 0x100E,
  FormatStore = 0x100F,
  ResetDevice = 0x1010,
  SelfTest = 0x1011,
  SetObjectProtection = 0x1012,
  PowerDown = 0x1013,
  GetDevicePropDesc = 0x1014,
  GetDevicePropValue = 0x1015,
  SetDevicePropValue = 0x1016,
  ResetDevicePropValue = 0x1017,
  TerminateOpenCapture = 0x1018,
  MoveObject = 0x1019,
  CopyObject = 0x101A,
  GetPartialObject = 0x101B,
  InitiateOpenCapture = 0x101C,
  StartEnumHandles = 0x101D,
  EnumHandles = 0x101E,
  StopEnumHandles = 0x101F,
  GetVendorExtensionMaps = 0x1020,
  GetVendorDeviceInfo = 0x1021,
  GetResizedImageObject = 0x1022,
  GetFilesystemManifest = 0x1023,
  GetStreamInfo = 0x1024,
  GetStream = 0x1025,
};
}

namespace ResponseCode {
enum ResponseCode : uint16_t {
  Undefined = 0x2000,
  OK = 0x2001,
  GeneralError = 0x2002,
  SessionNotOpen = 0x2003,
  InvalidTransactionID = 0x2004,
  OperationNotSupported = 0x2005,
  ParameterNotSupported = 0x2006,
  IncompleteTransfer = 0x2007,
  InvalidStorageId = 0x2008,
  InvalidObjectHandle = 0x2009,
  DevicePropNotSupported = 0x200A,
  InvalidObjectFormatCode = 0x200B,
  StoreFull = 0x200C,
  ObjectWriteProtected = 0x200D,
  StoreReadOnly = 0x200E,
  AccessDenied = 0x200F,
  NoThumbnailPresent = 0x2010,
  SelfTestFailed = 0x2011,
  PartialDeletion = 0x2012,
  StoreNotAvailable = 0x2013,
  SpecificationByFormatUnsupported = 0x2014,
  NoValidObjectInfo = 0x2015,
  InvalidCodeFormat = 0x2016,
  UnknownVendorCode = 0x2017,
  CaptureAlreadyTerminated = 0x2018,
  DeviceBusy = 0x2019,
  InvalidParentObject = 0x201A,
  InvalidDevicePropFormat = 0x201B,
  InvalidDevicePropValue = 0x201C,
  InvalidParameter = 0x201D,
  SessionAlreadyOpened = 0x201E,
  TransactionCanceled = 0x201F,
  SpecificationOfDestinationUnsupported = 0x2020,
  InvalidEnumHandle = 0x2021,
  NoStreamEnabled = 0x2022,
  InvalidDataset = 0x2023,
};
}

#endif