#ifndef TCP_HPP
#define TCP_HPP

#include <cstdint>
#include <vector>
#include <cstring>

struct TCPHeader 
{
    uint16_t srcPort;    // 16 bits
    uint16_t dstPort;    // 16 bits
    uint32_t seqNumber;  // 32 bits
    uint32_t ackNumber;  // 32 bits

    uint8_t  dataOffset; // upper 4 bits (header length)
                         // data offset 4, reserved 3, ns 1, flag 8
    uint8_t  flags;      // lower 6 bits: URG, ACK, PSH, RST, SYN, FIN
                         
    uint16_t windowSize; // 16 bits
    uint16_t checksum;   // 16 bits
    uint16_t urgentPointer; // 16 bits
    // paddings

    // serialize TCP header (minimum 20 bytes)
    std::vector<uint8_t> toBytes() const;

    // parse TCP header from byte buffer
    static TCPHeader parse(const uint8_t* data, size_t size);
};

// TCP pseudo-header checksum
uint16_t computeTCPChecksum
(
    const TCPHeader& tcp,
    const uint8_t* payload,
    size_t payloadLen,
    uint32_t srcIP,
    uint32_t dstIP
);

// Build full TCP segment
std::vector<uint8_t> buildTCPSegment
(
    uint16_t srcPort,
    uint16_t dstPort,
    uint32_t seq,
    uint32_t ack,
    uint8_t flags,
    uint16_t window,
    uint32_t srcIP,
    uint32_t dstIP,
    const std::vector<uint8_t>& payload
);

#endif // TCP_HPP
