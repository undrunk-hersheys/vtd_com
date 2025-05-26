#include <iostream>
#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <arpa/inet.h>

// Ethernet header: 14 bytes
struct EthernetHeader {
    uint8_t dst_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed));

// IPv4 header: 20 bytes
struct IPv4Header {
    uint8_t ver_ihl = 0x45;
    uint8_t dscp_ecn = 0;
    uint16_t total_length;
    uint16_t identification = htons(0x1234);
    uint16_t flags_offset = htons(0x4000);
    uint8_t ttl = 64;
    uint8_t protocol = 17; // UDP
    uint16_t checksum = 0;
    uint32_t src_ip;
    uint32_t dst_ip;
} __attribute__((packed));

// UDP header: 8 bytes
struct UDPHeader {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum = 0;
} __attribute__((packed));

// TAP 인터페이스 열기 함수
int tun_alloc(const char* devname) {
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        perror("open /dev/net/tun");
        return -1;
    }

    struct ifreq ifr {};
    std::strncpy(ifr.ifr_name, devname, IFNAMSIZ);
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    if (ioctl(fd, TUNSETIFF, (void*)&ifr) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return -1;
    }

    return fd;
}

uint16_t calculate_ip_checksum(const void* vdata, size_t length) {
    const uint8_t* data = (const uint8_t*)vdata;
    uint32_t sum = 0;

    for (size_t i = 0; i < length; i += 2) {
        uint16_t word = data[i] << 8;
        if (i + 1 < length)
            word |= data[i + 1];
        sum += word;
    }

    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}

int main() {
    const char* dev = "tap0";
    int fd = tun_alloc(dev);
    if (fd < 0) return 1;

    uint8_t packet[1500] = {};
    int offset = 0;

    // Ethernet Header
    EthernetHeader eth = {
        .dst_mac = {0xde, 0xad, 0xbe, 0xef, 0x00, 0x02}, // tap1 MAC
        .src_mac = {0xde, 0xad, 0xbe, 0xef, 0x00, 0x01}, // tap0 MAC
        .ethertype = htons(0x0800)                      // IPv4
    };
    std::memcpy(packet + offset, &eth, sizeof(eth));
    offset += sizeof(eth);

    // IPv4 Header
    IPv4Header ip;
    ip.total_length = htons(sizeof(IPv4Header) + sizeof(UDPHeader) + 17);
    ip.src_ip = inet_addr("10.0.0.1"); // tap0 IP
    ip.dst_ip = inet_addr("10.0.0.2"); // tap1 IP
    ip.checksum = 0;
    ip.checksum = calculate_ip_checksum(&ip, sizeof(IPv4Header));
    std::memcpy(packet + offset, &ip, sizeof(ip));
    offset += sizeof(ip);

    // UDP Header
    UDPHeader udp;
    udp.src_port = htons(12345);
    udp.dst_port = htons(54321);
    udp.length = htons(sizeof(UDPHeader) + 17);
    std::memcpy(packet + offset, &udp, sizeof(udp));
    offset += sizeof(udp);

    // Payload
    const char* payload = "Hello from TAP!";
    std::memcpy(packet + offset, payload, 17);
    offset += 17;

    // 전송
    ssize_t written = write(fd, packet, offset);
    if (written < 0) {
        perror("write");
    } else {
        std::cout << "Sent " << written << " bytes on " << dev << std::endl;
    }

    close(fd);
    return 0;
}
