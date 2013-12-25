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
#include "wpa2-sniffer.h"

#include <signal.h>
#include <string>

static Wpa2Sniffer* p_sniffer = nullptr;

static void quit_handler(int) {
    if (p_sniffer) {
        p_sniffer->Stop();
    }
}

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <wlanX> <bssid> <psk>\n", argv[0]);
        return 1;
    }

    std::string iface = argv[1];
    std::string bssid = argv[2];
    std::string psk = argv[3];

    Wpa2Sniffer sniffer(iface, bssid, psk);
    VstrmProtocol vstrm;
    AstrmProtocol astrm;
    CmdProtocol cmd;
    PortaudioHandler portaudio;
    SdlHandler sdl;

    vstrm.RegisterVideoHandler(&sdl);
    vstrm.SetPacketInjector(&sniffer);

    astrm.RegisterAudioHandler(&portaudio);

    sniffer.RegisterProtocolHandler(kDrcVstrmPort, &vstrm);
    sniffer.RegisterProtocolHandler(kDrcAstrmPort, &astrm);
    sniffer.RegisterProtocolHandler(kDrcCmdPort, &cmd);

    p_sniffer = &sniffer;
    signal(SIGINT, quit_handler);
    signal(SIGQUIT, quit_handler);
    sniffer.Sniff();

    return 0;
}
