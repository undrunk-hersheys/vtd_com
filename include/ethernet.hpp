#ifndef ETHERNET_HPP
#define ETHERNET_HPP

#include <cstdint>
#include <vector>
#include <cstring>

#define ETHERTYPE_IPV4 0x0800
#define ETHERTYPE_IPV6 0x86DD
#define ETHERTYPE_VLAN 0x8100

struct EthernetHeader 
{
    uint8_t dstMAC[6];        // Destination MAC address
    uint8_t srcMAC[6];        // Source MAC address
    uint16_t etherType;       // EtherType or VLAN tag identifier

    // Optional VLAN tag (802.1Q)
    bool hasVLAN = true;
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

uint16_t encodeVLAN(uint8_t pcp, bool dei, uint16_t vid);
uint8_t getPCP(uint16_t tci);
bool getDEI(uint16_t tci);
uint16_t getVID(uint16_t tci);

bool parseEthernetFrame(const uint8_t* data, size_t len,
                        EthernetHeader& hdr,
                        std::vector<uint8_t>& payload);


#endif // ETHERNET_HPP
