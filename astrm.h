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

#ifndef __DRC_CAP_ASTRM_H_
#define __DRC_CAP_ASTRM_H_

#include "protocol-handler.h"

#include <cstdint>
#include <vector>

const uint16_t kDrcAstrmPort = 50121;

class AudioHandler {
  public:
    virtual void HandleVibrationChange(bool vibrate) = 0;
    virtual void HandleAudioFrame(const std::vector<int16_t>& samples) = 0;
};

class AstrmProtocol : public ProtocolHandler {
  public:
    AstrmProtocol();
    virtual ~AstrmProtocol();

    void RegisterAudioHandler(AudioHandler* handler);
    void HandlePacket(const uint8_t* data, int len);

  private:
    void SendAudioFrame(const std::vector<int16_t>& samples);
    void SendVibrationChange(bool vibrate);

    bool CheckSequenceId(int seq_id);

    int expected_seq_id_;
    bool vibrating_;
    std::vector<AudioHandler*> handlers_;
};

#endif
