//
// Created by daniil on 24.04.16.
//

#include "io_service.h"

io_service::io_service(int listen_fd) {
    epoll_fd = epoll_create(1);

    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = listen_fd;
    control(EPOLL_CTL_ADD, listen_fd, &event);

    this->listen_fd = listen_fd;
    if (epoll_fd == -1) {
        throw std::runtime_error("Error when create epoll");
    } else {
        printf("Epoll created\n");
    }
}

void io_service::run() {
    printf("run\n");
    struct epoll_event events[MAX_EVENTS];
    struct epoll_event event;
    int conn_sock;
    int n;
    struct sockaddr_in client;
    socklen_t len = sizeof client;

    for (;;) {
        if ((n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1)) < 0) {
            printf("Error #%d\n", errno);
            throw std::runtime_error("Error while epoll_wait");
        }

        printf("Count of events == %i\n", n);

        for (int i = 0; i < n; i++) {
            if (events[i].data.fd == listen_fd) {
                printf("Catch listen socket\n");
                conn_sock = accept(listen_fd, (struct sockaddr *) &client, &len);

                if (conn_sock == -1) {
                    throw std::runtime_error("Error while epoll_wait");
                }

                set_flags(conn_sock, get_flags(conn_sock) | O_NONBLOCK);

                event.events = EPOLLIN | EPOLLET;
                event.data.fd = conn_sock;
                control(EPOLL_CTL_ADD, conn_sock, &event);
            } else {
                printf("Catch not listen socket\n");
            }
        }
        printf("\n");
    }
}

void io_service::control(int op, int fd, epoll_event *event) {
    if (epoll_ctl(epoll_fd, op, fd, event) == -1) {
        throw std::runtime_error("Error in epoll_ctl");
    }
}

io_service::~io_service() {
    close(listen_fd);
}




