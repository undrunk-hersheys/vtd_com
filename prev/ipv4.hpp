#pragma once
#include <cstdint>
#include <cstring>
#include <arpa/inet.h>


// IPv4 header structure

struct IPv4Header {
    uint8_t ver_ihl = 0x45;
    uint8_t dscp_ecn = 0;
    uint16_t total_length;
    uint16_t identification = htons(0x1234);
    uint16_t flags_offset = htons(0x4000);
    uint8_t ttl = 64;
    uint8_t protocol = 17;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dst_ip;

    void writeTo(uint8_t* buf) const {
        std::memset(buf, 0, 20);
        buf[0] = ver_ihl;
        buf[1] = dscp_ecn;
        buf[2] = total_length >> 8;
        buf[3] = total_length & 0xff;
        buf[4] = identification >> 8;
        buf[5] = identification & 0xff;
        buf[6] = flags_offset >> 8;
        buf[7] = flags_offset & 0xff;
        buf[8] = ttl;
        buf[9] = protocol;
        std::memcpy(buf + 12, &src_ip, 4);
        std::memcpy(buf + 16, &dst_ip, 4);

        // Calculate checksum
        uint16_t sum = 0;
        for (int i = 0; i < 20; i += 2)
            sum += (buf[i] << 8) | buf[i+1];
        while (sum >> 16) sum = (sum & 0xffff) + (sum >> 16);
        sum = ~sum;
        buf[10] = sum >> 8;
        buf[11] = sum & 0xff;
    }
};
