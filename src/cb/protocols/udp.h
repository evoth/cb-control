#ifndef CB_CONTROL_UDP_H
#define CB_CONTROL_UDP_H

#include <cb/socket.h>

namespace cb {

// TODO: Automatically listen on all interfaces
// TODO: Send from automatically assigned IP?
// TODO: Unicast (make separate interface or provide multicast flag or even a
// list of multicast groups to join?)
class UDPSocket : public virtual Socket {
 public:
  virtual bool begin(
      const std::string& ip,
      int port,
      bool multicast = false) = 0;  // Should not throw exceptions
  virtual std::string getPacketIp() const = 0;
  virtual int getPacketPort() const = 0;
};

}  // namespace cb

#endif