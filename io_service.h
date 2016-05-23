#ifndef PROXY_IO_SERVICE_H
#define PROXY_IO_SERVICE_H

#include <iostream>
#include <sys/epoll.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <unistd.h>
#include <functional>
#include <set>
#include <vector>
#include "utils.h"
#include "posix_socket.h"
#include "time_service.h"

const int DEFAULT_EPOLL_TIMEOUT = 1000;
const int MAX_EPOLL_EVENTS_COUNT = 1000;

class io_event;
class io_service {
    friend class io_event;
public:
    io_service();

    void add(file_descriptor & fd, io_event* event, uint32_t flags);
    void remove(file_descriptor & fd, io_event* event, uint32_t flags);
    void modify(file_descriptor & fd, io_event* event, uint32_t flags);

    file_descriptor create_signal_fd(std::vector<uint8_t> signals);
    time_service& get_time_service();

    void run();
    ~io_service();
private:
    int epoll_fd;
    bool finish;
    void control(int op, int fd, io_event *event, uint32_t flags);
    std::set<io_event*> available;
    time_service timeService;
};

class io_event{
    friend class io_service;
public:
    io_event(io_service &service, file_descriptor &fd, uint32_t flags, std::function<void(uint32_t)>);
    ~io_event();

    void add_flag(uint32_t flag);
    void remove_flag(uint32_t flag);
private:
    uint32_t flags;
    io_service &service;
    file_descriptor &fd;
    std::function<void(uint32_t)> callback;
};


#endif //PROXY_IO_SERVICE_H
