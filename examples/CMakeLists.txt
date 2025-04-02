cmake_minimum_required(VERSION 3.10)
project(WebTransportExamples)

# Path to the parent directory containing libwebtransport.so and web_transport.h
set(WEBTRANSPORT_DIR ${CMAKE_SOURCE_DIR}/..)

# Include directory for web_transport.h
include_directories(${WEBTRANSPORT_DIR})

# Link directory for libwebtransport.so
link_directories(
    ${WEBTRANSPORT_DIR}
)


# Create executables for each example
add_executable(bidirectional_stream_client 
    bidirectional_stream/client.cc
)

add_executable(bidirectional_stream_server 
    bidirectional_stream/server.cc
)

add_executable(datagrams_js 
    datagrams_js/server.cc
)

add_executable(media_bidirectional_stream_js 
    media_bidirectional_stream_js/server.cc 
    media_bidirectional_stream_js/https_server.cc
)

add_executable(media_datagrams_js
    media_datagrams_js/server.cc 
    media_datagrams_js/https_server.cc
)

add_executable(datagrams_client 
    datagrams/client.cc
)

add_executable(datagrams_server 
    datagrams/server.cc
)

add_executable(media_stream_client 
    media_stream/client.cc
    media_stream/rtp_packet.cc
    media_stream/rtp_packet.h
    media_stream/vp8_packet.cc
    media_stream/vp8_packet.h
    media_stream/vp8_rtp_depacketizer.cc
    media_stream/vp8_rtp_depacketizer.h
    media_stream/vp8_rtp_packetizer.cc
    media_stream/vp8_rtp_packetizer.h

    media_stream/h264_packet.cc
    media_stream/h264_packet.h
    media_stream/h264_rtp_depacketizer.cc
    media_stream/h264_rtp_depacketizer.h
    media_stream/h264_rtp_packetizer.cc
    media_stream/h264_rtp_packetizer.h

)
add_executable(media_stream_server media_stream/server.cc)

# Set C++20 standard for all executables
set(EXAMPLES_TARGETS
    bidirectional_stream_client
    bidirectional_stream_server
    media_datagrams_js
    media_bidirectional_stream_js
    datagrams_js
    datagrams_client
    datagrams_server
    media_stream_client
    media_stream_server
)

foreach(TARGET ${EXAMPLES_TARGETS})
    set_property(TARGET ${TARGET} PROPERTY CXX_STANDARD 20)
    # Link with the webtransport library
    target_link_libraries(${TARGET} webtransport)
    
endforeach()
