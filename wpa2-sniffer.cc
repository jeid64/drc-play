#include "wpa2-sniffer.h"

#include "protocol-handler.h"

#include <stdio.h>

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
    , bssid_(bssid)
    , psk_(psk) {
    SetBssidFilter();
}

Wpa2Sniffer::~Wpa2Sniffer() {
}

void Wpa2Sniffer::RegisterProtocolHandler(uint16_t port,
                                          ProtocolHandler* handler) {
    handlers_[port] = handler;
}

void Wpa2Sniffer::Sniff() {
    pcap_.Loop([=](const uint8_t* pkt, int len) {
        this->HandleCapturedPacket(pkt, len);
    });
}

void Wpa2Sniffer::SetBssidFilter() {
    char filter[1024];

    snprintf(filter, sizeof (filter),
             "(wlan type data) and ("
                "wlan addr1 %s or "
                "wlan addr2 %s or "
                "wlan addr3 %s or "
                "wlan addr4 %s)",
             bssid_.c_str(), bssid_.c_str(), bssid_.c_str(), bssid_.c_str());

    pcap_.SetFilter(filter);
}

void Wpa2Sniffer::HandleCapturedPacket(const uint8_t* pkt, int len) {
    (void)pkt;
    (void)len;
    printf("packet! size=%d\n", len);
}
