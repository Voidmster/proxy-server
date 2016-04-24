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
#include <fcntl.h>

int create_socket(int domain, int type);

void listen_socket(int socket_fd);

void bind_socket(int socket_fd, sa_family_t sin_family, in_addr_t s_addr, uint16_t port);

int get_flags(int socket_fd);

void set_flags(int socket_fd, int new_flags);

#endif //PROXY_POSIX_SOCKET_H
