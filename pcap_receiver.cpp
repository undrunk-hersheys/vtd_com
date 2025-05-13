#include <pcap.h>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>

void packet_handler(u_char* user, const struct pcap_pkthdr* h, const u_char* bytes) {
    // 최소 길이 체크: Ethernet(14) + IPv4(20) + UDP(8)
    if (h->len < 42) return;

    // Ethernet 헤더: 14바이트
    const u_char* ip_header = bytes + 14;
    uint8_t protocol = ip_header[9];
    if (protocol != 17) return; // UDP가 아니면 무시

    // IPv4 헤더 길이
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

    pcap_loop(handle, 0, packet_handler, nullptr);

    pcap_close(handle);
    return 0;
}
