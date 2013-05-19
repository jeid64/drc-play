#include "astrm.h"

AstrmProtocol::AstrmProtocol() {
}

AstrmProtocol::~AstrmProtocol() {
}

void AstrmProtocol::RegisterAudioHandler(AudioHandler* handler) {
    handlers_.push_back(handler);
}

void AstrmProtocol::HandlePacket(const std::vector<uint8_t>& data) {
    (void)data;
}

void AstrmProtocol::SendAudioFrame(const std::vector<int16_t>& samples) {
    for (auto& h : handlers_) {
        h->HandleAudioFrame(samples);
    }
}
