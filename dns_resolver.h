#ifndef PROXY_SERVER_DNS_RESOLVER_H
#define PROXY_SERVER_DNS_RESOLVER_H


#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "lru_cache.h"

class dns_resolver {
public:
    dns_resolver(size_t threads_count);
    ~dns_resolver();

    size_t resolve(std::string const&);
    bool result_is_ready(size_t id, sockaddr& x, socklen_t &y, bool &err_flag);
    void cancel(size_t id);

    class state {
    public:
        state() : canceled(false), corrupted(false) {};

        std::unique_ptr<std::pair<sockaddr , socklen_t >>& get_result();
        void set_result(sockaddr x, socklen_t y);
        void cancel();
        void reset();
        void corrupt();

        bool is_canceled();
        bool is_corrupted();
    private:
        bool canceled;
        bool corrupted;
        std::unique_ptr<std::pair<sockaddr , socklen_t>> result;
    };

    class dns_cache_node {
    public:
        std::pair<sockaddr, socklen_t> result;
        bool resolved = true;
    };

private:

    void worker();

    bool finish;
    size_t threads_count;
    std::vector<std::thread> workers;

    std::mutex fp_queue_mutex;
    std::queue<size_t > fp_queue;

    std::mutex vector_mutex;
    std::vector<state> place_vector;

    std::mutex t_queue_mutex;
    std::condition_variable condition;
    std::queue<std::pair<std::string, size_t>> t_queue;

    std::mutex cache_mutex;
    lru_cache< std::string, dns_cache_node > dns_cache;
};

#endif //PROXY_SERVER_DNS_RESOLVER_H