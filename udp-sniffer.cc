#include "udp-sniffer.h"

#include "crc32.h"
#include "polarssl-sha1.h"
#include "protocol-handler.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

// Network
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
#include "vstrm.h"
#include "sdl-handler.h"

UdpSniffer::UdpSniffer() {
    // Constructor. Set Socket here.

}

UdpSniffer::~UdpSniffer() {
}

void UdpSniffer::RegisterProtocolHandler(uint16_t port,
                                          ProtocolHandler* handler) {
    handlers_[port] = handler;
}

void UdpSniffer::Sniff() {
    std::thread th(&UdpSniffer::HandleReceivedPackets, this);

    int sock_fd_;

    sockaddr_in sin;
    memset(&sin, 0, sizeof (sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(50120);
    inet_aton("192.168.1.11", &sin.sin_addr);

    sock_fd_ = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock_fd_ == -1) {
        return;
    }

    if (bind(sock_fd_, reinterpret_cast<sockaddr*>(&sin), sizeof (sin)) == -1) {
        return;
    }

    sockaddr_in sender;
    socklen_t sender_len = sizeof (sender);
    int msg_max_size = 4096;

    while (1) {
        Buffer* buf = new Buffer;
        buf->data = (uint8_t*) malloc(4096);

        int size = recvfrom(sock_fd_, buf->data, msg_max_size, 0,
                                reinterpret_cast<sockaddr*>(&sender), &sender_len);
        buf->length = size;
        queue_.Push(buf);

    }
    // Send the stop signal (length == -1) to the thread.
    Buffer* buf = new Buffer;
    buf->length = -1;
    queue_.Push(buf);
    th.join();
}

void UdpSniffer::Stop() {
    //pcap_.BreakLoop();
}

void UdpSniffer::HandleReceivedPackets() {
    Buffer* buf;
    while (true) {
        while ((buf = queue_.Pop()) == nullptr) {
            std::chrono::microseconds dura(50);
            std::this_thread::sleep_for(dura);
        }

        if (buf->length == -1) { // Stop signal
            delete buf;
            return;
        }

        //ProtocolHandler* h = handlers_[dst_port];
        uint16_t dst_port = 50210;
        ProtocolHandler* h = handlers_[dst_port];
        h->HandlePacket(buf->data, buf->length);

        delete buf;
    }
}
