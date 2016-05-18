#ifndef PROXY_IO_SERVICE_H
#define PROXY_IO_SERVICE_H

#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <unistd.h>
#include <functional>
#include <set>
#include <vector>
#include "utils.h"
#include "posix_socket.h"

class io_event;
class io_service {
    friend class io_event;
public:
    io_service();

    void add(file_descriptor & fd, io_event* event, uint32_t flags);
    void remove(file_descriptor & fd, io_event* event, uint32_t flags);
    void modify(file_descriptor & fd, io_event* event, uint32_t flags);

    file_descriptor create_signal_fd(std::vector<uint8_t> signals);

    void run();
    ~io_service();
private:
    int epoll_fd;
    bool finish;
    void control(int op, int fd, io_event *event, uint32_t flags);
    std::set<io_event*> available;
};

class io_event{
    friend class io_service;
public:
    io_event(io_service &service, file_descriptor &fd, uint32_t flags, std::function<void(uint32_t)>);
    void modify(uint32_t flags);
    ~io_event();
private:
    io_service &service;
    file_descriptor &fd;
    std::function<void(uint32_t)> callback;
};


#endif //PROXY_IO_SERVICE_H
