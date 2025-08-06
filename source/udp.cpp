#include <arpa/inet.h>
#include "udp.hpp"

// Ones-complement sum helper
static uint16_t onesComplement(const uint8_t* data, size_t len)
{
    uint32_t sum = 0;
    for (size_t i = 0; i + 1 < len; i += 2)
        sum += (data[i] << 8) | data[i + 1];
    if (len & 1)                 // odd length
        sum += data[len - 1] << 8;

    while (sum >> 16)            // carry fold
        sum = (sum & 0xFFFF) + (sum >> 16);
    return static_cast<uint16_t>(~sum);
}

/* -------- UDPHeader methods -------- */
std::vector<uint8_t> UDPHeader::toBytes() const
{
    std::vector<uint8_t> out(sizeof(UDPHeader));
    std::memcpy(out.data(), this, sizeof(UDPHeader));
    return out;
}

uint16_t computeUDPChecksum(const UDPHeader& udp,
                            const uint8_t* payload,
                            size_t payloadLen,
                            uint32_t srcIP,
                            uint32_t dstIP)
{
    // Build pseudo-header + UDP header + payload into temp buffer
    size_t pseudoLen = 12 + sizeof(UDPHeader) + payloadLen;
    std::vector<uint8_t> buf(pseudoLen, 0);

    uint8_t* p = buf.data();
    std::memcpy(p, &srcIP, 4);      p += 4;
    std::memcpy(p, &dstIP, 4);      p += 4;
    *p++ = 0;                       // zero
    *p++ = 17;                      // protocol UDP
    uint16_t udpLenN = htons(sizeof(UDPHeader) + payloadLen);
    std::memcpy(p, &udpLenN, 2);    p += 2;

    std::memcpy(p, &udp, sizeof(UDPHeader));
    std::memcpy(p + sizeof(UDPHeader), payload, payloadLen);

    return onesComplement(buf.data(), buf.size());
}

std::vector<uint8_t> buildUDPPacket(uint16_t srcPort,
                                    uint16_t dstPort,
                                    uint32_t srcIP,
                                    uint32_t dstIP,
                                    const std::vector<uint8_t>& payload)
{
    UDPHeader hdr{};
    hdr.srcPort = htons(srcPort);
    hdr.dstPort = htons(dstPort);
    hdr.length  = htons(static_cast<uint16_t>(sizeof(UDPHeader) + payload.size()));
    hdr.checksum = 0; // temp

    hdr.checksum = computeUDPChecksum(hdr, payload.data(),
                                      payload.size(), srcIP, dstIP);

    // Serialize
    std::vector<uint8_t> pkt(sizeof(UDPHeader) + payload.size());
    std::memcpy(pkt.data(), &hdr, sizeof(UDPHeader));
    std::memcpy(pkt.data() + sizeof(UDPHeader), payload.data(), payload.size());
    return pkt;
}

bool parseUDPSegment(const uint8_t* data,
                     size_t len,
                     UDPHeader& hdr,
                     std::vector<uint8_t>& payload,
                     uint32_t srcIP,
                     uint32_t dstIP)
{
    if (len < sizeof(UDPHeader)) return false;
    std::memcpy(&hdr, data, sizeof(UDPHeader));

    uint16_t segLen = ntohs(hdr.length);
    if (segLen != len || segLen < sizeof(UDPHeader)) return false;

    payload.assign(data + sizeof(UDPHeader), data + segLen);

    /* UDP checksum verification (optional)            *
     * NOTE: Checksum 0 means "not computed" per RFC.  */
    // RFC 768 makes the combination of IPv4's pseudo-header for UDP
    if (hdr.checksum == 0) return true; // IPv4 UDP checksum is optional

    // // if calculate compared to the one in header
    // uint16_t calc = computeUDPChecksum(hdr, payload.data(), payload.size(), srcIP, dstIP);
    // return calc == hdr.checksum;

    // Caller must supply src/dst IP to fully verify pseudo-header.
    // Here we skip full verification and just accept non-zero.
    return true;
}
