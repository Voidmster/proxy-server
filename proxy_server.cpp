#include <iostream>
#include <bits/unique_ptr.h>
#include "proxy_server.h"

proxy_server::proxy_server(ipv4_endpoint endpoint)
        : endpoint(endpoint),
          server(service, endpoint, std::bind(&proxy_server::create_new_left_side, this)),
          resolver(5),
          proxy_cache(" pages cached", 10000),
          left_side_counter(0),
          right_side_counter(0)
{
    std::cerr << "Proxy server bind on " << server.get_local_endpoint().to_string() << "\n";
}

posix_socket &proxy_server::get_server() {
    return server.get_socket();
}

io_service &proxy_server::get_service() {
    return service;
}

void proxy_server::create_new_left_side() {
    std::unique_ptr<left_side> u_ptr(new left_side(this, [this](left_side* item) {left_sides.erase(item);}));
    left_side *ptr = u_ptr.get();
    left_sides.emplace(ptr, std::move(u_ptr));
    if (++left_side_counter % 10 == 0) {
        std::cerr << "> " << left_side_counter  << " left_sides created\n";
    }

}

right_side *proxy_server::create_new_right_side(left_side *caller) {
    std::unique_ptr<right_side> u_ptr(new right_side(this, caller, [this](right_side* item) {right_sides.erase(item);}));
    right_side *ptr = u_ptr.get();
    right_sides.emplace(ptr, std::move(u_ptr));
    if (++right_side_counter % 10 == 0) {
        std::cerr << "> " << right_side_counter  << " right_sides created\n";
    }
    return ptr;
}

dns_resolver &proxy_server::get_resolver() {
    return resolver;
}

void proxy_server::run() {
    service.run();
}

left_side::left_side(proxy_server *proxy, std::function<void(left_side*)> on_disconnect)
        : proxy(proxy),
          socket(proxy->get_server().accept()),
          partner(nullptr),
          ioEvent(proxy->get_service(), socket.get_fd(), EPOLLIN, [this] (uint32_t events) mutable throw(std::runtime_error)
          {
              if (events & EPOLLIN) {
                  if (read_request()) { return;}
              }
              if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                  this->on_disconnect(this);
              }
              if (events & EPOLLOUT) {
                  send_response();
              }

          }),
          on_disconnect(on_disconnect),
          left_side_timer(proxy->get_service().get_time_service(), SOCKET_TIMEOUT, [this]() {
              this->on_disconnect(this);
          })
{
}

left_side::~left_side() {
    while (connected.size()) {
        (*connected.begin())->on_disconnect(*connected.begin());
    }
    connected.clear();
}

int left_side::read_request() {
    std::string buffer;
    if (socket.read_input(buffer) == -1) {
        on_disconnect(this);
        return 1;
    }

    if (request.get() == nullptr) {
        request.reset(new http_request(buffer));
    } else {
        request->add_part(buffer);
    }

    if (request->get_state() == http_request::BAD) {
        messages.push(http_wrapper::BAD_REQUEST());
        ioEvent.add_flag(EPOLLOUT);
    } else if (request->get_state() == http_request::FULL_BODY) {
        partner = proxy->create_new_right_side(this);
        connected.insert(partner);
        request.release();
        left_side_timer.change_time(SOCKET_TIMEOUT);
    }
    return 0;
}

void left_side::send_response() {
    while (!messages.empty()) {
        ssize_t ind = socket.write_some(messages.front().c_str(), messages.front().size());
        if (ind != messages.front().size()) {
            messages.front() = messages.front().substr(ind);
            break;
        }
        messages.pop();
    }

    if (messages.empty()) {
        if (partner) {
            partner->ioEvent.add_flag(EPOLLIN);
        }
        ioEvent.remove_flag(EPOLLOUT);
    }
}

