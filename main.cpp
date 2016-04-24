#include <iostream>
#include "io_service.h"


using namespace std;

int main() {
    int socket_fd;
    uint16_t port = 8080;

    socket_fd = create_socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK);
    printf("Socket created\n");
    bind_socket(socket_fd, AF_INET, htonl(INADDR_ANY), htons(port));
    printf("Socket bind\n");
    listen_socket(socket_fd);
    printf("Socket listening\n");
    io_service service(socket_fd);
    service.run();
    return 0;
}