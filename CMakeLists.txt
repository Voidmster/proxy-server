cmake_minimum_required(VERSION 3.2)
project(proxy_server)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")

set(SOURCE_FILES
        io_service.cpp
        io_service.h
        main.cpp
        posix_socket.cpp
        posix_socket.h
        utils.cpp
        utils.h
        http_server.cpp
        http_server.h
        address_wrappers.cpp
        address_wrappers.h
        proxy_server.cpp
        proxy_server.h
        http_wrappers.cpp
        http_wrappers.h
        dns_resolver.cpp
        dns_resolver.h
        file_descriptor.cpp
        file_descriptor.h
        lru_cache.h)
add_executable(proxy_server ${SOURCE_FILES} main.cpp)