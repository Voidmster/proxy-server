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
    void bind(sa_family_t sa_family, in_addr_t s_addr, uint16_t port);
    void listen();
    void connect(sockaddr *adr, socklen_t addrlen);
    int accept();
    int get_flags();
    int get_available_bytes();
    int read_input(std::string &s);

    ssize_t read_some(void *buffer, size_t size);
    void write(std::string const &);
    void write(const char *buffer, size_t size);
    void set_flags(int nex_flags);
    file_descriptor& get_fd();
    ~posix_socket();
private:
    int create_socket_fd(int domain, int type);
    file_descriptor fd;
};


#endif //PROXY_POSIX_SOCKET_H
