#include <arpa/inet.h>
#include <cstring>

#include "ethernet.hpp"

std::vector<uint8_t> EthernetHeader::toBytes(const std::vector<uint8_t>& payload) const
{
    size_t header_length = hasVLAN ? 18 : 14;
    std::vector<uint8_t> frame(header_length + payload.size());

    uint8_t* p = frame.data();
    std::memcpy(p, dstMAC, 6); p += 6;
    std::memcpy(p, srcMAC, 6); p += 6;

    if (hasVLAN) {
        uint16_t tpid = htons(ETHERTYPE_VLAN);
        std::memcpy(p, &tpid, 2); p += 2;
        uint16_t tci = htons(vlanTCI);
        std::memcpy(p, &tci, 2); p += 2;
        uint16_t inner = htons(innerEtherType);
        std::memcpy(p, &inner, 2); p += 2;
    } else {
        uint16_t et = htons(etherType);
        std::memcpy(p, &et, 2); p += 2;
    }

    std::memcpy(p, payload.data(), payload.size());
    return frame;
}

std::vector<uint8_t> buildEthernetFrame(const uint8_t dst[6],
                                        const uint8_t src[6],
                                        uint16_t etherType,
                                        const std::vector<uint8_t>& payload,
                                        bool addVLAN,
                                        uint16_t vlanTCI,
                                        uint16_t innerType)
{
    EthernetHeader ether_header{};
    std::memcpy(ether_header.dstMAC, dst, 6);
    std::memcpy(ether_header.srcMAC, src, 6);
    ether_header.etherType = etherType; // ipv4 0x0800 / ipv6 0x86DD / arp 0x0806 ..
    ether_header.hasVLAN = addVLAN;
    ether_header.vlanTCI = vlanTCI;
    ether_header.innerEtherType = innerType;
    return ether_header.toBytes(payload);
}


static uint16_t getU16(const uint8_t* p)
{
    uint16_t v; std::memcpy(&v, p, 2); return ntohs(v);
}

bool parseEthernetFrame(const uint8_t* data,
                        size_t len,
                        EthernetHeader& hdr,
                        std::vector<uint8_t>& payload)
{
    if (len < 14) return false;               // minimal frame

    std::memcpy(hdr.dstMAC, data, 6);
    std::memcpy(hdr.srcMAC, data + 6, 6);
    hdr.etherType = getU16(data + 12);

    size_t offset = 14;

    if (hdr.etherType == ETHERTYPE_VLAN) {
        if (len < 18) return false;           // need TPID+TCI+innerType
        hdr.hasVLAN        = true;
        hdr.vlanTCI        = getU16(data + 14);
        hdr.innerEtherType = getU16(data + 16);
        offset = 18;
    } else {
        hdr.hasVLAN        = false;
        hdr.vlanTCI        = 0;
        hdr.innerEtherType = 0;
    }

    if (offset > len) return false;
    payload.assign(data + offset, data + len);
    return true;
}
