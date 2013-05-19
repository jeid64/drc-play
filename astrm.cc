#include "astrm.h"

#include <cstdio>

AstrmProtocol::AstrmProtocol()
    : expected_seq_id_(-1)
    , vibrating_(false) {
}

AstrmProtocol::~AstrmProtocol() {
}

void AstrmProtocol::RegisterAudioHandler(AudioHandler* handler) {
    handlers_.push_back(handler);
}

void AstrmProtocol::HandlePacket(const uint8_t* data, int len) {
    int format = data[0] >> 5;
    bool stereo = (data[0] >> 4) & 1;
    bool vibrate = (data[0] >> 3) & 1;
    int packet_type = (data[0] >> 2) & 1;
    int seq_id = ((data[0] & 3) << 8) | data[1];

    if (packet_type != 0) { // not audio data
        return;
    }

    if (!CheckSequenceId(seq_id)) {
        return;
    }

    if (format != 0) { // TODO(delroth): support more than PCM 48KHz
        return;
    }

    if (!stereo) { // TODO(delroth): support mono sound
        return;
    }

    if (vibrate != vibrating_) {
        SendVibrationChange(vibrate);
        vibrating_ = vibrate;
    }

    int nsamples = (len - 8) / 2;
    std::vector<int16_t> samples(nsamples);
    for (int i = 0; i < nsamples; ++i) {
        samples[i] = (data[8 + i * 2 + 1] << 8) | data[8 + i * 2];
    }
    SendAudioFrame(samples);
}

bool AstrmProtocol::CheckSequenceId(int seq_id) {
    bool ret = true;
    if (expected_seq_id_ == -1) {
        expected_seq_id_ = seq_id;
    } else if (expected_seq_id_ != seq_id) {
        ret = false;
    }
    expected_seq_id_ = (seq_id + 1) & 0x3ff;
    return ret;
}

void AstrmProtocol::SendAudioFrame(const std::vector<int16_t>& samples) {
    for (auto& h : handlers_) {
        h->HandleAudioFrame(samples);
    }
}

void AstrmProtocol::SendVibrationChange(bool vibrate) {
    for (auto& h : handlers_) {
        h->HandleVibrationChange(vibrate);
    }
}
