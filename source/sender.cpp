#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "ethernet.hpp"  // Layer-2 builder
#include "ipv4.hpp"      // Layer-3 builder
#include "udp.hpp"       // Layer-4 builder

int openTap(const char* devname) 
{
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        perror("open /dev/net/tun");
        return -1;
    }

    ifreq ifr{};
    std::strncpy(ifr.ifr_name, devname, IFNAMSIZ);
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return -1;
    }
    return fd;
}

int main() 
{
    const char* TAP_NAME = "tap0";

    const uint8_t SRC_MAC[6] = {0xde,0xad,0xbe,0xef,0x00,0x01};
    const uint8_t DST_MAC[6] = {0xde,0xad,0xbe,0xef,0x00,0x02};

    const uint32_t SRC_IP = inet_addr("10.0.0.1");  // tap0
    const uint32_t DST_IP = inet_addr("10.0.0.2");  // tap1

    const uint16_t SRC_PORT = 12345;
    const uint16_t DST_PORT = 54321;

    // application payload
    const char* msg = "Hello from TAP!";
    std::vector<uint8_t> payload(msg, msg + std::strlen(msg));

    // Layer4 UDP
    std::vector<uint8_t> udpPkt =
        buildUDPPacket(SRC_PORT, DST_PORT, SRC_IP, DST_IP, payload);

    // Layer3 IPv4
    std::vector<uint8_t> ipPkt =
        buildIPv4Packet(17 /* UDP */, SRC_IP, DST_IP, udpPkt, 64);

    // Layer4 Ethernet
    std::vector<uint8_t> etherFrame =
        buildEthernetFrame(DST_MAC, SRC_MAC, ETHERTYPE_IPV4, ipPkt);

    // TAP
    int tapFd = openTap(TAP_NAME);
    if (tapFd < 0) return 1;

    ssize_t n = write(tapFd, etherFrame.data(), static_cast<ssize_t>(etherFrame.size()));

    if (n < 0) {
        perror("write");
    } else {
        std::cout << "Sent " << n << " bytes on " << TAP_NAME << '\n';
    }
    close(tapFd);
    return 0;
}
