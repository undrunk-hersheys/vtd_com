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
    const char* TAP_NAME = "tap1";

    int tapFd = openTap(TAP_NAME);
    if (tapFd < 0) {
        std::cerr << "Failed to open TAP device.\n";
        return 1;
    }

    std::vector<uint8_t> buf(2048);

    while (true) {
        ssize_t n = read(tapFd, buf.data(), buf.size());
        if (n <= 0) { perror("read"); break; }

        /* ---------- L2 : Ethernet ---------- */
        EthernetHeader eth{};
        std::vector<uint8_t> l3;
        if (!parseEthernetFrame(buf.data(), n, eth, l3)) continue;

        uint16_t etype = eth.hasVLAN ? eth.innerEtherType : eth.etherType;
        if (etype != ETHERTYPE_IPV4) continue;          // not IPv4

        /* ---------- L3 : IPv4 -------------- */
        IPv4Header ip{};
        std::vector<uint8_t> l4;
        if (!parseIPv4Packet(l3.data(), l3.size(), ip, l4)) continue;
        if (ip.protocol != 17) continue;                // not UDP

        /* ---------- L4 : UDP --------------- */
        UDPHeader udp{};
        std::vector<uint8_t> app;
        if (!parseUDPSegment(l4.data(), l4.size(), udp, app,
                             ip.srcIP, ip.dstIP))       // checksum verify
            continue;

        /* ---------- Output ---------------- */
        std::string text(app.begin(), app.end());
        std::cout << "[UDP] "
                  << ntohs(udp.srcPort) << " -> "
                  << ntohs(udp.dstPort) << " | "
                  << "payload: " << text << '\n';
    }
    close(tapFd);
    return 0;
}
