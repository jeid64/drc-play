#include "cmd.h"

#include <cstdio>

CmdProtocol::CmdProtocol() {
}

CmdProtocol::~CmdProtocol() {
}

void CmdProtocol::HandlePacket(const uint8_t* data, int len) {
    // Just log commands sent from drh to drc
    if (len < 8) {
        return;
    }

    // Check query type
    if (data[2] == 0) {
        if (len < 20) {
            return;
        }
        printf("Received command 0 %d/%d\n", data[14], data[15]);
    } else {
        printf("Received command %d\n", data[2]);
    }
}
