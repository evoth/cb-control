#ifndef CB_CONTROL_PTP_PTP_H
#define CB_CONTROL_PTP_PTP_H

#include <array>
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

// TODO: Move stuff into implementation file

// TODO: Figure out elegant way to include name of error in message without
// having to duplicate definitions?
class PTPOperationException : public std::exception {
 public:
  PTPOperationException(uint32_t responseCode);

  const char* what() const noexcept override { return message.c_str(); }

 private:
  std::string message;
};

struct OperationRequestData {
  uint32_t operationCode = 0;
  std::array<uint32_t, 5> params = {0};
  std::vector<unsigned char> data;

  OperationRequestData(uint32_t operationCode,
                       std::array<uint32_t, 5> params = {},
                       std::vector<unsigned char> data = {})
      : operationCode(operationCode), params(params), data(data) {}
};

struct OperationResponseData {
  uint32_t responseCode = 0;
  std::array<uint32_t, 5> params = {0};
  std::vector<unsigned char> data;

  OperationResponseData(uint32_t responseCode,
                        std::array<uint32_t, 5> params,
                        std::vector<unsigned char> data)
      : responseCode(responseCode), params(params), data(data) {}
};

class PTPTransport {
 public:
  virtual OperationResponseData send(OperationRequestData request) = 0;
  virtual OperationResponseData recv(OperationRequestData request) = 0;
  virtual OperationResponseData mesg(OperationRequestData request) = 0;
  virtual void openTransport() = 0;
  virtual void closeTransport() = 0;

  ~PTPTransport() { closeTransport(); }
};

class PTPExtension {
 public:
  virtual void openSession() {
    // closeTransport()?
    transport.openTransport();
    // sessionId++;
    // transactionId = 1;
    // OperationRequestData request(OperationCode::OpenSession, {sessionId});
    // OperationResponseData response = transport.mesg(request);
  }
  virtual void closeSession() { transport.closeTransport(); }

  ~PTPExtension() { closeSession(); }

 protected:
  // How does initialization/ownership work in this case?
  PTPTransport& transport;
  // uint32_t sessionId = 0;
  // uint32_t transactionId = 0;
  // bool isSessionOpen = false;
};

#endif