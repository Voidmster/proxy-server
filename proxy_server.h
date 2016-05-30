#ifndef PROXY_PROXY_H
#define PROXY_PROXY_H


#include <vector>
#include <memory>
#include <map>
#include <set>
#include <queue>
#include <iostream>
#include <bits/unique_ptr.h>
#include "http_server.h"
#include "http_wrappers.h"
#include "dns_resolver.h"
#include "io_service.h"
#include "event_handler.h"


const std::chrono::seconds SOCKET_TIMEOUT = std::chrono::seconds(600);
const std::chrono::seconds CONNECTION_TIMEOUT = std::chrono::seconds(180);

class left_side;
class right_side;

class proxy_server {
    friend class right_side;
public:
    proxy_server(ipv4_endpoint endpoint);
    posix_socket& get_server();
    io_service& get_service();

    void create_new_left_side();
    void run();

    void create_new_right_side(left_side*);
private:
    io_service service;
    http_server server;
    ipv4_endpoint endpoint;
    std::map<left_side*, std::unique_ptr <left_side> > left_sides;
    std::map<right_side*, std::unique_ptr <right_side> > right_sides;
    std::map<uint64_t, left_side*> not_connected;
    lru_cache<std::string, http_response> proxy_cache;

    event_handler handler;
    dns_resolver resolver;

    int left_side_counter;
    int right_side_counter;
};

class left_side {
    friend class right_side;
    friend class proxy_server;
public:
    left_side(proxy_server *proxy, std::function<void(left_side*)> on_disconnect);
    ~left_side();

    void read_request();
    void send_response();
private:
    proxy_server* proxy;
    posix_socket socket;
    io_event ioEvent;
    right_side* partner;
    std::unique_ptr<http_request> request;
    std::queue<std::string> messages;
    std::function<void(left_side*)> on_disconnect;

    void set_relations(right_side* p);
    void send_smt_bad(std::string msg);

    std::set<right_side *> connected;
    timer left_side_timer;
};

class right_side {
    friend class left_side;
public:
    right_side(proxy_server *proxy, left_side* partner, sockaddr x, socklen_t y, std::function<void(right_side*)> on_disconnect);
    ~right_side();

    void send_request();
    void read_response();

    void try_cache();
private:
    proxy_server* proxy;
    posix_socket socket;
    io_event ioEvent;
    left_side* partner;
    std::unique_ptr<http_response> response;
    std::function<void(right_side*)> on_disconnect;

    std::unique_ptr<http_request> request;
    bool cache_hit;
    std::string host;
    std::string URI;
    bool read_after_cache_hit = false;
    timer right_side_timer;
    std::string rest;
};

#endif //PROXY_PROXY_H
