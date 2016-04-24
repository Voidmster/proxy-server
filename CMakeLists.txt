cmake_minimum_required(VERSION 3.2)
project(proxy)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp posix_socket.h io_service.cpp io_service.h posix_socket.cpp)
add_executable(proxy ${SOURCE_FILES} main.cpp)