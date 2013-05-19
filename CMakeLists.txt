cmake_minimum_required(VERSION 2.8)

project(drc-cap)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

add_definitions(-std=gnu++11 -Wall -Wextra)

add_executable(drc-cap
               astrm.cc
               astrm.h
               drc-cap.cc
               protocol-handler.h
               vstrm.cc
               vstrm.h
               wpa2-sniffer.cc
               wpa2-sniffer.h)

find_package(PCAP REQUIRED)
include_directories(${PCAP_INCLUDE_DIR})
target_link_libraries(drc-cap ${PCAP_LIBRARY})
