#ifndef PROXY_SERVER_EVENT_HANDLER_H
#define PROXY_SERVER_EVENT_HANDLER_H

#include <sys/eventfd.h>
#include "io_service.h"

class event_handler {
public:
    event_handler(io_service& service, std::function<void()> callback);

    file_descriptor& get_fd();
private:
    int create_eventfd();
    io_service &service;
    file_descriptor event_fd;
    io_event handler_event;
    std::function<void()> callback;
};

class handler_entry {
    typedef typename std::function<void(sockaddr, socklen_t)> callback_t;
public:
    handler_entry(uint64_t id, sockaddr x, socklen_t y, bool failed, callback_t callback);

    uint64_t id;
    sockaddr x;
    socklen_t y;
    callback_t callback;
    bool failed;
};


#endif //PROXY_SERVER_EVENT_HANDLER_H
