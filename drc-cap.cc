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

#include "astrm.h"
#include "cmd.h"
#include "portaudio-handler.h"
#include "wpa2-sniffer.h"

#include <signal.h>
#include <string>


#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>
#include <netinet/in.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <stdlib.h>
#include <stdio.h>

void send_msg(){

  int sock_fd_;

  sockaddr_in sin;
  memset(&sin, 0, sizeof (sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(50010);
  inet_aton("192.168.1.10", &sin.sin_addr);

  sock_fd_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if (sock_fd_ == -1) {
    printf("Fuck \n");
  }


    sockaddr_in sender;
    socklen_t sender_len = sizeof (sender);
    int msg_max_size = 2048;
    const char* message = "\1\0\0\0";
    sendto(sock_fd_, message,sizeof(message),0,reinterpret_cast<sockaddr*>(&sender),sender_len);
    printf("sent\n");

}

int main(int argc, char** argv) {
    //VstrmProtocol vstrm;
    //SdlHandler sdl;

    //vstrm.RegisterVideoHandler(&sdl);
    AstrmProtocol astrm;
    PortaudioHandler portaudio;
    astrm.RegisterAudioHandler(&portaudio);

    int sock_fd_;

    sockaddr_in sin;
    memset(&sin, 0, sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(50121);
    inet_aton("129.21.154.221", &sin.sin_addr);

    sock_fd_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd_ == -1) {
        return false;
    }

    if (bind(sock_fd_, reinterpret_cast<sockaddr*>(&sin), sizeof (sin)) == -1) {
        return false;
    }

    sockaddr_in sender;
    socklen_t sender_len = sizeof (sender);
    int msg_max_size = 2048;
    uint8_t* data = (uint8_t*) malloc(2048);
    while (1) {
        printf("got loop\n");
        int size = recvfrom(sock_fd_, data, msg_max_size, 0,
        reinterpret_cast<sockaddr*>(&sender), &sender_len);
        printf("size : %d\n", size);
        //vstrm.HandlePacket(data, size);
        astrm.HandlePacket(data, size);
    }
}
