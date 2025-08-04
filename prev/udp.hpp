#pragma once
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>

struct UDPHeader {
    uint16_t src_port;
    uint16_t dst_port;
    uint16_t length;
    uint16_t checksum = 0;

    void writeTo(uint8_t* buf) const {
        buf[0] = src_port >> 8;
        buf[1] = src_port & 0xff;
        buf[2] = dst_port >> 8;
        buf[3] = dst_port & 0xff;
        buf[4] = length >> 8;
        buf[5] = length & 0xff;
        buf[6] = 0;
        buf[7] = 0;
    }
};
