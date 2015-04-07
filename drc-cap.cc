// Copyright (c) 2013, Pierre Bourdon, All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
// this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "astrm.h"
#include "cmd.h"
#include "portaudio-handler.h"
#include "sdl-handler.h"
#include "vstrm.h"
#include "udp-sniffer.h"

#include <signal.h>
#include <string>


#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include "vstrm.h"
#include "sdl-handler.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "usage: %s <myip> <repeaterip> \n", argv[0]);
        return 1;
    }
    std::string myip = argv[1];
    std::string repeaterip = argv[2];
    VstrmProtocol vstrm(myip, repeaterip);

    UdpSniffer udpsniffer(myip, repeaterip);
    udpsniffer.RegisterProtocolHandler(50120, &vstrm);
    udpsniffer.Sniff();
}
