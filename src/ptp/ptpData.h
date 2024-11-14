#ifndef CB_CONTROL_PTP_PTPDATA_H
#define CB_CONTROL_PTP_PTPDATA_H

#include "../packet.h"

struct OperationRequestData {
  const bool dataPhase;
  const bool sending;
  const uint16_t operationCode;
  const uint32_t sessionId;
  const uint32_t transactionId;
  const std::array<uint32_t, 5> params;
  const std::vector<unsigned char> data;

  OperationRequestData(bool dataPhase,
                       bool sending,
                       uint16_t operationCode,
                       uint32_t sessionId,
                       uint32_t transactionId,
                       std::array<uint32_t, 5> params = {},
                       std::vector<unsigned char> data = {})
      : dataPhase(dataPhase),
        sending(sending),
        operationCode(operationCode),
        sessionId(sessionId),
        transactionId(transactionId),
        params(params),
        data(std::move(data)) {}
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

template <std::unsigned_integral T>
class PTPArray : public Packet {
 public:
  uint32_t numElements = 0;
  std::vector<T> array;

  PTPArray() {
    field(this->numElements);
    field(this->array, numElements);
  }
};

class PTPString : public Packet {
 public:
  uint8_t numChars = 0;
  std::string string;

  PTPString() {
    field(this->numChars);
    field(this->string);
  }

  void pack(Buffer& buffer, int& offset) override {
    if (string.length() > PTPString::MAX_CHARS)
      string = string.substr(0, PTPString::MAX_CHARS);
    numChars = string.length() + 1;
    Packet::pack(buffer, offset);
  };

  void unpack(Buffer& buffer, int& offset) override {
    // Deal with "empty" string, which consists of a single 0x00 byte
    if (buffer[offset] == 0x00) {
      numChars = 0;
      string.clear();
      offset++;
      return;
    }
    Packet::unpack(buffer, offset);
  };

 private:
  // PTP strings are limited to 255 characters (including null terminator)
  static const int MAX_CHARS = 254;
};

class DeviceInfo : public Packet {
 public:
  uint16_t standardVersion = 0;
  uint32_t vendorExtensionId = 0;
  uint16_t vendorExtensionVersion = 0;
  PTPString vendorExtensionDesc;
  uint16_t functionalMode = 0;
  PTPArray<uint16_t> operationsSupported;
  PTPArray<uint16_t> eventsSupported;
  PTPArray<uint16_t> devicePropertiesSupported;
  PTPArray<uint16_t> captureFormats;
  PTPArray<uint16_t> imageFormats;
  PTPString manufacturer;
  PTPString model;
  PTPString deviceVersion;
  PTPString serialNumber;

  DeviceInfo() {
    field(this->standardVersion);
    field(this->vendorExtensionId);
    field(this->vendorExtensionVersion);
    field(this->vendorExtensionDesc);
    field(this->functionalMode);
    field(this->operationsSupported);
    field(this->eventsSupported);
    field(this->devicePropertiesSupported);
    field(this->captureFormats);
    field(this->imageFormats);
    field(this->manufacturer);
    field(this->model);
    field(this->deviceVersion);
    field(this->serialNumber);
  }
};

/* PTP Enums */
// TODO: Figure out a cleaner way to do this? An OperationCode should be easily
// comparable against uint16_t and passed as uint16_t to functions because
// different PTPs will have vendor-specific operation codes

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