cmake_minimum_required(VERSION 3.2)
project(proxy)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp posix_socket.h utils.h)
add_executable(proxy ${SOURCE_FILES} main.cpp)