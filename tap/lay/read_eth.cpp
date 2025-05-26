#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <arpa/inet.h>

int tun_alloc(const char *devname) {
    struct ifreq ifr;
    int fd;

    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("open /dev/net/tun");
        return -1;
    }

    std::memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, devname, IFNAMSIZ);
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return -1;
    }

    std::cout << "TAP interface " << devname << " opened successfully." << std::endl;
    return fd;
}

int main() {
    const char *dev = "tap1";
    int fd = tun_alloc(dev);
    if (fd < 0) {
        std::cerr << "Failed to open TAP device." << std::endl;
        return 1;
    }

    char buffer[1600];
    while (true) {
        int nread = read(fd, buffer, sizeof(buffer));
        if (nread < 0) {
            perror("Reading from TAP interface");
            break;
        }

        std::cout << "Read " << nread << " bytes from " << dev << std::endl;

        if (nread < 42) continue; // Ethernet(14) + IP(20) + UDP(8)

        const uint8_t* eth = (uint8_t*)buffer;
        const uint8_t* ip = eth + 14;

        if (ip[9] != 17) continue; // Check protocol field: 17 = UDP

        uint8_t ihl = ip[0] & 0x0F;
        uint8_t ip_header_len = ihl * 4;

        const uint8_t* udp = ip + ip_header_len;
        uint16_t src_port = ntohs(*(uint16_t*)(udp));
        uint16_t dst_port = ntohs(*(uint16_t*)(udp + 2));
        uint16_t udp_len = ntohs(*(uint16_t*)(udp + 4));

        const uint8_t* payload = udp + 8;
        int payload_len = udp_len - 8;
        if (payload_len <= 0) continue;

        std::cout << "[UDP Packet] " << src_port << " -> " << dst_port << ", payload: ";
        for (int i = 0; i < payload_len; ++i) {
            std::cout << static_cast<char>(payload[i]);
        }
        std::cout << std::endl;
    }

    close(fd);
    return 0;
}
