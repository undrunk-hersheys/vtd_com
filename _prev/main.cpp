#include <iostream>
#include <pcap.h>
#include <cstring>
#include "ethernet.hpp"
#include "ipv4.hpp"
#include "udp.hpp"


int main() {
    // pcap initialization
    char errbuf[PCAP_ERRBUF_SIZE];                                  // Error buffer for pcap functions
    pcap_t* handle = pcap_open_live("lo", 65536, 1, 1000, errbuf);  // Loopback interface 
    if (!handle) {                  
        std::cerr << "pcap_open_live failed: " << errbuf << std::endl;
        return 1;
    }

    // Ethernet Maximum Transmission Unit (MTU)
    // Max Pyaload that Layer2 can send
    // 1500 bytes for Ethernet
    uint8_t packet[1500] = {};
    int offset = 0;

    // Ethernet header
    EthernetHeader eth {
        // Destination MAC address
        .dst_mac = {0x00,0x01,0x02,0x03,0x04,0x05},
        // Source MAC address
        .src_mac = {0x06,0x07,0x08,0x09,0x0a,0x0b},
        // EtherType for IPv4
        // 0x0800 for IPv4,
        // 0x86DD for IPv6,
        // 0x0806 for ARP, etc.
        .ethertype = 0x0800
    };
    eth.writeTo(packet + offset); 
    offset += 14;
    // packet = [ Ethernet Header ][ IPv4 Header ][ UDP Header ][ Data ]

    // IPv4 header
    IPv4Header ip {
        .total_length = htons(20 + 8 + 5),
        .src_ip = inet_addr("127.0.0.1"),
        .dst_ip = inet_addr("127.0.0.1")
    };
    ip.writeTo(packet + offset);
    offset += 20;

    // UDP header
    UDPHeader udp {
        .src_port = htons(12345),
        .dst_port = htons(54321),
        .length = htons(8 + 5)
    };
    udp.writeTo(packet + offset);
    offset += 8;

    // Payload
    const char* data = "Hello";
    std::memcpy(packet + offset, data, 5);
    offset += 5;

    if (pcap_sendpacket(handle, packet, offset) != 0) {
        std::cerr << "send failed: " << pcap_geterr(handle) << std::endl;
    } else {
        std::cout << "Packet sent." << std::endl;
    }

    pcap_close(handle);
    return 0;
}
