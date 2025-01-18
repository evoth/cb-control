#ifndef CB_CONTROL_PTP_NIKONDATA_H
#define CB_CONTROL_PTP_NIKONDATA_H

#include <cb/packet.h>

namespace cb {

/* Nikon vendor PTP Enums */

namespace NikonOperationCode {
enum NikonOperationCode : uint16_t {
  GetProfileAllData = 0x9006,
  SendProfileData = 0x9007,
  DeleteProfile = 0x9008,
  SetProfileData = 0x9009,
  AdvancedTransfer = 0x9010,
  GetFileInfoInBlock = 0x9011,
  Capture = 0x90c0,
  AFDrive = 0x90c1,
  SetControlMode = 0x90c2,
  DelImageSDRAM = 0x90c3,
  GetLargeThumb = 0x90c4,
  CurveDownload = 0x90c5,
  CurveUpload = 0x90c6,
  CheckEvents = 0x90c7,
  DeviceReady = 0x90c8,
  SetPreWBData = 0x90c9,
  GetVendorPropCodes = 0x90ca,
  AFCaptureSDRAM = 0x90cb,
  GetPictCtrlData = 0x90cc,
  SetPictCtrlData = 0x90cd,
  DelCstPicCtrl = 0x90ce,
  GetPicCtrlCapability = 0x90cf,
  GetPreviewImg = 0x9200,
  StartLiveView = 0x9201,
  EndLiveView = 0x9202,
  GetLiveViewImg = 0x9203,
  MfDrive = 0x9204,
  ChangeAFArea = 0x9205,
  AFDriveCancel = 0x9206,
  InitiateCaptureRecInMedia = 0x9207,
  GetVendorStorageIDs = 0x9209,
  StartMovieRecInCard = 0x920a,
  EndMovieRec = 0x920b,
  TerminateCapture = 0x920c,
  GetDevicePTPIPInfo = 0x90e0,
  GetPartialObjectHiSpeed = 0x9400,
  GetDevicePropEx = 0x9504,
};
}

}  // namespace cb

#endif