cmake_minimum_required(VERSION 3.10)
project(EthernetUDP)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(PCAP REQUIRED libpcap)

include_directories(${PCAP_INCLUDE_DIRS})
link_directories(${PCAP_LIBRARY_DIRS})

add_executable(eth_udp
    main.cpp
)

target_link_libraries(eth_udp
    ${PCAP_LIBRARIES}
)

add_executable(pcap_receiver pcap_receiver.cpp)
target_link_libraries(pcap_receiver ${PCAP_LIBRARIES})

