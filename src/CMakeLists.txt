cmake_minimum_required(VERSION 2.8.1)

project (mediaroom C CXX)

include_directories (../lib/libshared)
include_directories (../lib/libstack)
include_directories (core)
include_directories (common)
include_directories (drivers)

set(SRCS
    core/System.cpp
    core/Debug.cpp
    core/PacketDetails.cpp
    core/Sessions.cpp
    core/Stack.cpp
    common/ConfigParser.cpp
    drivers/Capture.cpp
    drivers/DriverPcap.cpp
    ../lib/libshared/SharedServer.cpp
    ../lib/libshared/SharedSocket.cpp
    ../lib/libshared/SharedFlowAccum.cpp
    )

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -pthread -g -fno-strict-aliasing -O0")

add_executable (mediaroom mediaroom.cpp ${SRCS})
target_link_libraries(mediaroom ${Boost_LIBRARIES} -lrt -lpcap)

