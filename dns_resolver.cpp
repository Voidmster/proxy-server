#include <iostream>
#include <netdb.h>
#include <string.h>
#include "dns_resolver.h"

dns_resolver::dns_resolver(event_handler *handler, size_t threads_count)
        : threads_count(threads_count),
          dns_cache(" addresses cached", 1000),
          finish(false),
          handler(handler)
{
    for (int i = 0; i < threads_count; i++) {
        workers.push_back(std::thread(&dns_resolver::worker, this));
    }
}

void dns_resolver::worker() {
    while (!finish) {
        std::unique_lock<std::mutex> tasks_locker(tasks_mutex);
        condition.wait(tasks_locker, [&]{ return (!tasks.empty() || finish);});
        if (finish) {
            break;
        }

        auto p = tasks.front();
        tasks.pop();
        tasks_locker.unlock();

        bool err_flag = false;
        sockaddr x;
        socklen_t y;

        std::unique_lock<std::mutex> cache_locker(cache_mutex);
        bool hit = dns_cache.contains(p.first.second);
        if (hit) {
            auto node = dns_cache.get(p.first.second);
            x = node.result.first;
            y = node.result.second;
        }
        cache_locker.unlock();

        if (!hit) {
            std::string host(p.first.second);
            std::string port("80");

            auto i = host.find(":");
            if (i != host.npos) {
                port = host.substr(i + 1);
                host = host.substr(0, i);
            }

            struct addrinfo hints, *r;
            bzero(&hints, sizeof(hints));
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_flags = AI_PASSIVE;

            int error = getaddrinfo(host.data(), port.data(), &hints, &r);
            if (error != 0) {
                perror(gai_strerror(error));
                err_flag = true;
            } else {
                x = *(r->ai_addr);
                y = r->ai_addrlen;
                freeaddrinfo(r);
            }

            cache_locker.lock();
            dns_cache_node new_node;
            if (!err_flag) {
                new_node.result.first = x;
                new_node.result.second = y;
            }
            dns_cache.put(host, std::move(new_node));
            cache_locker.unlock();
        }

        std::unique_lock<std::mutex> results_locker(resuts_mutex);
        results.emplace(p.first.first, x, y, err_flag, p.second);
        uint64_t i = 1;
        handler->get_fd().write_some(&i, sizeof(i));
    }
}

dns_resolver::~dns_resolver() {
    std::unique_lock<std::mutex> locker_t(tasks_mutex);
    finish = true;
    locker_t.unlock();
    condition.notify_all();
    for (int i = 0; i < threads_count; i++) {
        workers[i].join();
    }
}

uint64_t dns_resolver::resolve(std::string const &host, callback_t callback) {
    uint64_t x = poll.get_id();
    std::unique_lock<std::mutex> locker(tasks_mutex);
    tasks.push(std::make_pair(std::make_pair(x, host), std::move(callback)));
    condition.notify_one();
    locker.unlock();
    return x;
}

handler_entry dns_resolver::get_last_result() {
    std::unique_lock<std::mutex> results_locker(resuts_mutex);
    auto res = results.front();
    results.pop();
    results_locker.unlock();

    return res;
}

void dns_resolver::return_id(uint64_t id) {
    poll.insert_id(id);
}


id_poll::id_poll() : counter(0)
{
    poll.push(counter);
}

uint64_t id_poll::get_id() {
    uint64_t id;
    std::unique_lock<std::mutex> locker(mutex);
    if (poll.empty()) {
        poll.push(++counter);
    }
    id = poll.front();
    poll.pop();
    locker.unlock();

    return id;
}

void id_poll::insert_id(uint64_t id) {
    std::unique_lock<std::mutex> locker(mutex);
    poll.push(id);
}






