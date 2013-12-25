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

#ifndef __DRC_CAP_WPA2_SNIFFER_H_
#define __DRC_CAP_WPA2_SNIFFER_H_

#include "packet-injector.h"
#include "queue.h"

#include <cstdint>
#include <map>
#include <memory>
#include <pcap.h>
#include <string>

class ProtocolHandler;

class PcapInterface {
  public:
    PcapInterface(const std::string& iface);
    virtual ~PcapInterface();

    bool SetFilter(const std::string& filter);

    typedef std::function<void(const uint8_t*, int)> CallbackType;
    void Loop(CallbackType callback);
    void BreakLoop();

    void GetStats(pcap_stat* st);

    void SendPacket(const uint8_t* buffer, int len);

  private:
    pcap_t* pcap_;
    struct bpf_program filter_;
};

// Struct that contains all keys and data that needs to persist between
// packets.
struct Wpa2SessionData {
    // Default constructor initializes all members to 0.
    Wpa2SessionData();

    uint8_t pmk[32];

    uint8_t sta_mac[6];
    uint8_t ap_mac[6];

    uint8_t sta_nonce[32];
    uint8_t ap_nonce[32];

    uint8_t key_descriptor;

    union {
        uint8_t bytes[80];
        struct {
            uint8_t kck[16];
            uint8_t kek[16];
            uint8_t tk[16];
            uint8_t mic_tx[8];
            uint8_t mic_rx[8];
        } keys;
    } ptk;
};

struct Buffer {
    Buffer* next;
    uint8_t data[4096];
    int length;
};

class Wpa2Sniffer : public PacketInjector {
  public:
    // Initializes a WPA2 sniffer to monitor access point <bssid> on interface
    // <iface>. The PSK used by the WPA2 network is specified by <psk>.
    Wpa2Sniffer(const std::string& iface,
                const std::string& bssid,
                const std::string& psk);

    virtual ~Wpa2Sniffer();

    // Register a handler that will be notified when a packet for a given
    // protocol (identified by the UDP source port) is sniffed.
    //
    // Warning: the handlers are not called on the main thread! They are always
    // called in the same thread though.
    void RegisterProtocolHandler(uint16_t port, ProtocolHandler* handler);

    // Starts sniffing. Runs "forever", stops only when we were desynced for
    // some reason.
    void Sniff();

    // Stops sniffing.
    void Stop();

    // From PacketInjector: injects an IP packet in the WPA2 session.
    virtual void InjectPacket(PacketInjector::Direction dir,
                              const uint8_t* ip_pkt, int len);

  private:
    // Build the PCAP filter to only get data packets for our BSSID.
    void SetBssidFilter(const std::string& bssid);

    // Called by the PcapInterface when a packet is captured.
    void HandleCapturedPacket(const uint8_t* pkt, int len);

    // Checks if the last 4 bytes of a captured packet are the FCS (Frame
    // CheckSum). Some WiFi NICs add it, sometimes without adding the correct
    // flag to the radiotap header (iwlwifi).
    bool PacketHasFcs(const uint8_t* pkt, int len);

    // Handles an EAPOL packet to synchronize with the WPA2 stream. pkt is an
    // EAP packet, without 802.11 headers.
    void HandleEapolPacket(const uint8_t* pkt, int len);

    // Computes the PTK from the information we received in EAPOL packets + the
    // PSK (from the command line).
    void ComputePtk();

    // Verifies the MIC of an EAPOL message.
    bool CheckEapolMic(const uint8_t* pkt, int len, const uint8_t* mic);

    // Handles decrypted packets from the shared queue. Parses the LLC, IP and
    // UDP headers to pass the contents to the right handler.
    void HandleDecryptedPackets();

    PcapInterface pcap_;

    bool synced_;
    Wpa2SessionData wpa_;

    // { src_port, protocol_handler }
    std::map<uint16_t, ProtocolHandler*> handlers_;

    // Queue for consumer-producer data exchange.
    LockedQueue<Buffer> queue_;
};

#endif
