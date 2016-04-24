//
// Created by daniil on 24.04.16.
//

#ifndef PROXY_IO_SERVICE_H
#define PROXY_IO_SERVICE_H


#include <sys/epoll.h>
#include "posix_socket.h"

#define MAX_EVENTS 10

class io_service {
public:
    io_service(int listen_fd);
    void run();
    void control(int op, int fd, epoll_event* event);
    ~io_service();
private:
    int epoll_fd;
    int listen_fd;
};


#endif //PROXY_IO_SERVICE_H
