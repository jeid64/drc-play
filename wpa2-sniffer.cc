#include "wpa2-sniffer.h"

#include "ccmp.h"
#include "crc32.h"
#include "polarssl-sha1.h"
#include "protocol-handler.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <thread>

#define TENDONIN

Wpa2SessionData::Wpa2SessionData() {
    memset(this, 0, sizeof (*this));
}

PcapInterface::PcapInterface(const std::string& iface) : pcap_(nullptr) {
    char errbuf[PCAP_ERRBUF_SIZE];

    pcap_t* pd = pcap_create(iface.c_str(), errbuf);
    if (!pd) {
        fprintf(stderr, "error: pcap_create: %s\n", errbuf);
        return;
    }

    // TODO(delroth): keep the status and display it with pcap_statustostr

    if (pcap_set_snaplen(pd, 65535) != 0) {
        fprintf(stderr, "error: pcap_set_snaplen failed\n");
        return;
    }

    if (pcap_set_promisc(pd, 1) != 0) {
        fprintf(stderr, "error: cannot set promisc mode on %s\n",
                iface.c_str());
        return;
    }

    if (!pcap_can_set_rfmon(pd) || pcap_set_rfmon(pd, 1) != 0) {
        fprintf(stderr, "error: cannot set monitor mode on %s\n",
                iface.c_str());
        return;
    }

    if (pcap_set_timeout(pd, 100) != 0) {
        fprintf(stderr, "error: cannot set timeout on %s\n", iface.c_str());
        return;
    }

    if (pcap_activate(pd) != 0) {
        fprintf(stderr, "error: cannot activate pcap device\n");
        return;
    }

    pcap_ = pd;
}

PcapInterface::~PcapInterface() {
    if (pcap_)
        pcap_close(pcap_);
}

bool PcapInterface::SetFilter(const std::string& filter) {
    if (pcap_compile(pcap_, &filter_, filter.c_str(), 1, 0) != 0) {
        fprintf(stderr, "error: cannot compile the BPF filter\n");
        return false;
    }

    if (pcap_setfilter(pcap_, &filter_) != 0) {
        fprintf(stderr, "error: cannot set capture filter\n");
        return false;
    }

    return true;
}

static void my_pcap_callback(u_char* user_data,
                             const struct pcap_pkthdr* pkt_meta,
                             const u_char* pkt) {
    PcapInterface::CallbackType* cb =
        reinterpret_cast<PcapInterface::CallbackType*>(user_data);
    (*cb)(pkt, pkt_meta->caplen);
}

void PcapInterface::Loop(CallbackType callback) {
    if (pcap_loop(pcap_, -1, my_pcap_callback,
                  reinterpret_cast<u_char*>(&callback)) != 0) {
        fprintf(stderr, "error: pcap_loop returned != 0\n");
    }
}

Wpa2Sniffer::Wpa2Sniffer(const std::string& iface,
                         const std::string& bssid,
                         const std::string& psk)
    : pcap_(iface)
    , synced_(false) {
    SetBssidFilter(bssid);

    uint8_t hex_to_int[256] = { 0 };
    hex_to_int['0'] = 0x0; hex_to_int['1'] = 0x1; hex_to_int['2'] = 0x2;
    hex_to_int['3'] = 0x3; hex_to_int['4'] = 0x4; hex_to_int['5'] = 0x5;
    hex_to_int['6'] = 0x6; hex_to_int['7'] = 0x7; hex_to_int['8'] = 0x8;
    hex_to_int['9'] = 0x9; hex_to_int['a'] = 0xA; hex_to_int['b'] = 0xB;
    hex_to_int['c'] = 0xC; hex_to_int['d'] = 0xD; hex_to_int['e'] = 0xE;
    hex_to_int['f'] = 0xF; hex_to_int['A'] = 0xA; hex_to_int['B'] = 0xB;
    hex_to_int['C'] = 0xC; hex_to_int['D'] = 0xD; hex_to_int['E'] = 0xE;
    hex_to_int['F'] = 0xF;
    for (int i = 0; i < 32; ++i) {
        wpa_.pmk[i] = (hex_to_int[(uint8_t)psk[2 * i]] << 4) |
                        hex_to_int[(uint8_t)psk[2 * i + 1]];
    }
}

Wpa2Sniffer::~Wpa2Sniffer() {
}

void Wpa2Sniffer::RegisterProtocolHandler(uint16_t port,
                                          ProtocolHandler* handler) {
    handlers_[port] = handler;
}

