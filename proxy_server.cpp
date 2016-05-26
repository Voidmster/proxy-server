#include <iostream>
#include <bits/unique_ptr.h>
#include "proxy_server.h"

proxy_server::proxy_server(ipv4_endpoint endpoint)
        : endpoint(endpoint),
          server(service, endpoint, std::bind(&proxy_server::create_new_left_side, this)),
          resolver(&handler, 10),
          proxy_cache(" pages cached", 10000),
          left_side_counter(0),
          right_side_counter(0),
          handler(service, [this] () throw(std::runtime_error) {
              auto res = resolver.get_last_result();
              auto p = not_connected.find(res.id);
              if (p == not_connected.end()) {
                  std::cerr << "Error not connected\n";
              } else {
                  if (res.failed) {
                      if (left_sides.find(p->second) != left_sides.end()) {
                          p->second->send_smt_bad(http_wrapper::BAD_REQUEST());
                      }
                  } else {
                      if (left_sides.find(p->second) != left_sides.end()) {
                          res.callback(res.x, res.y);
                      } else {
                      }
                  }
                  not_connected.erase(res.id);
                  resolver.return_id(res.id);
              }
          })
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
    std::unique_ptr<left_side> u_ptr(new left_side(this, [this](left_side* item) throw(std::runtime_error) {left_sides.erase(item);}));
    left_side *ptr = u_ptr.get();
    left_sides.emplace(ptr, std::move(u_ptr));
    if (++left_side_counter % 10 == 0) {
        std::cerr << "> " << left_side_counter  << " left_sides created\n";
    }

}

void proxy_server::create_new_right_side(left_side *caller) {
    not_connected.emplace(resolver.resolve(caller->request->get_host(), [this, caller] (sockaddr x, socklen_t y) throw(std::runtime_error) {
        std::unique_ptr<right_side> u_ptr(new right_side(this, caller, x, y, [this](right_side* item) {right_sides.erase(item);}));
        right_side *ptr = u_ptr.get();
        this->right_sides.emplace(ptr, std::move(u_ptr));
        if (++this->right_side_counter % 10 == 0) {
            std::cerr << "> " << right_side_counter  << " right_sides created\n";
        }
    }), caller);
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
              try {
                  if (events & EPOLLIN) {
                      read_request();
                      return;
                  }
                  if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                      this->on_disconnect(this);
                  }
                  if (events & EPOLLOUT) {
                      send_response();
                      return;
                  }
              } catch(std::runtime_error &e) {
                  this->on_disconnect(this);
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

void left_side::read_request() {
    std::string buffer;

    socket.read_input(buffer);


    if (request.get() == nullptr) {
        request.reset(new http_request(buffer));
    } else {
        request->add_part(buffer);
    }

    if (request->get_state() == http_request::BAD) {
        messages.push(http_wrapper::BAD_REQUEST());
        ioEvent.add_flag(EPOLLOUT);
    } else if (request->get_state() == http_request::FULL_BODY) {
        proxy->create_new_right_side(this);
        left_side_timer.change_time(SOCKET_TIMEOUT);
    }
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

void left_side::set_relations(right_side *p) {
    partner = p;
    connected.insert(p);
}

void left_side::send_smt_bad(std::string msg) {
    messages.push(msg);
    ioEvent.add_flag(EPOLLOUT);
}


right_side::right_side(proxy_server *proxy, left_side *partner, sockaddr x, socklen_t y, std::function<void(right_side *)> on_disconnect)
        : socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK),
          partner(partner),
          ioEvent(proxy->get_service(), socket.get_fd(), EPOLLOUT, [this] (uint32_t events) mutable throw(std::runtime_error)
          {
              try {
                  if (events & EPOLLIN) {
                      read_response();
                  }
                  if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                      this->on_disconnect(this);
                  }
                  if (events & EPOLLOUT) {
                      send_request();
                  }
              } catch(std::runtime_error &e) {
                  this->on_disconnect(this);
              }
          }),
          on_disconnect(on_disconnect),
          proxy(proxy),
          request(std::move(partner->request)),
          cache_hit(false),
          right_side_timer(proxy->get_service().get_time_service(), CONNECTION_TIMEOUT, [this] {
              if (this->partner) {
                  this->partner->send_smt_bad(http_wrapper::NOT_FOUND());
              }
              this->on_disconnect(this);
          })
{
    partner->set_relations(this);
    socket.connect(&x, y);
}

right_side::~right_side() {
    if (partner != nullptr){
        partner->connected.erase(this);
        if (partner->partner == this) {
            partner->partner = nullptr;
        }
    }
    try_cache();
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
        throw_error("No partner");
    }
}

void right_side::read_response() {
    if (partner != nullptr) {
        std::string buffer;

        socket.read_input(buffer);

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
    } else {
        throw_error("No partner");
    }
}

void right_side::try_cache() {
    if (response && response->is_cacheable() && !cache_hit) {
        proxy->proxy_cache.put(host + URI, http_response(*response));
    }
}



