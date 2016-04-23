//
// Created by daniil on 23.04.16.
//

#ifndef PROXY_POSIX_SOCKET_H
#define PROXY_POSIX_SOCKET_H

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <stdexcept>
#include <netdb.h>
#include <string.h>

int create_socket(int domain, int type) {
    int socket_fd = socket(domain, type, 0);
    if (socket_fd == -1) {
        throw std::runtime_error("Crash in create_socket");
    }
    return socket_fd;
}

void listen_socket(int socket_fd) {
    if (listen(socket_fd, SOMAXCONN) == -1) {
        throw std::runtime_error("Crash in listen_socket");
    }
}

void bind_socket(int socket_fd, sa_family_t sin_family, in_addr_t s_addr, uint16_t port) {
    struct sockaddr_in hits;

    memset(&hits, 0, sizeof hits);

    hits.sin_family = sin_family;
    hits.sin_addr.s_addr = s_addr;
    hits.sin_port = port;


    if (bind(socket_fd, (struct sockaddr *) &hits, sizeof hits) < 0) {
        printf("Error #%d\n", errno);
        throw std::runtime_error("Crash in bind_socket");
    }
}


#endif //PROXY_POSIX_SOCKET_H
