#ifndef CB_CONTROL_PTP_PTPDATA_H
#define CB_CONTROL_PTP_PTPDATA_H

#include "../packet.h"

#include <exception>
#include <map>
#include <typeindex>
#include <typeinfo>

// TODO: Figure out where to catch and deal with exceptions (probably within
// PTP class)

class PTPException : public std::exception {
 public:
  template <typename... Args>
  PTPException(const char* format, Args... args) {
    snprintf(msg, sizeof(msg), format, args...);
  }

  virtual const char* what() const noexcept override { return msg; }

 private:
  char msg[256];
};

class PTPOperationException : public PTPException {
 public:
  const int errorCode;

  PTPOperationException(uint32_t responseCode)
      : PTPException("PTP Operation Error: 0x%04x", responseCode),
        errorCode(responseCode) {}
};

class PTPTransportException : public PTPException {
 public:
  template <typename... Args>
  PTPTransportException(const char* format, Args... args)
      : PTPException(format, args...) {}
};

class PTPCameraException : public PTPException {
 public:
  template <typename... Args>
  PTPCameraException(const char* format, Args... args)
      : PTPException(format, args...) {}
};

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

template <std::integral T>
class PTPArray : public Packet {
 public:
  uint32_t numElements = 0;
  std::vector<T>& array;

  PTPArray(std::vector<T>& array) : array(array) {
    field(this->numElements);
    field(this->array, numElements);
  }
};

class PTPString : public Packet {
 public:
  uint8_t numChars = 0;
  std::string& string;

  PTPString(std::string& string) : string(string) {
    field(this->numChars);
    field(this->string);
  }

  void pack(Buffer& buffer, int& offset) override;
  void unpack(Buffer& buffer,
              int& offset,
              std::optional<int> limitOffset) override;

 private:
  // PTP strings are limited to 255 characters (including null terminator)
  static const int MAX_CHARS = 254;
};

class PTPPacket : public Packet {
 public:
  using Packet::field;

  template <std::integral T>
  void field(std::vector<T>& values) {
    fields.push_back(std::make_unique<PTPArray<T>>(values));
  }

  void field(std::string& value) {
    fields.push_back(std::make_unique<PTPString>(value));
  }
};

class DeviceInfo : public PTPPacket {
 public:
  uint16_t standardVersion = 0;
  uint32_t vendorExtensionId = 0;
  uint16_t vendorExtensionVersion = 0;
  std::string vendorExtensionDesc;
  uint16_t functionalMode = 0;
  std::vector<uint16_t> operationsSupported;
  std::vector<uint16_t> eventsSupported;
  std::vector<uint16_t> devicePropertiesSupported;
  std::vector<uint16_t> captureFormats;
  std::vector<uint16_t> imageFormats;
  std::string manufacturer;
  std::string model;
  std::string deviceVersion;
  std::string serialNumber;

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

  bool isOpSupported(uint16_t operationCode, uint32_t vendorExtensionId = 0);
};

extern const std::unordered_map<std::type_index, uint16_t> DataTypeMap;

template <typename T>
class DevicePropDesc : public PTPPacket {
 public:
  uint16_t devicePropertyCode = 0;
  uint16_t dataType = 0;
  uint8_t getSet = 0;
  T factoryDefaultValue;
  T currentValue;
  Buffer form;

  DevicePropDesc() {
    if (!DataTypeMap.contains(typeid(T))) {
      // TODO: Specific exception type
      throw PTPException("Unsupported DevicePropValue type.");
    }
    dataType = DataTypeMap.at(typeid(T));

    field(this->devicePropertyCode);
    typeField(this->dataType);
    field(this->getSet);
    field(this->factoryDefaultValue);
    field(this->currentValue);
    Packet::field(this->form);
  }
};

class PropDescForm : public Packet {
 public:
  uint8_t formFlag = 0;

  PropDescForm(uint8_t formFlag = 0) : formFlag(formFlag) {
    typeField(this->formFlag);
  }
};

template <std::integral T>
class PropDescRange : public PropDescForm {
 public:
  T minimumValue = 0;
  T maximumValue = 0;
  T stepSize = 0;

  PropDescRange() : PropDescForm(0x01) {
    field(this->minimumValue);
    field(this->maximumValue);
    field(this->stepSize);
  }
};

// TODO: Will this only ever be integral? Or should it stay like this to support
// other types?
template <typename T>
class PropDescEnum : public PropDescForm {
 public:
  uint16_t numValues = 0;
  std::vector<T> supportedValues;

  PropDescEnum() : PropDescForm(0x02) {
    field(this->numValues);
    field(this->supportedValues, numValues);
  }
};

/* PTP Enums */
// TODO: Figure out a cleaner way to do this? An OperationCode should be easily
// comparable against uint16_t and passed as uint16_t to functions because
// different PTPs will have vendor-specific operation codes

namespace DataType {
enum DataType : uint16_t {
  UNDEF = 0x0000,
  INT8 = 0x0001,
  UINT8 = 0x0002,
  INT16 = 0x0003,
  UINT16 = 0x0004,
  INT32 = 0x0005,
  UINT32 = 0x0006,
  INT64 = 0x0007,
  UINT64 = 0x0008,
  INT128 = 0x0009,
  UINT128 = 0x000A,
  AINT8 = 0x4001,
  AUINT8 = 0x4002,
  AINT16 = 0x4003,
  AUINT16 = 0x4004,
  AINT32 = 0x4005,
  AUINT32 = 0x4006,
  AINT64 = 0x4007,
  AUINT64 = 0x4008,
  AINT128 = 0x4009,
  AUINT128 = 0x400A,
  STR = 0xFFFF,
};
}

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

enum class VendorExtensionId : uint32_t {
  EastmanKodak = 0x00000001,
  SeikoEpson = 0x00000002,
  Agilent = 0x00000003,
  Polaroid = 0x00000004,
  AgfaGevaert = 0x00000005,
  Microsoft = 0x00000006,
  Equinox = 0x00000007,
  Viewquest = 0x00000008,
  STMicroelectronics = 0x00000009,
  Nikon = 0x0000000A,
  Canon = 0x0000000B,
  FotoNation = 0x0000000C,
  PENTAX = 0x0000000D,
  Fuji = 0x0000000E,
  Sony = 0x00000011,
  NDD = 0x00000012,
  Samsung = 0x0000001A,
  Parrot = 0x0000001B,
  Panasonic = 0x0000001C,
};

#endif