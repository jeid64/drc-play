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
