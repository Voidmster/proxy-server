#include <iostream>
#include <signal.h>
#include "io_service.h"

io_service::io_service() : finish(false) {
    epoll_fd = epoll_create(1);
    if (epoll_fd == -1) {
        throw_error("Error in epoll_create");
    }
}

io_service::~io_service() {
    close(epoll_fd);
}

void io_service::control(int op, int fd, io_event *event, uint32_t flags) {
    struct epoll_event e_event;

    e_event.data.ptr = event;
    e_event.events = flags;

    if (epoll_ctl(epoll_fd, op, fd, &e_event) == -1) {
        throw_error("Error in io_service::control");
    }
}


void io_service::add(file_descriptor &fd, io_event *event, uint32_t flags) {
    available.insert(event);
    control(EPOLL_CTL_ADD, fd.get_fd(), event, flags);
}

void io_service::remove(file_descriptor & fd, io_event *event, uint32_t flags) {
    available.erase(event);
    control(EPOLL_CTL_DEL, fd.get_fd(), event, 0);
}

void io_service::modify(file_descriptor & fd, io_event *event, uint32_t flags) {
    control(EPOLL_CTL_MOD, fd.get_fd(), event, flags | EPOLLERR | EPOLLRDHUP | EPOLLHUP);
}

file_descriptor io_service::create_signal_fd(std::vector<uint8_t> signals) {
    sigset_t mask;
    sigemptyset(&mask);
    for (int i = 0; i < signals.size(); i++) {
        if (sigaddset(&mask, signals[i]) == -1) {
            std::cerr << "Not valid signal to block" << signals[i] << "\n";
        }
    }

    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        throw_error("Error in sigprocmask");
    }

    int signal_fd = signalfd(-1, &mask, SFD_CLOEXEC | SFD_NONBLOCK);
    if (signal_fd == -1) {
        throw_error("Error in create of signal_fd");
    }
    return file_descriptor(signal_fd);
}

void io_service::run() {
    file_descriptor signal_fd = create_signal_fd({SIGINT, SIGTERM});
    io_event signal_event(*this, signal_fd, EPOLLIN, [this](uint32_t) {
        perror("We get SIGNAL\n");
        this->finish = true;
    });

    epoll_event events[MAX_EPOLL_EVENTS_COUNT];

    while (!finish) {
        int count;
        int timeout = timeService.time_to_nearest_timeout();
        if (timeout == -1) {
            timeout = DEFAULT_EPOLL_TIMEOUT;
        }

        count = epoll_wait(epoll_fd, events, MAX_EPOLL_EVENTS_COUNT, timeout);

        if (count < 0) {
            if (errno != EINTR) {
                throw_error("Error in epoll_wait");
            } else {
                break;
            }
        }

        for (int i = 0; i < count; i++) {
            auto &ev = events[i];
            try {
                io_event * x = static_cast<io_event *>(ev.data.ptr);
                if (available.find(x) != available.end()) {
                    x->callback(ev.events);
                } else {
                    //std::cerr << "Io_event " << x << " is dead\n";
                }
            } catch (std::exception &e) {
                std::cerr << e.what();
            }
        }
    }
}

time_service &io_service::get_time_service() {
    return timeService;
}


io_event::io_event(io_service &service, file_descriptor &fd, uint32_t flags, std::function<void(uint32_t)> callback)
        : service(service),
          fd(fd),
          callback(callback)
{
    service.add(fd, this, flags);
}

void io_event::modify(uint32_t flags) {
    service.modify(fd, this, flags | EPOLLERR | EPOLLRDHUP | EPOLLHUP);
}

io_event::~io_event() {
    service.remove(fd, this, 0);
}










