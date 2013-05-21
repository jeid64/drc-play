cmake_minimum_required(VERSION 2.8)

project(drc-cap)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")

add_definitions(-std=gnu++11 -Wall -Wextra)

add_executable(drc-cap
               astrm.cc
               astrm.h
               ccmp.cc
               ccmp.h
               cmd.cc
               cmd.h
               crc32.cc
               crc32.h
               drc-cap.cc
               packet-injector.h
               polarssl-aes.cc
               polarssl-aes.h
               polarssl-sha1.cc
               polarssl-sha1.h
               portaudio-handler.cc
               portaudio-handler.h
               protocol-handler.h
               queue.h
               sdl-handler.cc
               sdl-handler.h
               vstrm.cc
               vstrm.h
               wpa2-sniffer.cc
               wpa2-sniffer.h)

find_package(PCAP REQUIRED)
include_directories(${PCAP_INCLUDE_DIR})

find_package(Portaudio REQUIRED)
include_directories(${PORTAUDIO_INCLUDE_DIRS})

find_package(FFmpeg REQUIRED)
include_directories(${FFMPEG_INCLUDE_DIR})

find_package(SDL REQUIRED)
include_directories(${SDL_INCLUDE_DIR})

target_link_libraries(drc-cap ${PCAP_LIBRARY} ${PORTAUDIO_LIBRARIES}
                              ${FFMPEG_LIBAVCODEC_LIBRARIES}
                              ${FFMPEG_LIBSWSCALE_LIBRARIES}
                              ${SDL_LIBRARY})
