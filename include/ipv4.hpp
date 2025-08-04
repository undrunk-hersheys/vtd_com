#ifndef IPV4_HPP
#define IPV4_HPP

#include <cstdint>
#include <vector>
#include <cstring>

// IPv4 header structure (20 bytes, no options)
struct IPv4Header {
    uint8_t versionIHL;            // 4 bits version + 4 bits IHL header length (usually 0x45)
    uint8_t typeOfService;         // DSCP + ECN
    uint16_t totalLength;          // Total length: header + payload
    uint16_t identification;       // Identification field for fragmentation
    uint16_t flagsFragmentOffset;  // Flags (3 bits) + Fragment Offset (13 bits)
    uint8_t ttl;                   // Time to live
    uint8_t protocol;              // Protocol: TCP=6, UDP=17
    uint16_t headerChecksum;       // Header checksum (only header, not payload)
    uint32_t srcIP;                // Source IP address
    uint32_t dstIP;                // Destination IP address

    // Serialize header into a byte array (network byte order)
    std::vector<uint8_t> toBytes() const;

    // Parse an IPv4 header from raw bytes
    static IPv4Header parse(const uint8_t* data, size_t size);
};

// Compute 16-bit checksum for the IPv4 header
uint16_t computeIPv4HeaderChecksum(const IPv4Header& header);

// Construct a complete IPv4 packet (header + payload)
std::vector<uint8_t> buildIPv4Packet(
    uint8_t protocol,                     // e.g., TCP=6, UDP=17
    uint32_t srcIP,
    uint32_t dstIP,
    const std::vector<uint8_t>& payload,
    uint8_t ttl = 64
);

#endif // IPV4_HPP
