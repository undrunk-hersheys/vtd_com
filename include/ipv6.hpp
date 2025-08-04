#ifndef IPV6_HPP
#define IPV6_HPP

#include <cstdint>
#include <vector>
#include <cstddef>

// IPv6 header is fixed at 40 bytes
struct IPv6Header {
    uint32_t versionTrafficFlow;  // Version (4 bits) + Traffic Class (8 bits) + Flow Label (20 bits)
    uint16_t payloadLength;       // Length of the payload (excluding header)
    uint8_t nextHeader;           // Next protocol: e.g., TCP=6, UDP=17, ICMPv6=58
    uint8_t hopLimit;             // Replaces TTL from IPv4
    uint8_t srcIP[16];            // Source IPv6 address
    uint8_t dstIP[16];            // Destination IPv6 address

    // Serialize header into byte array
    std::vector<uint8_t> toBytes() const;

    // Parse IPv6 header from raw bytes
    static IPv6Header parse(const uint8_t* data, size_t size);
};

// Build a complete IPv6 packet (header + payload)
std::vector<uint8_t> buildIPv6Packet(
    uint8_t nextHeader,
    const uint8_t srcIP[16],
    const uint8_t dstIP[16],
    const std::vector<uint8_t>& payload,
    uint8_t hopLimit = 64,
    uint32_t flowLabel = 0,
    uint8_t trafficClass = 0
);

#endif // IPV6_HPP
