#include <iostream>
#include <netdb.h>
#include <string.h>
#include "dns_resolver.h"

dns_resolver::dns_resolver(size_t threads_count)
        : threads_count(threads_count),
          dns_cache(1000),
          finish(false)
{
    for (int i = 0; i < threads_count; i++) {
        workers.push_back(std::thread(&dns_resolver::worker, this));
    }
}

void dns_resolver::worker() {
    while (!finish) {
        std::unique_lock<std::mutex> locker_t(t_queue_mutex);
        condition.wait(locker_t, [&]{ return (!t_queue.empty() || finish);});
        if (finish) {
            break;
        }

        auto p = t_queue.front();
        t_queue.pop();
        locker_t.unlock();

        bool err_flag = false;
        sockaddr x;
        socklen_t y;

        std::unique_lock<std::mutex> cache_locker(cache_mutex);
        bool hit = dns_cache.contains(p.first);
        if (hit) {
            std::cerr << "!!!DNS CACHE HIT!!!\n";
            auto node = dns_cache.get(p.first);
            if (node.resolved) {
                x = node.result.first;
                y = node.result.second;
            } else {
                err_flag = false;
            }
        }
        cache_locker.unlock();

        if (!hit) {
            std::string host(p.first);
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
            if (err_flag) {
                new_node.resolved = false;
            } else {
                new_node.result.first = x;
                new_node.result.second = y;
            }
            dns_cache.put(host, std::move(new_node));
            cache_locker.unlock();
        }

        std::unique_lock<std::mutex> locker_v(vector_mutex);
        if (place_vector[p.second].is_canceled()) {
            place_vector[p.second].reset();
            locker_v.unlock();
            std::unique_lock<std::mutex> locker_fp_q(fp_queue_mutex);
            fp_queue.push(p.second);
            locker_fp_q.unlock();
        } else {
            if (err_flag) {
                place_vector[p.second].corrupt();
            }
            place_vector[p.second].set_result(x, y);
            locker_v.unlock();
        }
    }
}

dns_resolver::~dns_resolver() {
    std::unique_lock<std::mutex> locker_t(t_queue_mutex);
    finish = true;
    locker_t.unlock();
    condition.notify_all();
    for (int i = 0; i < threads_count; i++) {
        workers[i].join();
    }
}

size_t dns_resolver::resolve(std::string const &host) {
    bool flag = false;
    size_t x;

    std::unique_lock<std::mutex> locker_fp_q(fp_queue_mutex);
    if (!fp_queue.empty()) {
        flag = true;
        x = fp_queue.front();
        fp_queue.pop();
    }
    locker_fp_q.unlock();

    if (!flag) {
        std::unique_lock<std::mutex> locker_v(vector_mutex);
        x = place_vector.size();
        place_vector.emplace_back();
        locker_v.unlock();
    }

    std::unique_lock<std::mutex> locker_t_q(t_queue_mutex);
    t_queue.push(std::make_pair(host, x));
    locker_t_q.unlock();

    condition.notify_one();

    return x;
}

bool dns_resolver::result_is_ready(size_t id, sockaddr& x, socklen_t &y, bool &err_flag) {
    std::unique_lock<std::mutex> locker_v(vector_mutex);
    if (place_vector[id].get_result() == nullptr) {
        return false;
    } else {
        x = place_vector[id].get_result()->first;
        y = place_vector[id].get_result()->second;
        err_flag = place_vector[id].is_corrupted();
        return true;
    }
}

void dns_resolver::cancel(size_t id) {
    std::unique_lock<std::mutex> locker_v(vector_mutex);
    if (place_vector[id].get_result() == nullptr) {
        place_vector[id].cancel();
        locker_v.unlock();
    } else {
        place_vector[id].reset();
        locker_v.unlock();
        std::unique_lock<std::mutex> locker_fp_q(fp_queue_mutex);
        fp_queue.push(id);
        locker_fp_q.unlock();
    }
}


void dns_resolver::state::set_result(sockaddr x, socklen_t y) {
    result.reset(new std::pair<sockaddr,socklen_t>(x, y));
}

std::unique_ptr<std::pair<sockaddr, socklen_t>> &dns_resolver::state::get_result() {
    return result;
}

void dns_resolver::state::cancel() {
    canceled = true;
}

void dns_resolver::state::reset() {
    result.reset(nullptr);
    corrupted = false;
    canceled = false;
}

bool dns_resolver::state::is_canceled() {
    return canceled;
}

bool dns_resolver::state::is_corrupted() {
    return corrupted;
}

void dns_resolver::state::corrupt() {
    corrupted = true;
}









