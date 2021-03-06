#ifndef PROXY_POSIX_SOCKET_H
#define PROXY_POSIX_SOCKET_H

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>
#include <fcntl.h>
#include <netdb.h>
#include "utils.h"
#include "file_descriptor.h"

class posix_socket {
public:
    posix_socket();
    posix_socket(int accepted_fd);
    posix_socket(int domain, int type);
    void bind(sa_family_t sa_family, uint16_t port, in_addr_t s_addr);
    void listen();
    void connect(sockaddr *adr, socklen_t addrlen);
    int accept();
    int get_flags();
    int get_available_bytes();
    void read_input(std::string &s);

    ssize_t read_some(void *buffer, size_t size);
    ssize_t write_some(void const *buffer, size_t _size);
    void set_flags(uint32_t nex_flags);
    file_descriptor& get_fd();
    ~posix_socket();
private:
    int create_socket_fd(int domain, int type);
    file_descriptor fd;
};


#endif //PROXY_POSIX_SOCKET_H
