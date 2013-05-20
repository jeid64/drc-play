#ifndef __DRC_CAP_SDL_HANDLER_H_
#define __DRC_CAP_SDL_HANDLER_H_

#include "vstrm.h"

class SdlHandler : public VideoHandler {
  public:
    SdlHandler();
    virtual ~SdlHandler();

    virtual void HandleVideoFrame(const std::vector<VideoPixel>& pixels,
                                  int width);
};

#endif
