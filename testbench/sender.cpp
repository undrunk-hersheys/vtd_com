#include <iostream>
#include <vector>
#include <array>
#include <cstdint>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <stdexcept>
#include "tap_util.hpp"

static constexpr uint16_t ETHERTYPE_TEST = 0x88B5; // arbitrary

int main(int argc, char** argv) {
    auto parse_mac = [](const char* s){
        std::array<uint8_t,6> m{};
        unsigned v[6];
        if (std::sscanf(s, "%x:%x:%x:%x:%x:%x",
                        &v[0],&v[1],&v[2],&v[3],&v[4],&v[5]) == 6) {
            for (int i = 0; i < 6; i++) m[i] = static_cast<uint8_t>(v[i]);
            return m;
        }
        throw std::runtime_error("bad MAC format (expect XX:XX:XX:XX:XX:XX)");
    };

    const char* IFACE   = "tap0";
    const char* PAYLOAD = (argc > 1) ? argv[1] : "HELLO_ETH";

    // 기본값: tap1 MAC. argv[2]로 덮어쓸 수 있음.
    std::array<uint8_t,6> dstmac = {0xde,0xad,0xbe,0xef,0x00,0x02};
    if (argc > 2) dstmac = parse_mac(argv[2]);

    int fd = open_tap(IFACE);
    auto srcmac = get_if_mac(IFACE);

    std::cout << "[send] opened " << IFACE
              << " src=" << mac_to_str(srcmac)
              << " dst=" << mac_to_str(dstmac)
              << " ethertype=0x" << std::hex << ETHERTYPE_TEST << std::dec
              << "\n";

    std::string pl(PAYLOAD);
    size_t framelen = 14 + pl.size();
    if (framelen < 60) framelen = 60; // Ethernet 최소 프레임 길이(패딩)

    std::vector<uint8_t> frame(framelen, 0);
    // dst/src mac
    std::memcpy(&frame[0], dstmac.data(), 6);
    std::memcpy(&frame[6], srcmac.data(), 6);
    // ethertype
    frame[12] = static_cast<uint8_t>((ETHERTYPE_TEST >> 8) & 0xFF);
    frame[13] = static_cast<uint8_t>(ETHERTYPE_TEST & 0xFF);
    // payload (패딩 나머지는 0)
    std::memcpy(&frame[14], pl.data(), pl.size());

    ssize_t n = write(fd, frame.data(), frame.size());
    if (n < 0) {
        perror("write");
        return 1;
    }
    if (static_cast<size_t>(n) != frame.size()) {
        std::cerr << "[send] short write: " << n
                  << " / " << frame.size() << " bytes\n";
    } else {
        std::cout << "[send] wrote " << n << " bytes\n";
    }
    return 0;
}
