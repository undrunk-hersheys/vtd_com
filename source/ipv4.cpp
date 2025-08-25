#include <arpa/inet.h>
#include <cstring>

#include "ipv4.hpp"

static uint16_t onesComplement(const uint8_t* data, size_t len)
{
    uint32_t sum = 0;
    for (size_t i = 0; i + 1 < len; i += 2)
        sum += (data[i] << 8) | data[i + 1];
    if (len & 1) sum += data[len - 1] << 8;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return static_cast<uint16_t>(~sum);
}

uint16_t computeIPv4HeaderChecksum(const IPv4Header& h)
{
    return onesComplement(reinterpret_cast<const uint8_t*>(&h),
                          sizeof(IPv4Header));
}

std::vector<uint8_t> IPv4Header::toBytes(const std::vector<uint8_t>& payload) const
{
    IPv4Header hdr = *this;
    hdr.totalLength = htons(static_cast<uint16_t>(sizeof(IPv4Header) + payload.size()));
    hdr.headerChecksum = 0;
    hdr.headerChecksum = computeIPv4HeaderChecksum(hdr);

    std::vector<uint8_t> out(sizeof(IPv4Header) + payload.size());
    std::memcpy(out.data(), &hdr, sizeof(IPv4Header));
    std::memcpy(out.data() + sizeof(IPv4Header),
                payload.data(), payload.size());
    return out;
}

std::vector<uint8_t> buildIPv4Packet(uint8_t protocol,
                                     uint32_t srcIP,
                                     uint32_t dstIP,
                                     const std::vector<uint8_t>& payload,
                                     uint8_t ttl)
{
    IPv4Header hdr{};
    hdr.versionIHL = 0x45;
    hdr.typeOfService = 0;
    hdr.identification = htons(0x1234);
    hdr.flagsFragmentOffset = htons(0x4000); // DF flag
    hdr.ttl = ttl;
    hdr.protocol = protocol;
    hdr.srcIP = srcIP;
    hdr.dstIP = dstIP;
    hdr.headerChecksum = 0;

    return hdr.toBytes(payload);
}

bool parseIPv4Packet(const uint8_t* data,
                     size_t len,
                     IPv4Header& hdr,
                     std::vector<uint8_t>& payload)
{
    if (len < 20) return false;

    std::memcpy(&hdr, data, 20);        // copy first 20 bytes
    uint8_t ihl = hdr.versionIHL & 0x0F;
    if (ihl < 5) return false;          // invalid IHL
    size_t hdrLen = ihl * 4;
    if (len < hdrLen) return false;

    uint16_t totalLen = ntohs(hdr.totalLength);
    if (totalLen > len) return false;

    /* --- verify header checksum --- */
    IPv4Header temp;
    std::memcpy(&temp, data, hdrLen);
    temp.headerChecksum = 0;
    if (onesComplement(reinterpret_cast<uint8_t*>(&temp), hdrLen) !=
        hdr.headerChecksum)
        return false;                   // checksum fail


    // //* --- verify header checksum --- *//
    // IPv4Header temp;
    // std::memcpy(&temp, data, hdrLen);
    // temp.headerChecksum = 0;
    // uint16_t calc = onesComplement(reinterpret_cast<uint8_t*>(&temp), hdrLen);
    // if (calc != ntohs(hdr.headerChecksum))
    //     return false; // checksum fail

    /* --- payload extraction --- */
    size_t payloadLen = totalLen - hdrLen;
    payload.assign(data + hdrLen, data + hdrLen + payloadLen);
    return true;
}