void Wpa2Sniffer::Sniff() {
    std::thread th(&Wpa2Sniffer::HandleDecryptedPackets, this);

    pcap_.Loop([=](const uint8_t* pkt, int len) {
        this->HandleCapturedPacket(pkt, len);
    });
}

void Wpa2Sniffer::SetBssidFilter(const std::string& bssid) {
    char filter[1024];

    snprintf(filter, sizeof (filter),
             "(wlan type data) and ("
                "wlan addr1 %s or "
                "wlan addr2 %s or "
                "wlan addr3 %s or "
                "wlan addr4 %s)",
             bssid.c_str(), bssid.c_str(), bssid.c_str(), bssid.c_str());

    pcap_.SetFilter(filter);
}

void Wpa2Sniffer::HandleCapturedPacket(const uint8_t* pkt, int len) {
    // Check if we have a FCS at the end of the packet and truncate it if
    // needed.
    if (PacketHasFcs(pkt, len)) {
        len -= 4;
    }

    // Skip the Radiotap header, we won't need it anymore.
    if (len < 4) {
        return;
    }
    uint16_t radiotap_len = (pkt[3] << 8) | pkt[2];
    if (radiotap_len >= len) {
        return;
    }
    pkt += radiotap_len;
    len -= radiotap_len;

    if (synced_) {
        // Check if the packet has a valid 802.11 header
        if (len <= 0x22) {
            return;
        }

        // Check if this is a data packet (the other types should already be
        // filtered by our PCAP filter, but better check).
        if ((pkt[0] & 0x0c) != 0x08) {
            return;
        }


        Buffer* buf = new Buffer;
        if (!CcmpDecrypt(pkt, len, wpa_.ptk.keys.tk, buf->data,
                         &buf->length)) {
            // Check if this is a broadcast packet (not supported yet)
            if (memcmp(pkt + 4, "\xff\xff\xff\xff\xff\xff", 6) == 0) {
                fprintf(stderr, "warn: failed to decrypt bcast packet\n");
            } else {
                fprintf(stderr, "error: failed to decrypt unicast packet\n");
                synced_ = false;
            }
        } else {
            queue_.Push(buf);
        }
    } else {
        // Try to find the EAPOL packets that contain the key exchange. Their
        // LLC type (u16_be @ 0x20) is 0x888e.
        if (len < 0x22 || pkt[0x20] != 0x88 || pkt[0x21] != 0x8e) {
            return;
        }

        HandleEapolPacket(pkt, len);
    }
}

bool Wpa2Sniffer::PacketHasFcs(const uint8_t* pkt, int len) {
    // Check the Radiotap header flags at offset 0x10. Bit 4 (0x10) indicates
    // the presence of the FCS.
    if (len > 0x10 && pkt[0x10] & 0x10) {
        return true;
    }

    // We can't trust the Radiotap flags: some WiFi drivers/NICs add the FCS
    // without setting the flag (iwlwifi). Check if the 4 bytes at the end
    // match the checksum of the packet (without its radiotap header).
    uint16_t radiotap_len = (pkt[3] << 8) | pkt[2];
    if (len - 4 <= radiotap_len) { // 0x12 == radiotap header length
        return false;
    }
    uint32_t computed_fcs = crc32(0, &pkt[radiotap_len],
                                  len - radiotap_len - 4);
    if (pkt[len - 4] == ((computed_fcs >> 0) & 0xFF) &&
        pkt[len - 3] == ((computed_fcs >> 8) & 0xFF) &&
        pkt[len - 2] == ((computed_fcs >> 16) & 0xFF) &&
        pkt[len - 1] == ((computed_fcs >> 24) & 0xFF)) {
        return true;
    }

    return false;
}

