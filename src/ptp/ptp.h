#ifndef CB_CONTROL_PTP_PTP_H
#define CB_CONTROL_PTP_PTP_H

#include <array>
#include <cstdint>
#include <exception>
#include <format>
#include <memory>
#include <string>
#include <vector>

// TODO: Figure out where to catch and deal with exceptions

class PTPOperationException : public std::exception {
 public:
  const int errorCode;

  PTPOperationException(uint32_t responseCode)
      : errorCode(responseCode),
        message(std::format("PTP Operation Error: {:#04x}", responseCode)) {}

  const char* what() const noexcept override { return message.c_str(); }

 private:
  const std::string message;
};

class PTPTransportException : public std::exception {
 public:
  PTPTransportException(const char* message)
      : message(std::format("PTP Transport Error: {}", message)) {}

  const char* what() const noexcept override { return message.c_str(); }

 private:
  const std::string message;
};

struct OperationRequestData {
  const uint32_t operationCode;
  const std::array<uint32_t, 5> params;
  const std::vector<unsigned char> data;

  OperationRequestData(uint32_t operationCode,
                       std::array<uint32_t, 5> params = {0},
                       std::vector<unsigned char> data)
      : operationCode(operationCode), params(params), data(data) {}
};

struct OperationResponseData {
  const uint32_t responseCode;
  const std::array<uint32_t, 5> params;
  const std::vector<unsigned char> data;

  OperationResponseData(uint32_t responseCode,
                        std::array<uint32_t, 5> params = {0},
                        std::vector<unsigned char> data)
      : responseCode(responseCode), params(params), data(data) {}
};

class PTPTransport {
 public:
  // Constructor should open transport?
  // Destructor should close transport?
  // TODO: What to do if transport closes early?

  virtual bool isOpen() = 0;

  virtual OperationResponseData send(OperationRequestData& request,
                                     uint32_t sessionId,
                                     uint32_t transactionId) = 0;
  virtual OperationResponseData recv(OperationRequestData& request,
                                     uint32_t sessionId,
                                     uint32_t transactionId) = 0;
  virtual OperationResponseData mesg(OperationRequestData& request,
                                     uint32_t sessionId,
                                     uint32_t transactionId) = 0;
};

class PTPExtension {
 public:
  PTPExtension(std::unique_ptr<PTPTransport> transport)
      : transport(std::move(transport)) {}
  ~PTPExtension() { closeSession(); }

  virtual void openSession();
  virtual void closeSession();

 protected:
  std::unique_ptr<PTPTransport> transport;

  OperationResponseData send(OperationRequestData& request);
  OperationResponseData recv(OperationRequestData& request);
  OperationResponseData mesg(OperationRequestData& request);

 private:
  uint32_t sessionId = 0;
  uint32_t transactionId = 0;
  bool isSessionOpen = false;

  uint32_t getSessionId() { return isSessionOpen ? sessionId : 0; }
  uint32_t getTransactionId() { return isSessionOpen ? transactionId++ : 0; }
  void checkTransport();
};

#endif