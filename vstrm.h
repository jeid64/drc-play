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
    bool stop_signal;
    std::vector<uint8_t> data;

    FrameBuffer() : next(nullptr), is_idr(false), broken(false),
                    stop_signal(false) {}
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
