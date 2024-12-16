#ifndef CB_CONTROL_PTP_NIKONDATA_H
#define CB_CONTROL_PTP_NIKONDATA_H

#include "../packet.h"

/* Nikon vendor PTP Enums */

namespace NikonOperationCode {
enum NikonOperationCode : uint16_t {
  GetProfileAllData = 0x9006,
  SendProfileData = 0x9007,
  DeleteProfile = 0x9008,
  SetProfileData = 0x9009,
  AdvancedTransfer = 0x9010,
  GetFileInfoInBlock = 0x9011,
  Capture = 0x90C0,
  AFDrive = 0x90C1,
  SetControlMode = 0x90C2,
  DelImageSDRAM = 0x90C3,
  GetLargeThumb = 0x90C4,
  CurveDownload = 0x90C5,
  CurveUpload = 0x90C6,
  CheckEvents = 0x90C7,
  DeviceReady = 0x90C8,
  SetPreWBData = 0x90C9,
  GetVendorPropCodes = 0x90CA,
  AFCaptureSDRAM = 0x90CB,
  GetPictCtrlData = 0x90CC,
  SetPictCtrlData = 0x90CD,
  DelCstPicCtrl = 0x90CE,
  GetPicCtrlCapability = 0x90CF,
  GetPreviewImg = 0x9200,
  StartLiveView = 0x9201,
  EndLiveView = 0x9202,
  GetLiveViewImg = 0x9203,
  MfDrive = 0x9204,
  ChangeAFArea = 0x9205,
  AFDriveCancel = 0x9206,
  InitiateCaptureRecInMedia = 0x9207,
  GetVendorStorageIDs = 0x9209,
  StartMovieRecInCard = 0x920A,
  EndMovieRec = 0x920B,
  TerminateCapture = 0x920C,
  GetDevicePTPIPInfo = 0x90E0,
  GetPartialObjectHiSpeed = 0x9400,
  GetDevicePropEx = 0x9504,
};
}

#endif