void Wpa2Sniffer::HandleEapolPacket(const uint8_t* pkt, int len) {
    const uint8_t* wlan_pkt = pkt;

    // Skip the 802.11 and LLC headers.
    pkt += 0x22;
    len -= 0x22;

    // Check EAP message type: should be Key == 0x03
    if (pkt[1] != 0x03) {
        return;
    }

    // Check key type: should be RSN == 0x02
    if (pkt[4] != 0x02) {
        return;
    }

    uint16_t info = (pkt[5] << 8) | pkt[6];

    // Message 1/4 has opt ACK set and INSTALL unset. From it, we get the AP
    // nonce as well as the key descriptor (it is in all EAPOL messages).
    //
    // We also copy the AP and STA MACs at this point.
    if ((info & 0x80) && !(info & 0x40)) {
        memcpy(wpa_.ap_nonce, pkt + 17, 32);
        wpa_.key_descriptor = info & 7;

        memcpy(wpa_.sta_mac, wlan_pkt + 4, 6);
        memcpy(wpa_.ap_mac, wlan_pkt + 10, 6);
    }

    // Message 2/4 has opt MIC set, ACK unset, SECURE unset. From it, we get
    // the STA nonce, so we can compute the PTK.
    if ((info & 0x100) && !(info & 0x80) && !(info & 0x200)) {
        memcpy(wpa_.sta_nonce, pkt + 17, 32);

        ComputePtk();
    }

    // Messages 2, 3 and 4 have a MIC to check. This is indicated by the MIC
    // option. We can only do it after we computed the KCK (which is part of
    // the PTK), but at this point we should have computed it.
    //
    // We also count the number of MICs we successfully computed. If it is 3,
    // we know we are correctly synchronized.
    static int valid_mic_count = 0;
    if ((info & 0x100)) {
        uint8_t mic[16];
        uint8_t mic_pkt[4096];

        memcpy(mic, pkt + 81, 16);
        memcpy(mic_pkt, pkt, len);
        memset(mic_pkt + 81, 0, 16);
        if (!CheckEapolMic(mic_pkt, len, mic)) {
            fprintf(stderr, "error: EAPOL MIC computation failed\n");
            return;
        } else {
            if (++valid_mic_count == 3) {
                fprintf(stderr, "info: now synchronized to the WPA2 session\n");
                synced_ = true;
            }
        }
    }
}

void Wpa2Sniffer::ComputePtk() {
    uint8_t pke[100];

    memcpy(pke, "Pairwise key expansion", 23);

    if (memcmp(wpa_.ap_mac, wpa_.sta_mac, 6) < 0) {
        memcpy(pke + 23, wpa_.ap_mac, 6);
        memcpy(pke + 29, wpa_.sta_mac, 6);
    } else {
        memcpy(pke + 29, wpa_.ap_mac, 6);
        memcpy(pke + 23, wpa_.sta_mac, 6);
    }

    if (memcmp(wpa_.ap_nonce, wpa_.sta_nonce, 32) < 0) {
        memcpy(pke + 35, wpa_.ap_nonce, 32);
        memcpy(pke + 67, wpa_.sta_nonce, 32);
    } else {
        memcpy(pke + 67, wpa_.ap_nonce, 32);
        memcpy(pke + 35, wpa_.sta_nonce, 32);
    }

    for (int i = 0; i < 4; ++i) {
        pke[99] = i;
        sha1_hmac(wpa_.pmk, 32, pke, 100, wpa_.ptk.bytes + i * 20);
    }

#ifdef TENDONIN
    uint8_t rot_buf[3];
    memcpy(rot_buf, wpa_.ptk.bytes, 3);
    memmove(wpa_.ptk.bytes, wpa_.ptk.bytes + 3, 48 - 3);
    memcpy(wpa_.ptk.bytes + 48 - 3, rot_buf, 3);
#endif
}

bool Wpa2Sniffer::CheckEapolMic(const uint8_t* pkt, int len,
                                const uint8_t* mic) {
    uint8_t computed_mic[20];

    sha1_hmac(wpa_.ptk.keys.kck, 16, pkt, len, computed_mic);

    return memcmp(computed_mic, mic, 16) == 0;
}

void Wpa2Sniffer::HandleDecryptedPackets() {
    Buffer* buf;
    while (true) {
        while ((buf = queue_.Pop()) == nullptr) {
            std::chrono::milliseconds dura(1);
            std::this_thread::sleep_for(dura);
        }

        if (buf->length >= 0x24 &&   // Enough space for LLC/IP/UDP headers
            buf->data[6] == 0x08 && buf->data[7] == 0x00 && // LLC->IP
            buf->data[17] == 0x11) { // IP->UDP
            uint16_t dst_port = (buf->data[30] << 8) | buf->data[31];
            const uint8_t* udp_data = buf->data + 0x24;
            int udp_len = buf->length - 0x24;
            if (handlers_.find(dst_port) != handlers_.end()) {
                ProtocolHandler* h = handlers_[dst_port];
                h->HandlePacket(udp_data, udp_len);
            }
        }

        delete buf;
    }
}
