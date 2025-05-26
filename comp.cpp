#include <iostream>
#include <pcap.h>
#include <cstring>
#include <arpa/inet.h>
#include "ethernet.hpp"
#include "ipv4.hpp"
#include "udp.hpp"

int main() {
    char errbuf[PCAP_ERRBUF_SIZE];
    const char* dev = "tap0";  // 송신용 TAP 인터페이스
    pcap_t* handle = pcap_open_live(dev, 65536, 1, 1000, errbuf);
    if (!handle) {
        std::cerr << "pcap_open_live failed: " << errbuf << std::endl;
        return 1;
    }

    uint8_t packet[1500] = {};
    int offset = 0;

    // tap0에서 보내는 MAC (임의값, tap1이 이걸 수신해야 하므로 dst_mac 설정 중요)
    EthernetHeader eth {
        .dst_mac = {0xde, 0xad, 0xbe, 0xef, 0x00, 0x02}, // tap1 MAC (수신측)
        .src_mac = {0xde, 0xad, 0xbe, 0xef, 0x00, 0x01}, // tap0 MAC
        .ethertype = 0x0800
    };
    eth.writeTo(packet + offset);
    offset += 14;

    IPv4Header ip {
        .total_length = htons(20 + 8 + 5),
        .src_ip = inet_addr("10.0.0.1"), // tap0 IP
        .dst_ip = inet_addr("10.0.0.2")  // tap1 IP
    };
    ip.writeTo(packet + offset);
    offset += 20;

    UDPHeader udp {
        .src_port = htons(12345),
        .dst_port = htons(54321),
        .length = htons(8 + 5)
    };
    udp.writeTo(packet + offset);
    offset += 8;

    const char* data = "Hello";
    std::memcpy(packet + offset, data, 5);
    offset += 5;

    if (pcap_sendpacket(handle, packet, offset) != 0) {
        std::cerr << "send failed: " << pcap_geterr(handle) << std::endl;
    } else {
        std::cout << "Packet sent on " << dev << std::endl;
    }

    pcap_close(handle);
    return 0;
}
