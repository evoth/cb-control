#include <cb/protocols/ssdp.h>

#include <cb/protocols/xml.h>
#include <cb/proxy.h>

namespace cb {

std::unique_ptr<CameraProxy> SSDPDiscovery::createCamera(
    const std::unique_ptr<DiscoveryAddEvent>& addEvent) {
  return std::make_unique<CameraWrapper>(clientGuid, clientName,
                                         addEvent->connectionAddress);
}

std::unique_ptr<DiscoveryEvent> SSDPDiscovery::popEvent() {
  getNewEvents();
  return EventEmitter<DiscoveryEvent>::popEvent();
}

void SSDPDiscovery::getNewEvents() {
  // Listen for NOTIFY messages on multicast socket
  HTTPRequest request;
  while (request.recv(*multicastSocket, 0)) {
    if (request.method != "NOTIFY")
      continue;

    std::string ip = multicastSocket->getPacketIp();
    std::string serviceName = request.headers["NT"];
    if (!searchTargets.contains(serviceName))
      continue;

    // Remove camera on ssdp:byebye
    if (request.headers["NTS"] == "ssdp:byebye") {
      pushEvent<DiscoveryRemoveEvent>(static_cast<uint16_t>(discoveryMethod),
                                      ip);
      advertisements.erase(serviceName);
      continue;
    } else if (request.headers["NTS"] != "ssdp:alive") {
      continue;
    }

    processAdvertisement(request, ip, serviceName);
  }

  // Listen for OK responses on unicast socket
  HTTPResponse response;
  while (response.recv(*unicastSocket, 0)) {
    if (response.statusCode != "200")
      continue;

    std::string ip = unicastSocket->getPacketIp();
    std::string serviceName = response.headers["ST"];
    if (!searchTargets.contains(serviceName))
      continue;

    processAdvertisement(response, ip, serviceName);
  }

  // Remove expired advertisements
  auto now = std::chrono::steady_clock::now();
  for (auto it = advertisements.begin(); it != advertisements.end();) {
    if (it->second.expirationTime < now) {
      pushEvent<DiscoveryRemoveEvent>(static_cast<uint16_t>(discoveryMethod),
                                      it->second.ip);
      it = advertisements.erase(it);
    } else {
      ++it;
    }
  }
}

// TODO: Send DiscoveryAddEvent regardless of whether advertisement is new
// (decide whether to request XML every time or cache it)
void SSDPDiscovery::processAdvertisement(HTTPMessage& message,
                                         std::string ip,
                                         std::string serviceName) {
  if (!advertisements.contains(serviceName)) {
    // New advertisement; request/parse DeviceDesc
    auto xmlResponse = URL(message.headers["Location"]).request(tcpSocket);
    XMLDoc deviceDesc;
    deviceDesc.unpack(xmlResponse->body);
    const XMLElement& device = deviceDesc["device"];

    pushEvent<DiscoveryAddEvent>(static_cast<int>(discoveryMethod), ip,
                                 device["serialNumber"], device["manufacturer"],
                                 device["modelName"], device["friendlyName"]);
  }

  // Keep track of time and IP of advertisement
  advertisements[serviceName] = {std::chrono::steady_clock::now(), ip};

  // Hacky way to get Cache-Control seconds value
  std::string durationStr = "";
  size_t durationStart = message.headers["Cache-Control"].find("=");
  if (durationStart != std::string::npos)
    durationStr = message.headers["Cache-Control"].substr(durationStart + 1);

  // Add max seconds value to expiration time
  if (!durationStr.empty()) {
    advertisements[serviceName].expirationTime +=
        std::chrono::seconds(std::stoi(durationStr));
  }
}

}  // namespace cb