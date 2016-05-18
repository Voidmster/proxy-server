#include <iostream>
#include <sys/ioctl.h>
#include "posix_socket.h"

posix_socket::posix_socket(int domain, int type) : fd(create_socket_fd(domain, type)) {}

posix_socket::~posix_socket() { }

int posix_socket::get_flags() {
    int result = fcntl(fd.get_fd(), F_GETFD);
    if (result == -1) {
        throw_error("Error in get_flags");
    }
    return result;
}

void posix_socket::set_flags(int nex_flags) {
    if (fcntl(fd.get_fd(), F_GETFD, nex_flags) == -1) {
        throw_error("Error in get_flags");
    }
}

void posix_socket::bind(sa_family_t sa_family, in_addr_t s_addr, uint16_t port) {
    struct sockaddr_in hints{};

    hints.sin_family = sa_family;
    hints.sin_addr.s_addr = s_addr;
    hints.sin_port = port;

    if (::bind(fd.get_fd(), (struct sockaddr*) &hints, sizeof hints) == -1) {
        throw_error("Error in bind posix_socket");
    }
}

void posix_socket::listen() {
    if (::listen(fd.get_fd(), SOMAXCONN) == -1) {
        throw_error("Error in listen posix_socket");
    }
}

int posix_socket::accept() {
    int result = ::accept(fd.get_fd(), NULL, NULL);
    if (result == -1) {
        throw_error("Error in accept posix_socket");
    }
    std::cerr << "Socket accepted : " << result << "\n";
    return result;
}

int posix_socket::get_available_bytes() {
    int n;
    if (ioctl(fd.get_fd(), FIONREAD, &n) == -1) {
        throw_error("Error in read_some");
    }

    return n;
}

ssize_t posix_socket::read_some(void *buffer, size_t size) {
    ssize_t res = ::read(fd.get_fd(), buffer, size);
    if (res == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            throw_error("Error in read_some");
        }
    }

    return res;
}

file_descriptor& posix_socket::get_fd() {
    return fd;
}

posix_socket::posix_socket() : fd(-1) {}

posix_socket::posix_socket(int accepted_fd) : fd(accepted_fd) {
    set_flags(get_flags() | SOCK_STREAM | SOCK_NONBLOCK);
}

void posix_socket::connect(sockaddr *adr, socklen_t addrlen) {
    if ((::connect(fd.get_fd(), adr, addrlen)) == -1) {
        if (errno != EINPROGRESS) {
            throw_error("Error in posix_socket::connect");
        }
    }
}

void posix_socket::write(const char *buffer, size_t size) {
    int total = 0;
    while (total != size) {
        ssize_t result = ::write(fd.get_fd(), &buffer[total], size - total);
        if (result <= 0) {
            break;
        }
        total += result;
    }
    if (total != size) {
        throw_error("Error in posix_socket::write");
    }
}

void posix_socket::write(std::string const &s) {
    write(s.data(), s.size());
}

int posix_socket::read_input(std::string &s) {
    try {
        int n = get_available_bytes();
        if (n == 0) {
            std::cerr << "No bytes are available\n";
            return -1;
        }

        char buffer[n + 1];
        ssize_t res = read_some(buffer, n);
        buffer[n] = '\0';

        if (res == 0) {
            return -1;
        }

        s = {buffer, n};
        return 1;
    } catch (std::exception &e) {
        return -1;
    }
}

int posix_socket::create_socket_fd(int domain, int type) {
    int r;
    if ((r = socket(domain, type, 0)) == -1) {
        throw_error("Can't create posix_socket");
    }
    return r;
}






































