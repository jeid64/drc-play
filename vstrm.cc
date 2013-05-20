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
    , decoding_th_(&VstrmProtocol::HandleEncodedFrames, this) {
}

VstrmProtocol::~VstrmProtocol() {
}

void VstrmProtocol::RegisterVideoHandler(VideoHandler* handler) {
    handlers_.push_back(handler);
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
