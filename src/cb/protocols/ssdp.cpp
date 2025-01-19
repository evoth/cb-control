#include <cb/protocols/ssdp.h>

#include <cb/protocols/xml.h>
#include <cb/proxy.h>

namespace cb {

std::unique_ptr<CameraProxy> SSDPDiscovery::createCamera(
    std::unique_ptr<DiscoveryAddEvent> addEvent) {
  return std::make_unique<CameraWrapper>(clientGuid, clientName,
                                         addEvent->connectionAddress);
}

std::unique_ptr<EventContainer> SSDPDiscovery::popEvent() {
  getEvents();
  return EventProxy<EventContainer>::popEvent();
}

void SSDPDiscovery::pushAndReceive(std::string containerId,
                                   std::unique_ptr<EventPacket> event) {
  pushEvent<EventContainer>(containerId, std::vector<Buffer>{event->pack()});
  receiveEvent(std::make_unique<EventContainer>(
      containerId, std::vector<Buffer>{event->pack()}));
}

void SSDPDiscovery::getEvents() {
  HTTPRequest request;
  while (request.recv(*udpSocket, 0)) {
    if (request.method != "NOTIFY")
      continue;

    std::string ip = udpSocket->getRemoteIp();
    std::string serviceName = request.headers["NT"];
    if (!searchTargets.contains(serviceName))
      continue;

    // Remove camera on ssdp:byebye
    if (request.headers["NTS"] == "ssdp:byebye") {
      pushAndReceive(createId(ip), std::make_unique<DiscoveryRemoveEvent>());
      advertisements.erase(serviceName);
      continue;
    } else if (request.headers["NTS"] != "ssdp:alive") {
      continue;
    }

    if (!advertisements.contains(serviceName)) {
      // New advertisement; request/parse DeviceDesc
      auto xmlResponse = URL(request.headers["Location"]).request(tcpSocket);
      XMLDoc deviceDesc;
      deviceDesc.unpack(xmlResponse->body);
      XMLElement& device = deviceDesc["device"];

      // Push DiscoveryAddEvent
      auto addEvent = std::make_unique<DiscoveryAddEvent>(
          static_cast<int>(DiscoveryMethod::SSDP), ip, device["serialNumber"],
          device["manufacturer"], device["modelName"], device["friendlyName"]);
      pushAndReceive(createId(ip), std::move(addEvent));
    }

    // Keep track of time and IP of advertisement
    advertisements[serviceName] = {std::chrono::steady_clock::now(), ip};

    // Hacky way to get Cache-Control seconds value
    std::string durationStr = "";
    size_t durationStart = request.headers["Cache-Control"].find("=");
    if (durationStart != std::string::npos)
      durationStr = request.headers["Cache-Control"].substr(durationStart + 1);

    // Add max seconds value to expiration time
    if (!durationStr.empty()) {
      advertisements[serviceName].expirationTime +=
          std::chrono::seconds(std::stoi(durationStr));
    }
  }

  // Remove expired advertisements
  auto now = std::chrono::steady_clock::now();
  for (auto it = advertisements.begin(); it != advertisements.end();) {
    if (it->second.expirationTime < now) {
      pushAndReceive(createId(it->second.ip),
                     std::make_unique<DiscoveryRemoveEvent>());
      it = advertisements.erase(it);
    } else {
      ++it;
    }
  }
}

}  // namespace cb