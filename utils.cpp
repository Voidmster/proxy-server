#include <stdexcept>
#include <sys/epoll.h>
#include "utils.h"

void throw_error(std::string msg) {
    throw std::runtime_error(msg);
}

std::string epoll_event_to_str(uint32_t &event) {
    std::string res;
    if (event & EPOLLIN) { res.append("EPOLLIN "); }
    if (event & EPOLLPRI) { res.append("EPOLLPRI "); }
    if (event & EPOLLOUT) { res.append("EPOLLOUT "); }
    if (event & EPOLLRDNORM) { res.append("EPOLLRDNORM "); }
    if (event & EPOLLRDBAND) { res.append("EPOLLRDBAND "); }
    if (event & EPOLLWRNORM) { res.append("EPOLLWRNORM "); }
    if (event & EPOLLWRBAND) { res.append("EPOLLWRBAND "); }
    if (event & EPOLLMSG) { res.append("EPOLLMSG "); }
    if (event & EPOLLERR) { res.append("EPOLLERR "); }
    if (event & EPOLLHUP) { res.append("EPOLLHUP "); }
    if (event & EPOLLRDHUP) { res.append("EPOLLRDHUP "); }
    if (event & EPOLLWAKEUP) { res.append("EPOLLWAKEUP "); }
    if (event & EPOLLONESHOT) { res.append("EPOLLONESHOT "); }
    if (event & EPOLLET) { res.append("EPOLLET "); }
    return res;
}











