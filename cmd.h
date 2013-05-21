#ifndef __DRC_CAP_CMD_H_
#define __DRC_CAP_CMD_H_

#include "protocol-handler.h"

const uint16_t kDrcCmdPort = 50123;

class CmdProtocol : public ProtocolHandler {
  public:
    CmdProtocol();
    virtual ~CmdProtocol();

    void HandlePacket(const uint8_t* data, int len);
};

#endif
