#include <iostream>
#include <vector>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/if_tun.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <getopt.h>


#include "ethernet.hpp"  // Layer-2 builder
#include "ipv4.hpp"      // Layer-3 builder
#include "udp.hpp"       // Layer-4 builder

bool parseMac(const std::string& s, uint8_t out[6]) {
    unsigned int b[6];
    if (std::sscanf(s.c_str(), "%02x:%02x:%02x:%02x:%02x:%02x",
                    &b[0],&b[1],&b[2],&b[3],&b[4],&b[5]) != 6) return false;
    for (int i=0;i<6;++i) out[i] = static_cast<uint8_t>(b[i]);
    return true;
}

void usage(const char* prog) {
    std::cerr <<
    "Usage: " << prog << " [options]\n"
    "  --tap NAME            TAP interface (default: tap0)\n"
    "  --src-mac MAC         Source MAC (default: 02:00:00:00:00:aa)\n"
    "  --dst-mac MAC         Destination MAC (REQUIRED)\n"
    "  --src-ip IP           Source IPv4 (default: 10.0.0.1)\n"
    "  --dst-ip IP           Destination IPv4 (default: 10.0.0.2)\n"
    "  --src-port N          UDP source port (default: 12345)\n"
    "  --dst-port N          UDP destination port (default: 54321)\n"
    "  --pcp N               VLAN PCP 0..7 (default: 0)\n"
    "  --vid N               VLAN ID 1..4094 (default: 1)\n"
    "  --payload BYTES       Payload bytes (default: 64)\n"
    "  --count N             Packet count, 0=run forever (default: 10)\n"
    "  --interval-us USEC    Interval between packets (default: 1000000)\n"
    "  --seq-start N         Start sequence number (default: 1)\n"
    "  --help                Show this help\n";
}

struct __attribute__((packed)) TsHeader {
    uint64_t ts_us;
    uint32_t seq;
};

uint64_t now_us() {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
}

int openTap(const char* devname) 
{
    int fd = open("/dev/net/tun", O_RDWR);
    if (fd < 0) {
        perror("open /dev/net/tun");
        return -1;
    }

    ifreq ifr{};
    std::strncpy(ifr.ifr_name, devname, IFNAMSIZ);
    // ifr.ifr_flags = IFF_TAP | IFF_NO_PI;
    ifr.ifr_flags = IFF_TAP | IFF_NO_PI | IFF_MULTI_QUEUE;

    if (ioctl(fd, TUNSETIFF, &ifr) < 0) {
        perror("ioctl(TUNSETIFF)");
        close(fd);
        return -1;
    }
    return fd;
}


