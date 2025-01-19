#ifndef CB_CONTROL_FACTORY_H
#define CB_CONTROL_FACTORY_H

#include <cb/camera.h>
#include <cb/ptp/ip.h>

namespace cb {

template <typename T>
class Factory {
 public:
  virtual ~Factory() = default;

  virtual bool isSupported() const = 0;
  virtual std::unique_ptr<T> create() const = 0;
};

class PTPCameraFactory : public Factory<EventCamera> {
 public:
  PTPCameraFactory(std::unique_ptr<Factory<PTPTransport>> transportFactory)
      : transportFactory(std::move(transportFactory)) {}

  bool isSupported() const override { return transportFactory->isSupported(); }
  std::unique_ptr<EventCamera> create() const override;

 private:
  std::unique_ptr<Factory<PTPTransport>> transportFactory;
};

class PTPIPFactory : public Factory<PTPTransport> {
 public:
  PTPIPFactory(std::array<uint8_t, 16> clientGuid,
               std::string clientName,
               std::string ip,
               int port = 15740)
      : clientGuid(clientGuid), clientName(clientName), ip(ip), port(port) {}

  bool isSupported() const override;
  std::unique_ptr<PTPTransport> create() const override;

 private:
  const std::array<uint8_t, 16> clientGuid;
  const std::string clientName;
  const std::string ip;
  const int port;
};

}  // namespace cb

#endif