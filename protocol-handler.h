#ifndef __DRC_CAP_PROTOCOL_HANDLER_H_
#define __DRC_CAP_PROTOCOL_HANDLER_H_

#include <cstdint>
#include <vector>

class ProtocolHandler {
  public:
    virtual void HandlePacket(const std::vector<uint8_t>& data) = 0;
};

#endif
