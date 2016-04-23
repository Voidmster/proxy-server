#include <iostream>
#include "posix_socket.h"


using namespace std;

int main() {
    int socket_fd, client_fd;
    uint16_t port = 8080;

    socket_fd = create_socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK);

    bind_socket(socket_fd, AF_INET, htonl(INADDR_LOOPBACK), htons(port));

    listen_socket(socket_fd);

    struct sockaddr_in client;
    socklen_t len = sizeof client;

    do {
        client_fd = accept(socket_fd, (struct sockaddr *) &client, &len);
    } while (client_fd < 0 && errno == EAGAIN);

    if (client_fd < 0 && errno != EAGAIN) {
        throw std::runtime_error("Crash in accept");
    }
    while (true) {
        {
            char buffer[256];
            int n = read(client_fd, buffer, sizeof buffer);

            if (n < 0) {
                perror("Crash in read\n");
            } else {
                printf("Message is\n%s\n", buffer);
            }
        }
    }
    close(socket_fd);
    close(client_fd);
    return 0;
}