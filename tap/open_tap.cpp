#include <iostream>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <net/if.h>

int tun_alloc(const char *devname) {
    struct ifreq ifr;
    int fd, err;

    // TUN/TAP 장치 열기
    if ((fd = open("/dev/net/tun", O_RDWR)) < 0) {
        perror("open /dev/net/tun");
        return -1;
    }

    std::memset(&ifr, 0, sizeof(ifr));

    // 인터페이스 이름 지정 (예: tap0)
    strncpy(ifr.ifr_name, devname, IFNAMSIZ);

    // TAP 모드, no packet info
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI;

    // ioctl로 장치 설정
    if ((err = ioctl(fd, TUNSETIFF, (void *)&ifr)) < 0) {
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

    // 무한 루프: 프레임 수신 대기
    char buffer[1600];
    while (true) {
        int nread = read(fd, buffer, sizeof(buffer));
        if (nread < 0) {
            perror("Reading from TAP interface");
            break;
        }
        std::cout << "Read " << nread << " bytes from " << dev << std::endl;
    }

    close(fd);
    return 0;
}
