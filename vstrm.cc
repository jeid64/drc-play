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

#include "vstrm.h"

#include <chrono>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
}

class H264Decoder {
  public:
    H264Decoder() {
        avcodec_register_all();

        av_init_packet(&packet_);

        codec_ = avcodec_find_decoder(CODEC_ID_H264);
        if (!codec_) {
            fprintf(stderr, "error: avcodec_find_decoder failed\n");
        }

        context_ = avcodec_alloc_context3(codec_);
        if (!context_) {
            fprintf(stderr, "error: avcodec_alloc_context3 failed\n");
        }

        if (avcodec_open2(context_, codec_, nullptr) < 0) {
            fprintf(stderr, "error: avcodec_open2 failed\n");
        }

        frame_ = avcodec_alloc_frame();
        out_frame_ = avcodec_alloc_frame();

        sws_context_ = sws_getContext(854, 480, PIX_FMT_YUV420P,
                                      854, 480, PIX_FMT_BGR24,
                                      SWS_FAST_BILINEAR, nullptr, nullptr,
                                      nullptr);

        int req_size = avpicture_get_size(PIX_FMT_BGR24, 854, 480);
        uint8_t* buffer = new uint8_t[req_size];
        avpicture_fill((AVPicture*)out_frame_, buffer, PIX_FMT_BGR24,
                       854, 480);
    }

  ~H264Decoder() {
      // TODO(delroth): cleanup
  }

  void DecodeFrame(const std::vector<uint8_t>& nalu,
                   std::vector<VideoPixel>& pixels) {
      packet_.data = const_cast<uint8_t*>(&nalu[0]);
      packet_.size = nalu.size();

      int got_frame = 0;
      int length = avcodec_decode_video2(context_, frame_, &got_frame,
                                         &packet_);
      if (length != (int)nalu.size()) {
          fprintf(stderr, "error: avcodec_decode_video2\n");
      }

      if (got_frame) {
          sws_scale(sws_context_, frame_->data, frame_->linesize, 0, 480,
                    out_frame_->data, out_frame_->linesize);

          pixels.resize(out_frame_->linesize[0] * 480);
          memcpy((uint8_t*)&pixels[0], out_frame_->data[0],
                 out_frame_->linesize[0] * 480);
      }
  }

  private:
    AVPacket packet_;
    AVCodec* codec_;
    AVCodecContext* context_;
    AVFrame* frame_;
    AVFrame* out_frame_;
    SwsContext* sws_context_;
};

VstrmProtocol::VstrmProtocol()
    : decoded_frame_num_(0)
    , expected_seq_id_(-1)
    , curr_frame_(new FrameBuffer)
    , h264decoder_(new H264Decoder)
    , injector_(nullptr)
    , decoding_th_(&VstrmProtocol::HandleEncodedFrames, this) {
}

VstrmProtocol::~VstrmProtocol() {
    FrameBuffer* frame = new FrameBuffer;
    frame->stop_signal = true;
    queue_.Push(frame);
    decoding_th_.join();
}

void VstrmProtocol::RegisterVideoHandler(VideoHandler* handler) {
    handlers_.push_back(handler);
}

void VstrmProtocol::SetPacketInjector(PacketInjector* injector) {
    injector_ = injector;
}

void VstrmProtocol::HandlePacket(const uint8_t* data, int len) {
    if (len < 16) {
        return;
    }

    int magic = data[0] >> 4;
    int packet_type = (data[0] >> 2) & 3;
    int seq_id = ((data[0] & 3) << 8) | data[1];
    //bool init = (data[2] >> 7) & 1;
    //bool frame_begin = (data[2] >> 6) & 1;
    //bool chunk_end = (data[2] >> 5) & 1;
    bool frame_end = (data[2] >> 4) & 1;
    bool has_timestamp = (data[2] >> 3) & 1;
    //int payload_size = ((data[2] & 7) << 8) | data[3];
    //uint32_t timestamp = (data[4] << 24) | (data[5] << 16) |
    //                     (data[6] << 8) | data[7];

    if (magic != 0xF) {
        return;
    }

    if (packet_type != 0) {
        return;
    }

    if (!has_timestamp) { // wtf?
        return;
    }

    if (!CheckSequenceId(seq_id)) {
        curr_frame_->broken = true;
        RequestIFrame();
    }

    // Check the extended header for the IDR option.
    curr_frame_->is_idr = false;
    for (int i = 8; i < 16; ++i) {
        if (data[i] == 0x80) {
            curr_frame_->is_idr = true;
            printf("Got IDR!\n");
            break;
        }
    }

    curr_frame_->data.insert(curr_frame_->data.end(), data + 16, data + len);
    if (frame_end) {
        if (!curr_frame_->broken) {
            queue_.Push(curr_frame_);
        } else {
            delete curr_frame_;
        }
        curr_frame_ = new FrameBuffer;
    }
}

