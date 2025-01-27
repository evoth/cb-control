#ifndef CB_CONTROL_SSDP_H
#define CB_CONTROL_SSDP_H

#include <cb/discovery.h>
#include <cb/logger.h>
#include <cb/protocols/http.h>

#include <chrono>
#include <set>

namespace cb {

class SSDPSearchMessage : public HTTPRequest {
 public:
  SSDPSearchMessage(std::string searchTarget, uint8_t maxWaitSeconds = 3)
      : HTTPRequest("M-SEARCH", "*") {
    headers["Host"] = "239.255.255.250:1900";
    headers["Man"] = "\"ssdp:discover\"";
    headers["MX"] = std::to_string(maxWaitSeconds);
    headers["ST"] = searchTarget;
  }
};

struct SSDPAdvertisementData {
  std::chrono::steady_clock::time_point expirationTime;
  std::string ip;
};

class SSDPDiscovery : public DiscoveryService {
 public:
  SSDPDiscovery(std::unique_ptr<UDPSocket> unicastSocket,
                std::unique_ptr<UDPSocket> multicastSocket,
                std::unique_ptr<TCPSocket> tcpSocket,
                std::set<std::string> searchTargets,
                std::array<uint8_t, 16> clientGuid,
                std::string clientName)
      : DiscoveryService(DiscoveryMethod::SSDP),
        unicastSocket(std::move(unicastSocket)),
        multicastSocket(std::move(multicastSocket)),
        tcpSocket(std::move(tcpSocket)),
        searchTargets(searchTargets),
        clientGuid(clientGuid),
        clientName(clientName) {
    this->unicastSocket->begin("239.255.255.250", 1900, false);
    this->multicastSocket->begin("239.255.255.250", 1900, true);
    for (const std::string& searchTarget : searchTargets) {
      SSDPSearchMessage(searchTarget).send(*this->unicastSocket);
    }
  }

  ~SSDPDiscovery() {
    unicastSocket->close();
    multicastSocket->close();
    tcpSocket->close();
  }

  std::unique_ptr<CameraProxy> createCamera(
      const std::unique_ptr<DiscoveryAddEvent>& addEvent) override;

  std::unique_ptr<EventContainer> popEvent() override;

 protected:
  void getNewEvents() override;

 private:
  std::unique_ptr<UDPSocket> unicastSocket;
  std::unique_ptr<UDPSocket> multicastSocket;
  std::unique_ptr<TCPSocket> tcpSocket;
  std::set<std::string> searchTargets;
  std::array<uint8_t, 16> clientGuid;
  std::string clientName;
  std::map<std::string, SSDPAdvertisementData> advertisements;

  void processAdvertisement(HTTPMessage& message,
                            std::string ip,
                            std::string serviceName);
};

}  // namespace cb

#endif