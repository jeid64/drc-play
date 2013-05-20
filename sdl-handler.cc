#include "sdl-handler.h"

#include <cstdio>
#include <SDL/SDL.h>

SdlHandler::SdlHandler() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetVideoMode(854, 480, 24, SDL_HWSURFACE | SDL_DOUBLEBUF);
}

SdlHandler::~SdlHandler() {
}

void SdlHandler::HandleVideoFrame(const std::vector<VideoPixel>& pixels,
                                  int width) {
    SDL_Surface* surf = SDL_GetVideoSurface();
    SDL_LockSurface(surf);
    for (int i = 0; i < 480; ++i) {
        memcpy((uint8_t*)surf->pixels + surf->pitch * i, &pixels[854 * i],
               854 * 3);
    }
    SDL_UnlockSurface(surf);
    SDL_Flip(surf);
}
