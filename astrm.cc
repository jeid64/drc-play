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
    if (len < 8) {
        return;
    }

    int format = data[0] >> 5;
    bool mono = (data[0] >> 4) & 1;
    bool vibrate = (data[0] >> 3) & 1;
    int packet_type = (data[0] >> 2) & 1;
    int seq_id = ((data[0] & 3) << 8) | data[1];

    if (packet_type != 0) { // not audio data
        return;
    }

    if (!CheckSequenceId(seq_id)) {
        return;
    }

    if (format != 1) { // TODO(delroth): support more than PCM 48KHz
        return;
    }

    if (mono) { // TODO(delroth): support mono sound
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
