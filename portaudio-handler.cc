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
