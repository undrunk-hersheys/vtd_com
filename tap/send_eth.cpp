#include <iostream>
#include <cstring>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <net/if.h>

// TAP 인터페이스 열기 함수
int tun_alloc(const char *devname) {
    std::string path = "/dev/net/tun";
    int fd = open(path.c_str(), O_RDWR);
    if (fd < 0) {
        perror("open /dev/net/tun");
        return -1;
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    strncpy(ifr.ifr_name, devname, IFNAMSIZ);
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    // TAP 장치 열기만 (만들지 않음)
    if (ioctl(fd, TUNSETIFF, (void *)&ifr) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return -1;
    }

    return fd;
}

int main() {
    const char *dev = "tap0";
    int fd = tun_alloc(dev);
    if (fd < 0) return 1;

    // 이더넷 프레임 생성
    uint8_t frame[60] = {0};  // 최소 Ethernet frame size: 60 bytes (padding 포함)

    // 목적지 MAC 주소 (broadcast)
    std::memset(frame, 0xff, 6);
    // 출발지 MAC 주소 (임의)
    frame[6] = 0x00; frame[7] = 0x11; frame[8] = 0x22;
    frame[9] = 0x33; frame[10] = 0x44; frame[11] = 0x55;
    // EtherType (0x0800 = IPv4)
    frame[12] = 0x08;
    frame[13] = 0x00;
    // Payload (임의 내용)
    const char *payload = "Hello from TAP!";
    std::memcpy(&frame[14], payload, std::strlen(payload));

    // 프레임 송신
    ssize_t written = write(fd, frame, sizeof(frame));
    if (written < 0) {
        perror("write");
    } else {
        std::cout << "Sent " << written << " bytes on " << dev << std::endl;
    }

    close(fd);
    return 0;
}
