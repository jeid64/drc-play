#include "queue.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>

class ProtocolHandler;

struct Buffer {
    Buffer* next;
    uint8_t* data;
    int length;
};

class UdpSniffer{
  public:
    // Initializes a WPA2 sniffer to monitor access point <bssid> on interface
    // <iface>. The PSK used by the WPA2 network is specified by <psk>.
    UdpSniffer();

    virtual ~UdpSniffer();

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

  private:

    // Called by the PcapInterface when a packet is captured.
    void HandleCapturedPacket(const uint8_t* pkt, int len);

    // Handles decrypted packets from the shared queue. Parses the LLC, IP and
    // UDP headers to pass the contents to the right handler.
    void HandleReceivedPackets();

    // { src_port, protocol_handler }
    std::map<uint16_t, ProtocolHandler*> handlers_;

    // Queue for consumer-producer data exchange.
    LockedQueue<Buffer> queue_;
};