right_side::right_side(proxy_server *proxy, left_side *partner, std::function<void(right_side *)> on_disconnect)
        : socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK),
          partner(partner),
          ioEvent(proxy->get_service(), socket.get_fd(), 0, [this] (uint32_t events) mutable throw(std::runtime_error)
          {
              if (!connected && (events == EPOLLHUP)) {
                  create_connection();
                  return;
              }
              if (events & EPOLLIN) {
                  if (read_response()) { return;}
              }
              if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                  this->on_disconnect(this);
                  return;
              }
              if (events & EPOLLOUT) {
                  send_request();
              }
          }),
          on_disconnect(on_disconnect),
          proxy(proxy),
          connected(false),
          request(std::move(partner->request)),
          cache_hit(false),
          right_side_timer(proxy->get_service().get_time_service(), CONNECTION_TIMEOUT, [this] {
              if (this->partner) {
                  this->partner->messages.push(http_wrapper::NOT_FOUND());
                  this->partner->ioEvent.add_flag(EPOLLOUT);
              }
              this->on_disconnect(this);
          })
{
    resolver_id = this->proxy->get_resolver().resolve(request->get_host());
}

right_side::~right_side() {
    if (partner != nullptr){
        partner->connected.erase(this);
        if (partner->partner == this) {
            partner->partner = nullptr;
        }
    }
    if (!connected) {
        proxy->get_resolver().cancel(resolver_id);
    }
    try_cache();
}


void right_side::create_connection() {
    sockaddr x;
    socklen_t y;
    bool err_flag;
    if (proxy->get_resolver().result_is_ready(resolver_id, x, y, err_flag)) {
        if (!err_flag) {
            socket.connect(&x, y);
            connected = true;
            ioEvent.add_flag(EPOLLOUT);
        } else {
            on_disconnect(this);
        }
    }
}


void right_side::send_request() {
    if (partner != nullptr) {
        std::string buf;
        if (rest.empty()) {
            right_side_timer.stop();
            host = request->get_host();
            URI = request->get_URI();
            auto is_valid = request->is_validating();
            cache_hit = proxy->proxy_cache.contains(host + URI);
            if (!is_valid && cache_hit) {
                auto cache_entry = proxy->proxy_cache.get(host + URI);
                auto etag = cache_entry.get_header("Etag");
                request->append_header("If-None-Match", etag);
            }
            buf = request->get_request_text();
        } else {
            buf = rest;
        }

        ssize_t ind = socket.write_some(buf.c_str(), buf.size());
        if (ind != buf.size()) {
            rest = buf.substr(ind);
        } else {
            ioEvent.add_flag(EPOLLIN);
            ioEvent.remove_flag(EPOLLOUT);
        }
    } else {
        on_disconnect(this);
    }
}

int right_side::read_response() {
    if (partner != nullptr) {
        std::string buffer;

        if (socket.read_input(buffer) == -1) {
            on_disconnect(this);
            return 1;
        }
        std::string sub(buffer);
        if (!read_after_cache_hit) {
            if (response.get() == nullptr) {
                response.reset(new http_response(sub));
            } else {
                response->add_part(sub);
            }

            if (response->get_state() >= http_wrapper::FIRST_LINE) {
                if (response->get_code() == "304" && cache_hit) {
                    partner->messages.push(proxy->proxy_cache.get(host + URI).get_text());
                    read_after_cache_hit = true;
                } else {
                    cache_hit = false;
                    partner->messages.push(sub);
                }
                partner->ioEvent.add_flag(EPOLLOUT);
                ioEvent.remove_flag(EPOLLIN);
            } else if (response->get_state() == http_wrapper::BAD) {
                partner->messages.push(sub);
                partner->ioEvent.add_flag(EPOLLOUT);
                ioEvent.remove_flag(EPOLLIN);
            }
        } else {
            //Read after get cache
        }
        return 0;
    } else {
        on_disconnect(this);
        return 1;
    }
}

void right_side::try_cache() {
    if (response && response->is_cacheable() && !cache_hit) {
        proxy->proxy_cache.put(host + URI, http_response(*response));
    }
}



