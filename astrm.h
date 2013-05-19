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
