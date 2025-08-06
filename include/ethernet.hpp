#ifndef ETHERNET_HPP
#define ETHERNET_HPP

#include <cstdint>
#include <vector>
#include <cstring>

constexpr uint16_t ETHERTYPE_IPV4 = 0x0800;
constexpr uint16_t ETHERTYPE_IPV6 = 0x86DD;
constexpr uint16_t ETHERTYPE_VLAN = 0x8100;

// Ethernet frame (with optional VLAN tag)
struct EthernetHeader 
{
    uint8_t dstMAC[6];        // Destination MAC address
    uint8_t srcMAC[6];        // Source MAC address
    uint16_t etherType;       // EtherType or VLAN tag identifier

    // Optional VLAN tag (802.1Q)
    bool hasVLAN = false;
    uint16_t vlanTCI = 0;     // Tag Control Information: PCP(3b), DEI(1b), VID(12b)
    uint16_t innerEtherType = 0; // EtherType after VLAN

    // Serialize Ethernet header (with or without VLAN)
    std::vector<uint8_t> toBytes(
        const std::vector<uint8_t>& payload
    ) const;

    // Parse Ethernet header (detects VLAN if present)
    static EthernetHeader parse(const uint8_t* data, size_t size);
};

// Build a complete Ethernet frame (header + payload)
std::vector<uint8_t> buildEthernetFrame
(
    const uint8_t dstMAC[6],
    const uint8_t srcMAC[6],
    uint16_t etherType,
    const std::vector<uint8_t>& payload,
    bool addVLAN = false,
    uint16_t vlanTCI = 0,
    uint16_t innerType = 0
);

bool parseEthernetFrame(const uint8_t* data, size_t len,
                        EthernetHeader& hdr,
                        std::vector<uint8_t>& payload);


#endif // ETHERNET_HPP
