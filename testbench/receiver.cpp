#include <iostream>
#include <vector>
#include <cstdint>
#include <unistd.h>
#include <arpa/inet.h>
#include "tap_util.hpp"

static constexpr uint16_t ETHERTYPE_TEST = 0x88B5; // arbitrary

int main() {
    const char* IFACE = "tap1";
    int fd = open_tap(IFACE);
    auto mymac = get_if_mac(IFACE);
    std::cout << "[recv] opened " << IFACE
              << " mac=" << mac_to_str(mymac) << "\n";

    std::vector<uint8_t> buf(2048);
    while (true) {
        ssize_t n = read(fd, buf.data(), buf.size());
        if (n <= 0) continue;
        if (n < 14) continue; // ether header

        auto* dst = &buf[0];
        auto* src = &buf[6];
        uint16_t ethertype = (buf[12] << 8) | buf[13];

        if (ethertype != ETHERTYPE_TEST) {
            continue; // ignore non-test frames
        }

        std::array<uint8_t,6> d{}, s{};
        std::memcpy(d.data(), dst, 6);
        std::memcpy(s.data(), src, 6);

        std::cout << "[recv] frame len=" << n
                  << " dst=" << mac_to_str(d)
                  << " src=" << mac_to_str(s)
                  << " ethertype=0x" << std::hex << ethertype << std::dec
                  << "\n";

        if (n > 14) {
            std::string payload(reinterpret_cast<char*>(&buf[14]), n - 14);
            std::cout << "       payload=\"" << payload << "\"\n";
        }
    }
    return 0;
}
