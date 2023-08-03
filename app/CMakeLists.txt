cmake_minimum_required(VERSION 2.8.1)

project (example C CXX)

include_directories (../lib/libstack)
include_directories (core)

set(SRCS
    ../src/core/Debug.cpp
    ../lib/libshared/SharedClient.cpp
    ../lib/libshared/SharedSocket.cpp
    )

add_executable (example example.cpp ${SRCS})

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -pthread -g -fno-strict-aliasing -O0")

target_link_libraries(example ${Boost_LIBRARIES} -lrt)
