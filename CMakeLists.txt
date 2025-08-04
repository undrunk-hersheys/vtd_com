cmake_minimum_required(VERSION 3.10)
project(vtd_com)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files
set(SENDER_SOURCES
    ${CMAKE_SOURCE_DIR}/source/sender.cpp

    ${CMAKE_SOURCE_DIR}/source/ethernet.cpp
    ${CMAKE_SOURCE_DIR}/source/ipv4.cpp
    ${CMAKE_SOURCE_DIR}/source/udp.cpp
)
add_executable(sender ${SENDER_SOURCES})

set(RECEIVER_SOURCES
    ${CMAKE_SOURCE_DIR}/source/receiver.cpp

    ${CMAKE_SOURCE_DIR}/source/ethernet.cpp
    ${CMAKE_SOURCE_DIR}/source/ipv4.cpp
    ${CMAKE_SOURCE_DIR}/source/udp.cpp
)
add_executable(receiver ${RECEIVER_SOURCES})