bool VstrmProtocol::CheckSequenceId(int seq_id) {
    bool ret = true;
    if (expected_seq_id_ == -1) {
        expected_seq_id_ = seq_id;
    } else if (expected_seq_id_ != seq_id) {
        fprintf(stderr, "expected: %d seqid: %d\n", expected_seq_id_, seq_id);
        ret = false;
    }
    expected_seq_id_ = (seq_id + 1) & 0x3ff;
    return ret;
}

void VstrmProtocol::SendVideoFrame(const std::vector<VideoPixel>& pixels,
                                   int width) {
    for (auto& h : handlers_) {
        h->HandleVideoFrame(pixels, width);
    }
}

void VstrmProtocol::HandleEncodedFrames() {
    while (true) {
        FrameBuffer* fr;
        while ((fr = queue_.Pop()) == nullptr) {
            std::chrono::microseconds dura(50);
            std::this_thread::sleep_for(dura);
        }

        if (fr->stop_signal) {
            delete fr;
            return;
        }

        std::vector<uint8_t> nal_encapsulated;
        EncapsulateH264(fr, nal_encapsulated);

        std::vector<VideoPixel> pixels;
        h264decoder_->DecodeFrame(nal_encapsulated, pixels);

        SendVideoFrame(pixels, 854);

        delete fr;
    }
}

void VstrmProtocol::EncapsulateH264(const FrameBuffer* frame,
                                    std::vector<uint8_t>& out) {
    if (frame->is_idr) {
        static const uint8_t sps_pps[] = {
            // SPS
            0x00, 0x00, 0x00, 0x01,
            0x67, 0x64, 0x00, 0x20, 0xac, 0x2b, 0x40, 0x6c, 0x1e, 0xf3, 0x68,

            // PPS
            0x00, 0x00, 0x00, 0x01,
            0x68, 0xee, 0x06, 0x0c, 0xe8
        };

        out.insert(out.end(), sps_pps, sps_pps + sizeof (sps_pps));
    }

    // Slice NAL unit
    out.push_back(0); out.push_back(0); out.push_back(0); out.push_back(1);

    uint32_t slice_header;
    if (frame->is_idr) {
        slice_header = 0x25b804ff;
    } else {
        slice_header = 0x21e003ff | ((decoded_frame_num_ & 0xFF) << 13);
        decoded_frame_num_ += 1;
    }
    out.push_back((slice_header >> 24) & 0xFF);
    out.push_back((slice_header >> 16) & 0xFF);
    out.push_back((slice_header >> 8) & 0xFF);
    out.push_back((slice_header >> 0) & 0xFF);

    // Add the video data, NAL escaped.
    out.push_back(frame->data[0]); out.push_back(frame->data[1]);
    for (size_t i = 2; i < frame->data.size(); ++i) {
        if (frame->data[i] <= 3 && out[out.size() - 2] == 0 &&
                out[out.size() - 1] == 0) {
            out.push_back(3);
        }
        out.push_back(frame->data[i]);
    }
}

void VstrmProtocol::RequestIFrame() {
    uint8_t ip_packet[] = {
        // IPv4 header
        0x45, // IPv4 and header size == 5 * 32b
        0x00, // DSCP: default
        0x00, 0x20, // Packet length (20 + 8 + 4)
        0x00, 0x00, // Identifier
        0x00, 0x00, // Flags and fragment offset
        0x40, // TTL
        0x11, // Protocol = UDP
        0x00, 0x00, // Checksum: computed later
        0xc0, 0xa8, 0x01, 0x0b, // Source: 192.168.1.11
        0xc0, 0xa8, 0x01, 0x0a, // Destination: 192.168.1.10

        // UDP header
        0xc3, 0xbe, // Source port: 50110
        0xc3, 0x5a, // Destination port: 50010
        0x00, 0x0c, // Packet length (8 + 4)
        0x00, 0x00, // Checksum (none)

        // Data: sync message
        0x01, 0x00, 0x00, 0x00,
    };

    // Compute the IP checksum
    uint32_t checksum = 0;
    for (int i = 0; i < 10; ++i) {
        checksum += (ip_packet[2 * i] << 8) | ip_packet[2 * i + 1];
    }
    checksum = (checksum >> 16) + (checksum & 0xFFFF);
    checksum = ~checksum & 0xFFFF;
    ip_packet[10] = checksum >> 8;
    ip_packet[11] = checksum & 0xFF;

    // Send to our packet injector if it is set.
    if (injector_) {
        injector_->InjectPacket(PacketInjector::STA_TO_AP, ip_packet,
                                sizeof (ip_packet));
    } else {
        fprintf(stderr, "warn: cannot inject, injector not set in vstrm\n");
    }
}
