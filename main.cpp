#include <iostream>
#include <pcap.h>
#include <cstring>
#include "ethernet.hpp"
#include "ipv4.hpp"
#include "udp.hpp"

int main() {
    char errbuf[PCAP_ERRBUF_SIZE];                                  // Error buffer for pcap functions
    pcap_t* handle = pcap_open_live("lo", 65536, 1, 1000, errbuf);  // Loopback interface 
    if (!handle) {                  
        std::cerr << "pcap_open_live failed: " << errbuf << std::endl;
        return 1;
    }

    uint8_t packet[1500] = {};
    int offset = 0;

    EthernetHeader eth {
        .dst_mac = {0x00,1,2,3,4,5},
        .src_mac = {0x06,7,8,9,0xa,0xb},
        .ethertype = 0x0800
    };
    eth.writeTo(packet + offset); offset += 14;

    IPv4Header ip {
        .total_length = htons(20 + 8 + 5),
        .src_ip = inet_addr("127.0.0.1"),
        .dst_ip = inet_addr("127.0.0.1")
    };
    ip.writeTo(packet + offset); offset += 20;

    UDPHeader udp {
        .src_port = htons(12345),
        .dst_port = htons(54321),
        .length = htons(8 + 5)
    };
    udp.writeTo(packet + offset); offset += 8;

    const char* data = "Hello";
    std::memcpy(packet + offset, data, 5); offset += 5;

    if (pcap_sendpacket(handle, packet, offset) != 0) {
        std::cerr << "send failed: " << pcap_geterr(handle) << std::endl;
    } else {
        std::cout << "Packet sent." << std::endl;
    }

    pcap_close(handle);
    return 0;
}
