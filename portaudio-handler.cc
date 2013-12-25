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

#include "portaudio-handler.h"

#include <cstring>

static int my_pa_cb(const void* input, void* output, unsigned long frame_count,
                    const PaStreamCallbackTimeInfo* time_info,
                    PaStreamCallbackFlags status_flags, void* user_data) {
    (void)input;
    (void)time_info;
    (void)status_flags;

    LockedQueue<SoundBuffer>* pqueue =
            reinterpret_cast<LockedQueue<SoundBuffer>*>(user_data);
    SoundBuffer* buf = pqueue->Pop();
    if (buf) {
        memcpy(output, &buf->samples[0], 2 * 2 * frame_count);
        delete buf;
    } else {
        memset(output, 0, 2 * 2 * frame_count);
    }
    return 0;
}

PortaudioHandler::PortaudioHandler() {
    Pa_Initialize();

    Pa_OpenDefaultStream(&pa_stream_, 0, 2, paInt16, 48000, 416, my_pa_cb,
                         &queue_);
    Pa_StartStream(pa_stream_);
}

PortaudioHandler::~PortaudioHandler() {
    Pa_Terminate();
}

void PortaudioHandler::HandleVibrationChange(bool vibrate) {
    // TODO(delroth): generate hard disk activity to vibrate the computer
    (void)vibrate;
}

void PortaudioHandler::HandleAudioFrame(const std::vector<int16_t>& samples) {
    SoundBuffer* buf = new SoundBuffer;
    buf->samples = samples;
    queue_.Push(buf);
}
