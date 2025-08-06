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

//     std::vector<uint8_t> buffer(1600);
//     while (true) {
//         ssize_t nread = read(tapFd, buffer.data(), buffer.size());
//         if (nread < 0) {
//             perror("read");
//             break;
//         }

//         if (nread < 42) // Ethernet(14) + IPv4(20) + UDP(8)
//             continue;

//         const uint8_t* eth = buffer.data();
//         uint16_t etherType = (eth[12] << 8) | eth[13];
//         if (etherType != ETHERTYPE_IPV4)
//             continue;

//         const uint8_t* ip = eth + 14;
//         uint8_t protocol = ip[9];
//         if (protocol != 17) // UDP
//             continue;

//         uint8_t ihl = ip[0] & 0x0F;
//         uint8_t ip_header_len = ihl * 4;
//         const uint8_t* udp = ip + ip_header_len;

//         uint16_t src_port = ntohs(*(uint16_t*)(udp));
//         uint16_t dst_port = ntohs(*(uint16_t*)(udp + 2));
//         uint16_t udp_len = ntohs(*(uint16_t*)(udp + 4));

//         const uint8_t* payload = udp + 8;
//         int payload_len = udp_len - 8;
//         if (payload_len <= 0)
//             continue;

//         std::cout << "Read " << nread << " bytes from " << TAP_NAME << "\n";
//         std::cout << "[UDP Packet] " << src_port << " -> " << dst_port << ", payload: ";
//         for (int i = 0; i < payload_len; ++i) {
//             std::cout << static_cast<char>(payload[i]);
//         }
//         std::cout << "\n";
//     }

//     close(tapFd);
//     return 0;
// }
