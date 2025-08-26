#pragma once
#include <string>
#include <array>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/if.h>

inline int open_tap(const char* ifname) {
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) throw std::runtime_error("open /dev/net/tun failed");
    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        close(fd);
        throw std::runtime_error("ioctl(TUNSETIFF) failed (is device created & up?)");
    }
    return fd;
}

inline std::array<uint8_t,6> get_if_mac(const char* ifname) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) throw std::runtime_error("socket() failed");
    struct ifreq ifr{};
    std::strncpy(ifr.ifr_name, ifname, IFNAMSIZ);
    if (ioctl(sock, SIOCGIFHWADDR, &ifr) < 0) {
        close(sock);
        throw std::runtime_error("SIOCGIFHWADDR failed");
    }
    close(sock);
    std::array<uint8_t,6> mac{};
    std::memcpy(mac.data(), ifr.ifr_hwaddr.sa_data, 6);
    return mac;
}

inline std::string mac_to_str(const std::array<uint8_t,6>& m) {
    char buf[18];
    std::snprintf(buf, sizeof(buf), "%02x:%02x:%02x:%02x:%02x:%02x",
                  m[0],m[1],m[2],m[3],m[4],m[5]);
    return std::string(buf);
}
