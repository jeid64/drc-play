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
