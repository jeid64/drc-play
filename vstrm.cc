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

// send_msg

#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <stdlib.h>
#include <stdio.h>
#include "vstrm.h"
#include "sdl-handler.h"

class H264Decoder {
  public:
    H264Decoder() {
    }

  ~H264Decoder() {
      // TODO(delroth): cleanup
  }

  int DecodeFrame(const std::vector<uint8_t>& nalu,
                   std::vector<VideoPixel>& pixels) {
      return -1;
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
    bool frame_begin = (data[2] >> 6) & 1;
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
    }

    if (frame_begin) {
        curr_frame_->frame_begin = true;
    }

    // Check the extended header for the IDR option.
    curr_frame_->is_idr = false;
    for (int i = 8; i < 16; ++i) {
        if (data[i] == 0x80) {
            curr_frame_->is_idr = true;
            break;
        }
    }

    if (curr_frame_->frame_begin) {
        curr_frame_->data.insert(curr_frame_->data.end(), data + 16, data + len);
    }

    if (frame_end && curr_frame_->frame_begin) {
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
        send_msg();
    }
    expected_seq_id_ = (seq_id + 1) & 0x3ff;
    return ret;
}

void VstrmProtocol::send_msg() {
    struct sockaddr_in si_other;
    int s, slen=sizeof(si_other);

    const char* message =  "\1\0\0\0";

    if ( (s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        fprintf(stderr, "Failed to open MSG stream socket. \n");
    }

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(50010); // Wii U MSG port.

    if (inet_aton("192.168.1.10" , &si_other.sin_addr) == 0)
    {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    if (sendto(s, message, sizeof(uint32_t) , 0 , (struct sockaddr *) &si_other, slen)==-1)
    {
        fprintf(stderr, "Failed to send IDR request message.\n");
    }

    close(s);
}

void VstrmProtocol::SendVideoFrame(const std::vector<VideoPixel>& pixels,
                                   int width) {
    for (auto& h : handlers_) {
        h->HandleVideoFrame(pixels, width);
    }
}

void DumpH264Headers(FILE* fp) {
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t byte;
  u8 nal_start_code[] = { 0x00, 0x00, 0x00, 0x01 };
  u8 gamepad_sps[] = { 0x67, 0x64, 0x00, 0x20, 0xac, 0x2b, 0x40,
                       0x6c, 0x1e, 0xf3, 0x68 };
  u8 gamepad_pps[] = { 0x68, 0xee, 0x06, 0x0c, 0xe8 };

  fwrite(nal_start_code, sizeof (nal_start_code), 1, fp);
  fwrite(gamepad_sps, sizeof (gamepad_sps), 1, fp);
  fwrite(nal_start_code, sizeof (nal_start_code), 1, fp);
  fwrite(gamepad_pps, sizeof (gamepad_pps), 1, fp);
}
void VstrmProtocol::HandleEncodedFrames() {
  FILE* dump_file_;
    dump_file_ = fopen("dump.h264", "wb");
    if (dump_file_) {
      DumpH264Headers(dump_file_);
    }
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

        uint8_t* a = &nal_encapsulated[0];
        size_t length = nal_encapsulated.size();
        fwrite(a, length, 1, dump_file_);
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
