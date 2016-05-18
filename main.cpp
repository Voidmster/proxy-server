#include <iostream>
#include "io_service.h"
#include "proxy_server.h"

int main() {
    io_service service;
    proxy_server proxy1(service, ipv4_endpoint(8080, ipv4_address::any()));
    service.run();

    return 0;
}