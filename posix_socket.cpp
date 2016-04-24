//
// Created by daniil on 24.04.16.
//
#include "posix_socket.h"

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

int get_flags(int socket_fd) {
    int flags;
    if ((flags = fcntl(socket_fd, F_GETFD, 0)) == -1) {
        throw std::runtime_error("Error in get_flags");
    }
    return flags;
}

void set_flags(int socket_fd, int new_flags) {
    if (fcntl(socket_fd, F_SETFD, new_flags) == -1) {
        throw std::runtime_error("Error in set_flags");
    }
}