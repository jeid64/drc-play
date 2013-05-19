#ifndef __DRC_CAP_PORTAUDIO_HANDLER_H_
#define __DRC_CAP_PORTAUDIO_HANDLER_H_

#include "astrm.h"
#include "queue.h"

#include <portaudio.h>

struct SoundBuffer {
    SoundBuffer* next;
    std::vector<int16_t> samples;
};

class PortaudioHandler : public AudioHandler {
  public:
    PortaudioHandler();
    virtual ~PortaudioHandler();

    virtual void HandleVibrationChange(bool vibrate);
    virtual void HandleAudioFrame(const std::vector<int16_t>& samples);

  private:
    PaStream* pa_stream_;
    LockedQueue<SoundBuffer> queue_;
};

#endif
