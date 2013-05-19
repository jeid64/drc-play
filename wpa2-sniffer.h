#ifndef __DRC_CAP_WPA2_SNIFFER_H_
#define __DRC_CAP_WPA2_SNIFFER_H_

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

  private:
    pcap_t* pcap_;
    struct bpf_program filter_;
};

class Wpa2Sniffer {
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

  private:
    // Build the PCAP filter to only get data packets for our BSSID.
    void SetBssidFilter();

    // Called by the PcapInterface when a packet is captured.
    void HandleCapturedPacket(const uint8_t* pkt, int len);

    PcapInterface pcap_;

    std::string bssid_;
    std::string psk_;

    // { src_port, protocol_handler }
    std::map<uint16_t, ProtocolHandler*> handlers_;
};

#endif
