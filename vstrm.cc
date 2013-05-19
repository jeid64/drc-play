#include "vstrm.h"

VstrmProtocol::VstrmProtocol() {
}

VstrmProtocol::~VstrmProtocol() {
}

void VstrmProtocol::RegisterVideoHandler(VideoHandler* handler) {
    handlers_.push_back(handler);
}

void VstrmProtocol::HandlePacket(const uint8_t* data, int len) {
    (void)data;
}

void VstrmProtocol::SendVideoFrame(const std::vector<VideoPixel>& pixels,
                                   int width) {
    for (auto& h : handlers_) {
        h->HandleVideoFrame(pixels, width);
    }
}
