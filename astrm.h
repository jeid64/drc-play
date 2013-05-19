#ifndef __DRC_CAP_ASTRM_H_
#define __DRC_CAP_ASTRM_H_

#include "protocol-handler.h"

#include <cstdint>
#include <vector>

const uint16_t kDrcAstrmPort = 50011;

class AudioHandler {
  public:
    virtual void HandleAudioFrame(const std::vector<int16_t>& samples) = 0;
};

class AstrmProtocol : public ProtocolHandler {
  public:
    AstrmProtocol();
    virtual ~AstrmProtocol();

    void RegisterAudioHandler(AudioHandler* handler);
    void HandlePacket(const std::vector<uint8_t>& data);

  private:
    void SendAudioFrame(const std::vector<int16_t>& samples);

    std::vector<AudioHandler*> handlers_;
};

#endif
