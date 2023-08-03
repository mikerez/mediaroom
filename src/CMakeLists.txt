cmake_minimum_required(VERSION 2.8.1)

project (mediaroom C CXX)

include_directories (../lib/libstack)
include_directories (core)
include_directories (common)

set(SRCS
    core/System.cpp
    core/Debug.cpp
    core/PacketDetails.cpp
    common/ConfigParser.cpp
    ../lib/libshared/SharedServer.cpp
    ../lib/libshared/SharedSocket.cpp
    )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -pthread -g -fno-strict-aliasing -O0")

add_executable (mediaroom mediaroom.cpp ${SRCS})
target_link_libraries(mediaroom ${Boost_LIBRARIES} -lrt)

