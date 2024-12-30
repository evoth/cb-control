#ifndef CB_CONTROL_PTP_CANONDATA_H
#define CB_CONTROL_PTP_CANONDATA_H

#include "../camera.h"
#include "../packet.h"
#include "ptp.h"

class EOSDeviceInfo : public PTPPacket {
 public:
  uint32_t length;
  std::vector<uint32_t> eventsSupported;
  std::vector<uint32_t> devicePropertiesSupported;

  EOSDeviceInfo() {
    lengthField(this->length);
    field(this->eventsSupported);
    field(this->devicePropertiesSupported);
    field(this->unknown);
  }

 private:
  // On my camera I got {0x00, 0x01, 0x02, 0x04, 0x08}
  std::vector<uint32_t> unknown;
};

template <std::unsigned_integral T>
class EOSDeviceProp : public PTPPacket {
 public:
  uint32_t length = 0;
  uint32_t devicePropertyCode = 0;
  T value = 0;

  EOSDeviceProp(uint32_t devicePropertyCode = 0, T value = 0)
      : devicePropertyCode(devicePropertyCode), value(value) {
    lengthField(this->length);
    typeField(this->devicePropertyCode);
    field(this->value);
  }
};

class EOSEventPacket : public Packet {
 public:
  uint32_t length = 0;
  uint32_t eventType = 0;

  EOSEventPacket(uint32_t eventType = 0) : eventType(eventType) {
    lengthField(this->length);
    typeField(this->eventType);
  }

  template <typename T>
    requires(std::derived_from<T, EOSEventPacket>)
  static std::unique_ptr<T> unpackAs(const Buffer& buffer) {
    return Packet::unpackAs<EOSEventPacket, T>(buffer);
  }
};

class EOSEventData : public Packet {
 public:
  std::vector<Buffer> events;

  EOSEventData() { field<EOSEventPacket>(this->events); }
};

class EOSPropChanged : public EOSEventPacket {
 public:
  uint32_t propertyCode = 0;
  uint32_t propertyValue = 0;

  EOSPropChanged() : EOSEventPacket(0xc189) {
    field(this->propertyCode);
    field(this->propertyValue);
  }
};

/* Canon vendor PTP Enums */

namespace CanonOperationCode {
enum CanonOperationCode : uint16_t {
  GetObjectSize = 0x9001,
  SetObjectArchive = 0x9002,
  KeepDeviceOn = 0x9003,
  LockDeviceUI = 0x9004,
  UnlockDeviceUI = 0x9005,
  GetObjectHandleByName = 0x9006,
  InitiateReleaseControl = 0x9008,
  TerminateReleaseControl = 0x9009,
  TerminatePlaybackMode = 0x900a,
  ViewfinderOn = 0x900b,
  ViewfinderOff = 0x900c,
  DoAeAfAwb = 0x900d,
  GetCustomizeSpec = 0x900e,
  GetCustomizeItemInfo = 0x900f,
  GetCustomizeData = 0x9010,
  SetCustomizeData = 0x9011,
  GetCaptureStatus = 0x9012,
  CheckEvent = 0x9013,
  FocusLock = 0x9014,
  FocusUnlock = 0x9015,
  GetLocalReleaseParam = 0x9016,
  SetLocalReleaseParam = 0x9017,
  AskAboutPcEvf = 0x9018,
  SendPartialObject = 0x9019,
  InitiateCaptureInMemory = 0x901a,
  GetPartialObjectEx = 0x901b,
  SetObjectTime = 0x901c,
  GetViewfinderImage = 0x901d,
  GetObjectAttributes = 0x901e,
  ChangeUSBProtocol = 0x901f,
  GetChanges = 0x9020,
  GetObjectInfoEx = 0x9021,
  InitiateDirectTransfer = 0x9022,
  TerminateDirectTransfer = 0x9023,
  SendObjectInfoByPath = 0x9024,
  SendObjectByPath = 0x9025,
  InitiateDirectTransferEx = 0x9026,
  GetAncillaryObjectHandles = 0x9027,
  GetTreeInfo = 0x9028,
  GetTreeSize = 0x9029,
  NotifyProgress = 0x902a,
  NotifyCancelAccepted = 0x902b,
  GetDirectory = 0x902d,
  SetPairingInfo = 0x9030,
  GetPairingInfo = 0x9031,
  DeletePairingInfo = 0x9032,
  GetMACAddress = 0x9033,
  SetDisplayMonitor = 0x9034,
  PairingComplete = 0x9035,
  GetWirelessMAXChannel = 0x9036,
  EOSGetStorageIDs = 0x9101,
  EOSGetStorageInfo = 0x9102,
  EOSGetObjectInfo = 0x9103,
  EOSGetObject = 0x9104,
  EOSDeleteObject = 0x9105,
  EOSFormatStore = 0x9106,
  EOSGetPartialObject = 0x9107,
  EOSGetDeviceInfoEx = 0x9108,
  EOSGetObjectInfoEx = 0x9109,
  EOSGetThumbEx = 0x910a,
  EOSSendPartialObject = 0x910b,
  EOSSetObjectAttributes = 0x910c,
  EOSGetObjectTime = 0x910d,
  EOSSetObjectTime = 0x910e,
  EOSRemoteRelease = 0x910f,
  EOSSetDevicePropValueEx = 0x9110,
  EOSGetRemoteMode = 0x9113,
  EOSSetRemoteMode = 0x9114,
  EOSSetEventMode = 0x9115,
  EOSGetEvent = 0x9116,
  EOSTransferComplete = 0x9117,
  EOSCancelTransfer = 0x9118,
  EOSResetTransfer = 0x9119,
  EOSPCHDDCapacity = 0x911a,
  EOSSetUILock = 0x911b,
  EOSResetUILock = 0x911c,
  EOSKeepDeviceOn = 0x911d,
  EOSSetNullPacketMode = 0x911e,
  EOSUpdateFirmware = 0x911f,
  EOSTransferCompleteDT = 0x9120,
  EOSCancelTransferDT = 0x9121,
  EOSSetWftProfile = 0x9122,
  EOSGetWftProfile = 0x9122,
  EOSSetProfileToWft = 0x9124,
  EOSBulbStart = 0x9125,
  EOSBulbEnd = 0x9126,
  EOSRequestDevicePropValue = 0x9127,
  EOSRemoteReleaseOn = 0x9128,
  EOSRemoteReleaseOff = 0x9129,
  EOSInitiateViewfinder = 0x9151,
  EOSTerminateViewfinder = 0x9152,
  EOSGetViewFinderImage = 0x9153,
  EOSDoAf = 0x9154,
  EOSDriveLens = 0x9155,
  EOSDepthOfFieldPreview = 0x9156,
  EOSClickWB = 0x9157,
  EOSZoom = 0x9158,
  EOSZoomPosition = 0x9159,
  EOSSetLiveAfFrame = 0x915a,
  EOSAfCancel = 0x9160,
  EOSFAPIMessageTX = 0x91fe,
  EOSFAPIMessageRX = 0x91ff,
};
}

