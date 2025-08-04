#include <arpa/inet.h>
#include <cstring>

#include "ethernet.hpp"

std::vector<uint8_t> EthernetHeader::toBytes(const std::vector<uint8_t>& payload) const
{
    size_t hdrLen = hasVLAN ? 18 : 14;
    std::vector<uint8_t> frame(hdrLen + payload.size());

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
    EthernetHeader h{};
    std::memcpy(h.dstMAC, dst, 6);
    std::memcpy(h.srcMAC, src, 6);
    h.etherType = etherType;
    h.hasVLAN = addVLAN;
    h.vlanTCI = vlanTCI;
    h.innerEtherType = innerType;
    return h.toBytes(payload);
}