int main(int argc, char** argv)
{
    std::string tapName   = "tap0";

    std::string srcIpStr  = "10.0.0.1";
    std::string dstIpStr  = "10.0.0.2";

    std::string srcMacStr = "de:ad:be:ef:00:01";
    std::string dstMacStr = "de:ad:be:ef:00:02";
    // std::string dstMacStr = "";

    uint16_t srcPort = 12345;
    uint16_t dstPort = 54321;

    // uint8_t  pcp     = 3;
    uint8_t pcp = 0;
    bool     dei     = false;
    uint16_t vid     = 1;

    size_t   payloadBytes = 64;
    uint64_t count = 10;  // 0 = infinite
    uint64_t intervalUs = 1000000;  // 1 MHz 1sec
    uint32_t seqStart = 1;

    static option longopts[] = 
    {
        {"tap",        required_argument, nullptr, 0},
        {"src-mac",    required_argument, nullptr, 0},
        {"dst-mac",    required_argument, nullptr, 0},
        {"src-ip",     required_argument, nullptr, 0},
        {"dst-ip",     required_argument, nullptr, 0},
        {"src-port",   required_argument, nullptr, 0},
        {"dst-port",   required_argument, nullptr, 0},
        {"pcp",        required_argument, nullptr, 0},
        {"vid",        required_argument, nullptr, 0},
        {"payload",    required_argument, nullptr, 0},
        {"count",      required_argument, nullptr, 0},
        {"interval-us",required_argument, nullptr, 0},
        {"seq-start",  required_argument, nullptr, 0},
        {"help",       no_argument,       nullptr, 0},
        {nullptr, 0, nullptr, 0}
    };

    int optidx = 0;

    while (true) 
    {
        int c = getopt_long(argc, argv, "", longopts, &optidx);
        if (c == -1) break;
        if (c == 0) {
            std::string key = longopts[optidx].name;
            if (key == "tap") tapName = optarg;
            else if (key == "src-mac") srcMacStr = optarg;
            else if (key == "dst-mac") dstMacStr = optarg;
            else if (key == "src-ip") srcIpStr = optarg;
            else if (key == "dst-ip") dstIpStr = optarg;
            else if (key == "src-port") srcPort = static_cast<uint16_t>(std::stoi(optarg));
            else if (key == "dst-port") dstPort = static_cast<uint16_t>(std::stoi(optarg));
            else if (key == "pcp")      pcp     = static_cast<uint8_t>(std::stoi(optarg)); // dei?
            else if (key == "vid")      vid     = static_cast<uint16_t>(std::stoi(optarg));
            else if (key == "payload")  payloadBytes = static_cast<size_t>(std::stoul(optarg));
            else if (key == "count")    count   = static_cast<uint64_t>(std::stoull(optarg));
            else if (key == "interval-us") intervalUs = static_cast<uint64_t>(std::stoull(optarg));
            else if (key == "seq-start") seqStart = static_cast<uint32_t>(std::stoul(optarg));
            else if (key == "help") { usage(argv[0]); return 0; } // payload data?
        } else { usage(argv[0]); return 1; }
    }

    if (dstMacStr.empty()) { std::cerr << "ERROR: --dst-mac is required\n"; usage(argv[0]); return 1; }
    if (pcp > 7) { std::cerr << "ERROR: --pcp must be 0..7\n"; return 1; }
    if (vid == 0 || vid > 4094) { std::cerr << "ERROR: --vid must be 1..4094\n"; return 1; }

    int tapFd = openTap(tapName.c_str());
    if (tapFd < 0) 
    { std::cerr << "Failed to open TAP device.\n"; return 1; }


    uint8_t srcMac[6], dstMac[6];
    if (!parseMac(srcMacStr, srcMac) || !parseMac(dstMacStr, dstMac)) 
    { std::cerr << "Invalid MAC format. Use aa:bb:cc:dd:ee:ff\n"; return 1; }

    uint32_t srcIp = inet_addr(srcIpStr.c_str());
    uint32_t dstIp = inet_addr(dstIpStr.c_str());
    if (srcIp == INADDR_NONE || dstIp == INADDR_NONE)
    { std::cerr << "Invalid IPv4 address\n"; return 1; }

    uint16_t tci = encodeVLAN(pcp, dei, vid);

    // application payload
    const char* msg = "-";
    size_t msglen = std::strlen(msg);
    // std::vector<uint8_t> payload(msg, msg + msglen);

    uint32_t seq = seqStart;


    // for (uint64_t sent = 0; (count == 0) || (sent < count); ++sent) {
    //     TsHeader header{ now_us(), seq++ };
    //     std::vector<uint8_t> payload(sizeof(header) + msglen);
    //     std::memcpy(payload.data(), &header, sizeof(header));   
    //     std::memcpy(payload.data() + sizeof(header), msg, msglen);

    //     // ===== L4: UDP =====
    //     std::vector<uint8_t> udp_packet = buildUDPPacket(srcPort, dstPort, srcIp, dstIp, payload);

    //     // ===== L3: IPv4 =====
    //     std::vector<uint8_t> ip_packet = buildIPv4Packet(UDP_PROTOCOL, srcIp, dstIp, udp_packet, 64);

    //     // ===== L2: Ethernet (+VLAN) =====
    //     std::vector<uint8_t> ethernet_frame = buildEthernetFrame(dstMac, srcMac, ETHERTYPE_IPV4, ip_packet, true, tci, 0);
    //     // std::vector<uint8_t> ethernet_frame = buildEthernetFrame(dstMac, srcMac, ETHERTYPE_IPV4, ip_packet, false, 0, 0);

    //     ssize_t n = write(tapFd, ethernet_frame.data(), static_cast<ssize_t>(ethernet_frame.size()));
        
    //     if (n < 0) {
    //         perror("write");
    //     } else {
    //         std::cout << "Sent " << n << " bytes on " << tapName << '\n';
    //     }

    //     if (intervalUs > 0) {
    //         std::this_thread::sleep_for(std::chrono::microseconds(intervalUs));
    //     }
    // }
    for (uint64_t sent = 0; (count == 0) || (sent < count); ++sent) {
        // 1) payloadBytes를 진짜로 반영
        TsHeader header{ now_us(), seq++ };

        // 헤더를 포함해서 정확히 --payload 바이트로 맞춤
        size_t totalBytes = payloadBytes;                      // 옵션으로 받은 값
        if (totalBytes < sizeof(header)) totalBytes = sizeof(header);

        std::vector<uint8_t> payload(totalBytes);
        // 타임스탬프/시퀀스 헤더
        std::memcpy(payload.data(), &header, sizeof(header));
        // 나머지 바이트는 패턴으로 채움(원하면 모두 0으로 채워도 됨)
        for (size_t i = sizeof(header); i < totalBytes; ++i)
            payload[i] = static_cast<uint8_t>(i);

        // 2) L4: UDP
        std::vector<uint8_t> udp_packet =
            buildUDPPacket(srcPort, dstPort, srcIp, dstIp, payload);

        // 3) L3: IPv4  (DF를 쓰지 않는 게 좋으면 ipv4.cpp에서 DF 비트를 끄세요)
        std::vector<uint8_t> ip_packet =
            buildIPv4Packet(UDP_PROTOCOL, srcIp, dstIp, udp_packet, 64);

        // 4) L2: Ethernet (+VLAN)
        std::vector<uint8_t> ethernet_frame =
            buildEthernetFrame(dstMac, srcMac, ETHERTYPE_IPV4, ip_packet, true, tci, 0);

        ssize_t n = write(tapFd, ethernet_frame.data(),
                        static_cast<ssize_t>(ethernet_frame.size()));
        if (n < 0) {
            perror("write");
        } else {
            std::cout << "Sent " << n << " bytes on " << tapName << '\n';
        }

        if (intervalUs > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(intervalUs));
        }
    }


    close(tapFd);
    return 0;
}
