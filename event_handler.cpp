#include "event_handler.h"

event_handler::event_handler(io_service &service, std::function<void()> callback)
        : service(service),
          event_fd(create_eventfd()),
          callback(std::move(callback)),
          handler_event(service, event_fd, EPOLLIN, [this] (uint32_t) throw(std::runtime_error) {
              uint64_t i;
              while (event_fd.read_some(&i, sizeof(i)) != -1) {
                  this->callback();
              }
          })
{

}

int event_handler::create_eventfd() {
    int res = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC | EFD_SEMAPHORE);
    if (res == -1) {
        throw_error("create eventfd");
    }

    return res;
}

file_descriptor &event_handler::get_fd() {
    return event_fd;
}


handler_entry::handler_entry(uint64_t id, sockaddr x, socklen_t y, bool failed, callback_t callback)
        : id(id),
          x(x),
          y(y),
          failed(failed),
          callback(std::move(callback))
{ }

