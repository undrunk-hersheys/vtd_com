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
#include <thread>

#include "ethernet.hpp"  // Layer-2 builder
#include "ipv4.hpp"      // Layer-3 builder
#include "udp.hpp"       // Layer-4 builder

struct __attribute__((packed)) TsHeader {
    uint64_t ts_us;
    uint32_t seq;
};

uint64_t now_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

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
    uint8_t  pcp = 3;
    bool     dei = false;
    uint16_t vid = 1;
    uint16_t tci = encodeVLAN(pcp, dei, vid);

    const uint32_t SRC_IP = inet_addr("10.0.0.1");  // tap0
    const uint32_t DST_IP = inet_addr("10.0.0.2");  // tap1

    const uint16_t SRC_PORT = 12345;
    const uint16_t DST_PORT = 33333;

    // application payload
    const char* msg = "-";
    size_t msglen = std::strlen(msg);
    // std::vector<uint8_t> payload(msg, msg + msglen);

    static uint32_t seq = 1;
    const int SEND_COUNT = 10;
    const int INTERVAL_MS = 1000; // 1000ms = 1s

    // TAP
    int tapFd = openTap(TAP_NAME);
    if (tapFd < 0) return 1;

    for (int i = 0; i < SEND_COUNT; ++i) 
    {

    // chrono
    TsHeader header{ now_us(), seq++ };
    std::vector<uint8_t> payload(sizeof(header) + msglen);
    std::memcpy(payload.data(), &header, sizeof(header));
    std::memcpy(payload.data() + sizeof(header), msg, msglen);


    // Layer4 TCP/UDP
    std::vector<uint8_t> udp_packet = buildUDPPacket(SRC_PORT, DST_PORT, SRC_IP, DST_IP, payload);

    // Layer3 IPv4
    std::vector<uint8_t> ip_packet = buildIPv4Packet(UDP_PROTOCOL, SRC_IP, DST_IP, udp_packet, 64);

    // Layer2 Ethernet
    // std::vector<uint8_t> ethernet_frame = buildEthernetFrame(DST_MAC, SRC_MAC, ETHERTYPE_IPV4, ip_packet, false, 0, 0);
    std::vector<uint8_t> ethernet_frame = buildEthernetFrame(DST_MAC, SRC_MAC, ETHERTYPE_IPV4, ip_packet, true, tci, 0);

    ssize_t n = write(tapFd, ethernet_frame.data(), static_cast<ssize_t>(ethernet_frame.size()));

    if (n < 0) {
        perror("write");
    } else {
        std::cout << "Sent " << n << " bytes on " << TAP_NAME << '\n';
    }

        std::this_thread::sleep_for(std::chrono::milliseconds(INTERVAL_MS));
    }

    close(tapFd);
    return 0;
}
