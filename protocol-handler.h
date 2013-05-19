#ifndef __DRC_CAP_PROTOCOL_HANDLER_H_
#define __DRC_CAP_PROTOCOL_HANDLER_H_

#include <cstdint>
#include <vector>

class ProtocolHandler {
  public:
    virtual void HandlePacket(const uint8_t* data, int len) = 0;
};

#endif
