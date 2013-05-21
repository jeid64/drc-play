#ifndef __DRC_CAP_VSTRM_H_
#define __DRC_CAP_VSTRM_H_

#include "packet-injector.h"
#include "protocol-handler.h"
#include "queue.h"

#include <cstdint>
#include <memory>
#include <thread>
#include <vector>

const uint16_t kDrcVstrmPort = 50120;

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

struct FrameBuffer {
    FrameBuffer* next;
    bool is_idr;
    bool broken;
    std::vector<uint8_t> data;

    FrameBuffer() : next(nullptr), is_idr(false), broken(false) {}
};

class H264Decoder;

class VstrmProtocol : public ProtocolHandler {
  public:
    VstrmProtocol();
    virtual ~VstrmProtocol();

    void RegisterVideoHandler(VideoHandler* handler);
    void SetPacketInjector(PacketInjector* injector);

    void HandlePacket(const uint8_t* data, int len);

  private:
    bool CheckSequenceId(int seq_id);
    void RequestIFrame();

    void SendVideoFrame(const std::vector<VideoPixel>& pixels, int width);

    void HandleEncodedFrames();
    void EncapsulateH264(const FrameBuffer* frame, std::vector<uint8_t>& out);

    std::vector<VideoHandler*> handlers_;

    int decoded_frame_num_;
    int expected_seq_id_;

    FrameBuffer* curr_frame_;
    LockedQueue<FrameBuffer> queue_;

    std::unique_ptr<H264Decoder> h264decoder_;
    PacketInjector* injector_;

    std::thread decoding_th_;
};

#endif
