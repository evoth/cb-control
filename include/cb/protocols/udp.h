#ifndef CB_CONTROL_UDP_H
#define CB_CONTROL_UDP_H

#include <cb/socket.h>

namespace cb {

class UDPMulticastSocket : public virtual Socket {
 public:
  virtual bool begin(const std::string& ip,
                     int port) = 0;  // Should not throw exceptions
  virtual std::string getRemoteIp() const = 0;
  virtual int getRemotePort() const = 0;
};

}  // namespace cb

#endif