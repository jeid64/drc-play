#include "vstrm.h"

VstrmProtocol::VstrmProtocol() {
}

VstrmProtocol::~VstrmProtocol() {
}

void VstrmProtocol::RegisterVideoHandler(VideoHandler* handler) {
    handlers_.push_back(handler);
}

void VstrmProtocol::HandlePacket(const std::vector<uint8_t>& data) {
    (void)data;
}

void VstrmProtocol::SendVideoFrame(const std::vector<VideoPixel>& pixels,
                                   int width) {
    for (auto& h : handlers_) {
        h->HandleVideoFrame(pixels, width);
    }
}
