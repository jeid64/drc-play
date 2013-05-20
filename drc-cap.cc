#include "astrm.h"
#include "vstrm.h"
#include "portaudio-handler.h"
#include "sdl-handler.h"
#include "wpa2-sniffer.h"

#include <string>

int main(int argc, char** argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: %s <wlanX> <bssid> <psk>\n", argv[0]);
        return 1;
    }

    std::string iface = argv[1];
    std::string bssid = argv[2];
    std::string psk = argv[3];

    VstrmProtocol vstrm;
    AstrmProtocol astrm;

    PortaudioHandler portaudio;
    astrm.RegisterAudioHandler(&portaudio);

    SdlHandler sdl;
    vstrm.RegisterVideoHandler(&sdl);

    Wpa2Sniffer sniffer(iface, bssid, psk);
    sniffer.RegisterProtocolHandler(kDrcVstrmPort, &vstrm);
    sniffer.RegisterProtocolHandler(kDrcAstrmPort, &astrm);

    sniffer.Sniff();

    return 0;
}
