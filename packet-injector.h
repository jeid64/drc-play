#ifndef __DRC_CAP_PACKET_INJECTOR_H_
#define __DRC_CAP_PACKET_INJECTOR_H_

#include <cstdint>

class PacketInjector {
  public:
    enum Direction {
        STA_TO_AP,
        AP_TO_STA
    };

    virtual void InjectPacket(Direction dir, const uint8_t* ip_pkt,
                              int len) = 0;
};

#endif
