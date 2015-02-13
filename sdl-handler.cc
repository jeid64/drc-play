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

float framespersecond;

SdlHandler::SdlHandler() {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_SetVideoMode(854, 480, 24, SDL_HWSURFACE | SDL_DOUBLEBUF);
    fpsinit();
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
    fpsthink();
    printf("FPS %f\n", framespersecond);
}

// How many frames time values to keep
// The higher the value the smoother the result is...
// Don't make it 0 or less :)
#define FRAME_VALUES 10

// An array to store frame times:
Uint32 frametimes[FRAME_VALUES];

// Last calculated SDL_GetTicks
Uint32 frametimelast;

// total frames rendered
Uint32 framecount;

// the value you want


// This function gets called once on startup.
void SdlHandler::fpsinit() {

        // Set all frame times to 0ms.
        memset(frametimes, 0, sizeof(frametimes));
        framecount = 0;
        framespersecond = 0;
        frametimelast = SDL_GetTicks();

}

void SdlHandler::fpsthink() {

        Uint32 frametimesindex;
        Uint32 getticks;
        Uint32 count;
        Uint32 i;

        // frametimesindex is the position in the array. It ranges from 0 to FRAME_VALUES.
        // This value rotates back to 0 after it hits FRAME_VALUES.
        frametimesindex = framecount % FRAME_VALUES;

        // store the current time
        getticks = SDL_GetTicks();

        // save the frame time value
        frametimes[frametimesindex] = getticks - frametimelast;

        // save the last frame time for the next fpsthink
        frametimelast = getticks;

        // increment the frame count
        framecount++;

        // Work out the current framerate

        // The code below could be moved into another function if you don't need the value every frame.

        // I've included a test to see if the whole array has been written to or not. This will stop
        // strange values on the first few (FRAME_VALUES) frames.
        if (framecount < FRAME_VALUES) {

                count = framecount;

        } else {

                count = FRAME_VALUES;

        }

        // add up all the values and divide to get the average frame time.
        framespersecond = 0;
        for (i = 0; i < count; i++) {

                framespersecond += frametimes[i];

        }

        framespersecond /= count;

        // now to make it an actual frames per second value...
        framespersecond = 1000.f / framespersecond;

}
