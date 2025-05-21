#include <iostream>
#include <pcap.h>
#include <cstring>

int main() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* handle = pcap_open_live("tap0", 65536, 0, 1000, errbuf);
    if (!handle) {
        std::cerr << "Failed to open tap0: " << errbuf << std::endl;
        return 1;
    }

    uint8_t packet[64] = {};
    int offset = 0;

    // Ethernet Header (14 bytes)
    uint8_t dst_mac[6] = {0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};
    uint8_t src_mac[6] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
    uint16_t ethertype = htons(0x88B5); // Custom Ethertype

    std::memcpy(packet + offset, dst_mac, 6); offset += 6;
    std::memcpy(packet + offset, src_mac, 6); offset += 6;
    std::memcpy(packet + offset, &ethertype, 2); offset += 2;

    // Payload
    const char* message = "TAP test!";
    std::memcpy(packet + offset, message, strlen(message));
    offset += strlen(message);

    if (pcap_sendpacket(handle, packet, offset) != 0) {
        std::cerr << "send failed: " << pcap_geterr(handle) << std::endl;
    } else {
        std::cout << "Ethernet frame sent on tap0." << std::endl;
    }

    pcap_close(handle);
    return 0;
}