namespace EOSPropertyCode {
enum EOSPropertyCode : uint32_t {
  Aperture = 0xd101,
  ShutterSpeed = 0xd102,
  ISO = 0xd103,
  ExposureCompensation = 0xd104,
  ShootingMode = 0xd105,
  DriveMode = 0xd106,
  ExposureMeteringMode = 0xd107,
  AutoFocusMode = 0xd108,
  WhiteBalance = 0xd109,
  ColorTemperature = 0xd10a,
  WhiteBalanceAdjustBA = 0xd10b,
  WhiteBalanceAdjustMG = 0xd10c,
  WhiteBalanceBracketBA = 0xd10d,
  WhiteBalanceBracketMG = 0xd10e,
  ColorSpace = 0xd10f,
  PictureStyle = 0xd110,
  BatteryPower = 0xd111,
  BatterySelect = 0xd112,
  CameraTime = 0xd113,
  AutoPowerOff = 0xd114,
  Owner = 0xd115,
  ModelID = 0xd116,
  PTPExtensionVersion = 0xd119,
  DPOFVersion = 0xd11a,
  AvailableShots = 0xd11b,
  CaptureDestination = 0xd11c,
  BracketMode = 0xd11d,
  CurrentStorage = 0xd11e,
  CurrentFolder = 0xd11f,
  ImageFormat = 0xd120,
  ImageFormatCF = 0xd121,
  ImageFormatSD = 0xd122,
  ImageFormatHDD = 0xd123,
  CompressionS = 0xd130,
  CompressionM1 = 0xd131,
  CompressionM2 = 0xd132,
  CompressionL = 0xd133,
  AEModeDial = 0xd138,
  AEModeCustom = 0xd139,
  MirrorUpSetting = 0xd13a,
  HighlightTonePriority = 0xd13b,
  AFSelectFocusArea = 0xd13c,
  HDRSetting = 0xd13d,
  PCWhiteBalance1 = 0xd140,
  PCWhiteBalance2 = 0xd141,
  PCWhiteBalance3 = 0xd142,
  PCWhiteBalance4 = 0xd143,
  PCWhiteBalance5 = 0xd144,
  MWhiteBalance = 0xd145,
  MWhiteBalanceEx = 0xd146,
  PictureStyleStandard = 0xd150,
  PictureStylePortrait = 0xd151,
  PictureStyleLandscape = 0xd152,
  PictureStyleNeutral = 0xd153,
  PictureStyleFaithful = 0xd154,
  PictureStyleBlackWhite = 0xd155,
  PictureStyleAuto = 0xd156,
  PictureStyleUserSet1 = 0xd160,
  PictureStyleUserSet2 = 0xd161,
  PictureStyleUserSet3 = 0xd162,
  PictureStyleParam1 = 0xd170,
  PictureStyleParam2 = 0xd171,
  PictureStyleParam3 = 0xd172,
  HighISOSettingNoiseReduction = 0xd178,
  MovieServoAF = 0xd179,
  ContinuousAFValid = 0xd17a,
  Attenuator = 0xd17b,
  UTCTime = 0xd17c,
  Timezone = 0xd17d,
  Summertime = 0xd17e,
  FlavorLUTParams = 0xd17f,
  CustomFunc1 = 0xd180,
  CustomFunc2 = 0xd181,
  CustomFunc3 = 0xd182,
  CustomFunc4 = 0xd183,
  CustomFunc5 = 0xd184,
  CustomFunc6 = 0xd185,
  CustomFunc7 = 0xd186,
  CustomFunc8 = 0xd187,
  CustomFunc9 = 0xd188,
  CustomFunc10 = 0xd189,
  CustomFunc11 = 0xd18a,
  CustomFunc12 = 0xd18b,
  CustomFunc13 = 0xd18c,
  CustomFunc14 = 0xd18d,
  CustomFunc15 = 0xd18e,
  CustomFunc16 = 0xd18f,
  CustomFunc17 = 0xd190,
  CustomFunc18 = 0xd191,
  CustomFunc19 = 0xd192,
  InnerDevelop = 0xd193,
  MultiAspect = 0xd194,
  MovieSoundRecord = 0xd195,
  MovieRecordVolume = 0xd196,
  WindCut = 0xd197,
  ExtenderType = 0xd198,
  OLCInfoVersion = 0xd199,
  CustomFuncEx = 0xd1a0,
  MyMenu = 0xd1a1,
  MyMenuList = 0xd1a2,
  WftStatus = 0xd1a3,
  WftInputTransmission = 0xd1a4,
  HDDDirectoryStructure = 0xd1a5,
  BatteryInfo = 0xd1a6,
  AdapterInfo = 0xd1a7,
  LensStatus = 0xd1a8,
  QuickReviewTime = 0xd1a9,
  CardExtension = 0xd1aa,
  TempStatus = 0xd1ab,
  ShutterCounter = 0xd1ac,
  SpecialOption = 0xd1ad,
  PhotoStudioMode = 0xd1ae,
  SerialNumber = 0xd1af,
  EVFOutputDevice = 0xd1b0,
  EVFMode = 0xd1b1,
  DepthOfFieldPreview = 0xd1b2,
  EVFSharpness = 0xd1b3,
  EVFWBMode = 0xd1b4,
  EVFClickWBCoeffs = 0xd1b5,
  EVFColorTemp = 0xd1b6,
  ExposureSimMode = 0xd1b7,
  EVFRecordStatus = 0xd1b8,
  LvAfSystem = 0xd1ba,
  MovSize = 0xd1bb,
  LvViewTypeSelect = 0xd1bc,
  MirrorDownStatus = 0xd1bd,
  MovieParam = 0xd1be,
  MirrorLockupState = 0xd1bf,
  FlashChargingState = 0xd1c0,
  AloMode = 0xd1c1,
  FixedMovie = 0xd1c2,
  OneShotRawOn = 0xd1c3,
  ErrorForDisplay = 0xd1c4,
  AEModeMovie = 0xd1c5,
  BuiltinStroboMode = 0xd1c6,
  StroboDispState = 0xd1c7,
  StroboETTL2Metering = 0xd1c8,
  ContinousAFMode = 0xd1c9,
  MovieParam2 = 0xd1ca,
  StroboSettingExpComposition = 0xd1cb,
  MovieParam3 = 0xd1cc,
  LVMedicalRotate = 0xd1cf,
  Artist = 0xd1d0,
  Copyright = 0xd1d1,
  BracketValue = 0xd1d2,
  FocusInfoEx = 0xd1d3,
  DepthOfField = 0xd1d4,
  Brightness = 0xd1d5,
  LensAdjustParams = 0xd1d6,
  EFComp = 0xd1d7,
  LensName = 0xd1d8,
  AEB = 0xd1d9,
  StroboSetting = 0xd1da,
  StroboWirelessSetting = 0xd1db,
  StroboFiring = 0xd1dc,
  LensID = 0xd1dd,
  LCDBrightness = 0xd1de,
  CADarkBright = 0xd1df,
};
}

extern PairMap<CameraProp, uint32_t> EOSProps;

extern std::map<CameraProp, PairMap<uint32_t, CameraPropValue>> EOSPropValues;

#endif