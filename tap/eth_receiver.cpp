#include <pcap.h>
#include <iostream>
#include <iomanip>

void handler(u_char*, const struct pcap_pkthdr* h, const u_char* bytes) {
    if (h->len < 14) return;

    std::cout << "[Ethernet Frame] len = " << h->len << std::endl;

    std::cout << "Dst MAC: ";
    for (int i = 0; i < 6; ++i)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i] << (i < 5 ? ":" : "\n");

    std::cout << "Src MAC: ";
    for (int i = 6; i < 12; ++i)
        std::cout << std::hex << std::setw(2) << std::setfill('0') << (int)bytes[i] << (i < 11 ? ":" : "\n");

    std::cout << "Ethertype: 0x"
              << std::hex << ((bytes[12] << 8) | bytes[13]) << std::endl;

    std::cout << "Payload: ";
    for (int i = 14; i < h->len; ++i)
        std::cout << (char)(std::isprint(bytes[i]) ? bytes[i] : '.');
    std::cout << std::endl << std::dec;
}

int main() {
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* handle = pcap_open_live("tap0", 65536, 0, 1000, errbuf);
    if (!handle) {
        std::cerr << "Failed to open tap0: " << errbuf << std::endl;
        return 1;
    }

    std::cout << "Listening on tap0 (Ethernet Layer only)..." << std::endl;
    pcap_loop(handle, 0, handler, nullptr);
    pcap_close(handle);
    return 0;
}
