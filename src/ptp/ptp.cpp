#include "ptp.h"
#include "ptpEnums.h"

#include <format>

PTPOperationException::PTPOperationException(uint32_t responseCode) {
  message = "PTP Operation Error: ";
  if (OperationCodeMap.contains(responseCode))
    message += OperationCodeMap.at(responseCode);
  else
    message += "[Unknown]";
  message += std::format(" ({:#04x})", responseCode);
}