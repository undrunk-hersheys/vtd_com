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
#include <chrono>

#include "ethernet.hpp"  // Layer-2 builder
#include "ipv4.hpp"      // Layer-3 builder
#include "udp.hpp"       // Layer-4 builder

uint64_t now_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
               std::chrono::steady_clock::now().time_since_epoch()).count();
}

struct __attribute__((packed)) TsHeader {
    uint64_t ts_us;
    uint32_t seq;
};

int openTap(const char* devname) 
{
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        perror("open /dev/net/tun");
        return -1;
    }

    ifreq ifr{};
    std::strncpy(ifr.ifr_name, devname, IFNAMSIZ);
    // ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI | IFF_MULTI_QUEUE;


    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return -1;
    }
    return fd;
}

int main()
{
    std::cout.setf(std::ios::unitbuf);

    const char* TAP_NAME = "tap1";

    int tapFd = openTap(TAP_NAME);
    if (tapFd < 0) {
        std::cerr << "Failed to open TAP device.\n";
        return 1;
    }

    std::vector<uint8_t> buf(2048); //2048

    uint64_t prev_latency = 0;

    while (true) {
        ssize_t n = read(tapFd, buf.data(), buf.size());
        if (n <= 0) { perror("read"); break; }

        // Layer2 Ethernet
        EthernetHeader eth{};
        std::vector<uint8_t> l3;
        if (!parseEthernetFrame(buf.data(), n, eth, l3)) 
            continue;

        uint16_t etype = eth.hasVLAN ? eth.innerEtherType : eth.etherType;
        if (etype != ETHERTYPE_IPV4) 
            continue;          // not IPv4

        if (eth.hasVLAN) {
            uint8_t pcp = getPCP(eth.vlanTCI);
            uint16_t vid = getVID(eth.vlanTCI);
            std::cout << "[VLAN] PCP=" << (int)pcp << " VID=" << vid << ",";
        }

        // Layer3 IPv4
        IPv4Header ip{};
        std::vector<uint8_t> l4;
        if (!parseIPv4Packet(l3.data(), l3.size(), ip, l4)) 
            continue;
        if (ip.protocol != 17) 
            continue;                // not UDP

        // Layer4 TCP/UDP
        UDPHeader udp{};
        std::vector<uint8_t> app;
        if (!parseUDPSegment(l4.data(), l4.size(), udp, app, ip.srcIP, ip.dstIP)) // checksum verify
            continue;


        if (app.size() < sizeof(TsHeader)) 
            continue;
        TsHeader h{};
        std::memcpy(&h, app.data(), sizeof(TsHeader));
        std::string text(app.begin() + sizeof(TsHeader), app.end());

        uint64_t recv_us = now_us();
        uint64_t latency = recv_us - h.ts_us;
        uint64_t jitter = prev_latency ? std::llabs((long long)(latency - prev_latency)) : 0;
        prev_latency = latency;

        // // Output
        // std::cout << "[UDP] " << ntohs(udp.srcPort) << " -> " << ntohs(udp.dstPort)
        //           << " | seq: " << h.seq
        //           << " | delay: " << latency << " us"
        //           << " | jitter: " << jitter << " us"
        // //           << " | payload: " << text 
        //           << '\n';
        std::cout << ntohs(udp.srcPort) << ","
          << ntohs(udp.dstPort) << ","
          << h.seq << ","
          << latency << ","
          << jitter
          << '\n';

    }
    close(tapFd);
    return 0;
}
