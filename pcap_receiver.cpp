#include <pcap.h>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>

void packet_handler(u_char* user, const struct pcap_pkthdr* h, const u_char* bytes) {
    // minimum length checker: Ethernet(14) + IPv4(20) + UDP(8)
    if (h->len < 42) return;

    // Ethernet header: 14bytes
    const u_char* ip_header = bytes + 14;
    uint8_t protocol = ip_header[9];
    if (protocol != 17) return; //ignore non-UDP packets

    // IPv4 header length
    uint8_t ihl = ip_header[0] & 0x0F;
    uint8_t ip_header_len = ihl * 4;

    const u_char* udp_header = ip_header + ip_header_len;
    uint16_t src_port = ntohs(*(uint16_t*)(udp_header + 0));
    uint16_t dst_port = ntohs(*(uint16_t*)(udp_header + 2));
    uint16_t udp_len = ntohs(*(uint16_t*)(udp_header + 4));

    const u_char* payload = udp_header + 8;
    int payload_len = udp_len - 8;
    if (payload_len <= 0) return;

    std::cout << "[UDP Packet] " << src_port << " -> " << dst_port << ", payload: ";
    for (int i = 0; i < payload_len; ++i) {
        std::cout << static_cast<char>(payload[i]);
    }
    std::cout << std::endl;
}


int main() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* handle = pcap_open_live("lo", 65536, 1, 1000, errbuf);
    if (!handle) {
        std::cerr << "pcap_open_live failed: " << errbuf << std::endl;
        return 1;
    }

    std::cout << "Listening on loopback interface for UDP packets..." << std::endl;

    // BPF filter
    // "udp and port 54321" means we only want to capture UDP packets
    // struct bpf_program fp;
    // if (pcap_compile(handle, &fp, "udp src port 12345 and dst port 54321", 0, PCAP_NETMASK_UNKNOWN) == -1) {
    //     std::cerr << "pcap_compile failed: " << pcap_geterr(handle) << std::endl;
    //     return 1;
    // }
    // if (pcap_setfilter(handle, &fp) == -1) {
    //     std::cerr << "pcap_setfilter failed: " << pcap_geterr(handle) << std::endl;
    //     return 1;
    // }

    pcap_loop(handle, 0, packet_handler, nullptr);

    pcap_close(handle);
    return 0;
}
