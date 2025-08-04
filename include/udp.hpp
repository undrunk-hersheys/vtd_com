#ifndef UDP_HPP
#define UDP_HPP

#include <cstdint>
#include <vector>
#include <cstring>

struct UDPHeader
{
    uint16_t srcPort;    //16bits
    uint16_t dstPort;    //16bits
    uint16_t length;     //16bits
    uint16_t checksum;   //16bits

    // serialize header into byte vector
    std::vector<uint8_t> toBytes() const;

    // parse header from byte stream
    static UDPHeader parse(const uint8_t* data, size_t size);
};

// Utility for checksum computation (pseudo-header + data)
uint16_t computeUDPChecksum
(
    const UDPHeader& udp,
    const uint8_t* payload,
    size_t payloadLen,
    uint32_t srcIP,
    uint32_t dstIP
);

// build full UDP packet (header + payload)
std::vector<uint8_t> buildUDPPacket
(
    uint16_t srcPort,
    uint16_t dstPort,
    uint32_t srcIP,
    uint32_t dstIP,
    const std::vector<uint8_t>& payload
);

#endif // UDP_HPP
