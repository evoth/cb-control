#ifndef CB_CONTROL_EXCEPTION_H
#define CB_CONTROL_EXCEPTION_H

#include <cstdio>
#include <exception>

namespace cb {

enum class ExceptionContext {
  Socket,
  CameraConnect,
  PTPIPConnect,
  PTPIPTransaction,
  PTPTransport,
  PTPDevicePropDesc,
  Factory,
  CameraSetProp,
};

enum class ExceptionType {
  SendFailure,
  Receivefailure,
  TimedOut,
  UnsupportedCamera,
  ConnectFailure,
  UnexpectedPacket,
  NotConnected,
  IsNull,
  OperationFailure,
  InitFailure,
  WrongType,
  UnsupportedType,
  WrongDataLength,
  UnsupportedTransport,
  UnsupportedProperty,
  UnsupportedValue,
};

class Exception : public std::exception {
 public:
  Exception(ExceptionContext context, ExceptionType type)
      : context(context), type(type) {
    snprintf(msg, sizeof(msg), "CB Control Exception (context=%d, type=%d)",
             static_cast<int>(context), static_cast<int>(type));
  }

  virtual ~Exception() = default;

  const ExceptionContext context;
  const ExceptionType type;

  virtual const char* what() const noexcept override { return msg; }

 private:
  char msg[256];
};

}  // namespace cb

#endif