cmake_minimum_required(VERSION 2.8)

project(drc-cap)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

add_definitions(-std=gnu++11 -Wall -Wextra)

add_executable(drc-cap
               astrm.cc
               astrm.h
               cmd.cc
               cmd.h
               drc-cap.cc
               portaudio-handler.cc
               portaudio-handler.h
               protocol-handler.h
               queue.h
               vstrm.cc
               vstrm.h
               udp-sniffer.cc
               udp-sniffer.h)

find_package(Portaudio REQUIRED)
include_directories(${PORTAUDIO_INCLUDE_DIRS})

target_link_libraries(drc-cap ${PORTAUDIO_LIBRARIES})
