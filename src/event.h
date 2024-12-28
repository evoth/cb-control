#ifndef CB_CONTROL_EVENT_H
#define CB_CONTROL_EVENT_H

#include "packet.h"

class EventPacket : public Packet {
 public:
  uint32_t length = 0;
  uint32_t eventId = 0;

  EventPacket(uint32_t eventId = 0) : eventId(eventId) {
    lengthField(this->length);
    typeField(this->eventId);
  }
};

class ConnectedEvent : public EventPacket {
 public:
  bool isConnected = false;

  ConnectedEvent(bool isConnected = false)
      : EventPacket(0x01), isConnected(isConnected) {
    field(this->isConnected);
  }
};

#endif