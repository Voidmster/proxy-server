#include <iostream>
#include "http_server.h"

http_server::http_server(io_service &service, ipv4_endpoint &endpoint, std::function<void()> on_accept)
        : service(service),
          socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK),
          event(service, socket.get_fd(), EPOLLIN, [this](uint32_t event)
          {
              if (event == EPOLLIN) {
                this->on_accept();
              }
          }),
          on_accept(on_accept)
{
    std::cerr << "Server socket created\n";
    socket.bind(AF_INET, endpoint.port(), endpoint.addrnet());
    socket.listen();
}

ipv4_endpoint http_server::get_local_endpoint() {
    sockaddr_in adr;
    socklen_t socklen = sizeof adr;
    if (::getsockname(socket.get_fd().get_fd(), reinterpret_cast<sockaddr*>(&adr), &socklen) == -1) {
        throw_error("Error in get_local_endpoint");
    }
    return ipv4_endpoint(adr.sin_port, adr.sin_addr.s_addr);
}

posix_socket &http_server::get_socket() {
    return socket;
}





