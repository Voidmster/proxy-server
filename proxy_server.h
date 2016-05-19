#ifndef PROXY_PROXY_H
#define PROXY_PROXY_H


#include <vector>
#include <memory>
#include <map>
#include <set>
#include <queue>
#include "http_server.h"
#include "http_wrappers.h"
#include "dns_resolver.h"
#include "io_service.h"


#define SOCKET_TIMEOUT std::chrono::seconds(600)
#define CONNECTION_TIMEOUT std::chrono::seconds(180)

class left_side;
class right_side;

class proxy_server {
    friend class right_side;
public:
    proxy_server(ipv4_endpoint endpoint);
    posix_socket& get_server();
    io_service& get_service();

    dns_resolver& get_resolver();

    void create_new_left_side();
    void run();

    right_side* create_new_right_side(left_side*);
private:
    dns_resolver resolver;
    io_service service;
    http_server server;
    ipv4_endpoint endpoint;
    std::map<left_side*, std::unique_ptr <left_side> > left_sides;
    std::map<right_side*, std::unique_ptr <right_side> > right_sides;
    lru_cache<std::string, http_response> proxy_cache;

    int left_side_counter;
    int right_side_counter;
};

class left_side {
    friend class right_side;
public:
    left_side(proxy_server *proxy, std::function<void(left_side*)> on_disconnect);
    ~left_side();

    int read_request();
    void send_response();

    void update_state();
    void set_on_read(bool state);
    void set_on_write(bool state);
private:
    proxy_server* proxy;
    posix_socket socket;
    io_event ioEvent;
    right_side* partner;
    std::unique_ptr<http_request> request;
    std::queue<std::string> messages;
    std::function<void(left_side*)> on_disconnect;
    bool on_read;
    bool on_write;

    std::set<right_side *> connected;
    timer left_side_timer;
};

class right_side {
    friend class left_side;
public:
    right_side(proxy_server *proxy, left_side* partner, std::function<void(right_side*)> on_disconnect);
    ~right_side();

    void create_connection();
    void send_request();
    int read_response();

    void update_state();
    void set_on_read(bool state);
    void set_on_write(bool state);

    void try_cache();
private:
    proxy_server* proxy;
    posix_socket socket;
    io_event ioEvent;
    left_side* partner;
    std::unique_ptr<http_response> response;
    std::function<void(right_side*)> on_disconnect;
    bool on_read;
    bool on_write;

    size_t resolver_id;
    bool connected;
    std::unique_ptr<http_request> request;
    bool cache_hit;
    std::string host;
    std::string URI;
    bool read_after_cache_hit = false;
    timer right_side_timer;
};

#endif //PROXY_PROXY_H
