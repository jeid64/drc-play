#ifndef __DRC_CAP_VSTRM_H_
#define __DRC_CAP_VSTRM_H_

#include "protocol-handler.h"

#include <cstdint>
#include <vector>

const uint16_t kDrcVstrmPort = 50010;

struct VideoPixel {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

class VideoHandler {
  public:
    virtual void HandleVideoFrame(const std::vector<VideoPixel>& pixels,
                                  int width) = 0;
};

class VstrmProtocol : public ProtocolHandler {
  public:
    VstrmProtocol();
    virtual ~VstrmProtocol();

    void RegisterVideoHandler(VideoHandler* handler);
    void HandlePacket(const uint8_t* data, int len);

  private:
    void SendVideoFrame(const std::vector<VideoPixel>& pixels, int width);

    std::vector<VideoHandler*> handlers_;
};

#endif
