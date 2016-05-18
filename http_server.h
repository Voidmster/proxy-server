#ifndef PROXY_HTTP_SERVER_H
#define PROXY_HTTP_SERVER_H


#include "io_service.h"
#include "address_wrappers.h"

class http_server {
public:
    http_server(io_service &service, ipv4_endpoint &endpoint, std::function<void()> on_accept);
    posix_socket& get_socket();
    ipv4_endpoint get_local_endpoint();
private:
    io_service &service;
    posix_socket socket;
    io_event event;
    std::function<void()> on_accept;
};


#endif //PROXY_HTTP_SERVER_H
