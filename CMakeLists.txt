cmake_minimum_required(VERSION 2.8.1)

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME     OFF)

find_package(Boost 1.82.0 REQUIRED COMPONENTS system thread filesystem program_options chrono)
set(CMAKE_LDFLAGS "${CMAKE_LDFLAGS} -lboost_system -lboost-filesystem -lboost_program_options -lboost_thread -lboost_chrono")
include_directories(${Boost_INCLUDE_DIRS})
add_definitions( -DBOOST_ALL_NO_LIB )
link_directories(${Boost_LIBRARY_DIRS})

add_subdirectory(app)
add_subdirectory(src)
