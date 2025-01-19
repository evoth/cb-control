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
  SSDPDiscovery(std::map<std::string, std::unique_ptr<CameraProxy>>& cameras,
                std::unique_ptr<UDPMulticastSocket> udpSocket,
                std::unique_ptr<TCPSocket> tcpSocket,
                std::set<std::string> searchTargets,
                std::array<uint8_t, 16> clientGuid,
                std::string clientName)
      : DiscoveryService(cameras, DiscoveryMethod::SSDP),
        udpSocket(std::move(udpSocket)),
        tcpSocket(std::move(tcpSocket)),
        searchTargets(searchTargets),
        clientGuid(clientGuid),
        clientName(clientName) {
    // TODO: Fix port binding and listen for UDP unicast responses
    this->udpSocket->begin("239.255.255.250", 1900);
    for (const std::string& searchTarget : searchTargets) {
      SSDPSearchMessage(searchTarget).send(*this->udpSocket);
    }
  }

  ~SSDPDiscovery() { udpSocket->close(); }

  std::unique_ptr<CameraProxy> createCamera(
      std::unique_ptr<DiscoveryAddEvent> addEvent) override;

  std::unique_ptr<EventContainer> popEvent() override;

 protected:
  void getEvents() override;

 private:
  std::unique_ptr<UDPMulticastSocket> udpSocket;
  std::unique_ptr<TCPSocket> tcpSocket;
  std::set<std::string> searchTargets;
  std::array<uint8_t, 16> clientGuid;
  std::string clientName;
  std::map<std::string, SSDPAdvertisementData> advertisements;

  void pushAndReceive(std::string containerId,
                      std::unique_ptr<EventPacket> event);
};

}  // namespace cb

#endif