#pragma once
#include <array>
#include <cstdint>

struct EthernetHeader {
    std::array<uint8_t, 6> dst_mac;
    std::array<uint8_t, 6> src_mac;
    uint16_t ethertype;

    void writeTo(uint8_t* buf) const {
        std::copy(dst_mac.begin(), dst_mac.end(), buf);
        std::copy(src_mac.begin(), src_mac.end(), buf + 6);
        // for the upper 8 bits of the ethertype
        buf[12] = ethertype >> 8;
        // for the lower 8 bits of the ethertype
        buf[13] = ethertype & 0xff;
    }
};
