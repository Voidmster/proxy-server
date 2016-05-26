#ifndef PROXY_SERVER_DNS_RESOLVER_H
#define PROXY_SERVER_DNS_RESOLVER_H


#include <vector>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include "lru_cache.h"
#include "io_service.h"
#include "event_handler.h"

class id_poll {
public:
    id_poll();

    uint64_t get_id();
    void insert_id(uint64_t id);
private:
    uint64_t counter;
    std::queue<uint64_t> poll;
    std::mutex mutex;
};

class dns_resolver {
    typedef typename std::function<void(sockaddr, socklen_t)> callback_t;
public:
    dns_resolver(event_handler *handler, size_t threads_count);
    ~dns_resolver();

    uint64_t resolve(std::string const& host, callback_t callback);
    handler_entry get_last_result();

    void return_id(uint64_t id);

    class dns_cache_node {
    public:
        std::pair<sockaddr, socklen_t> result;
    };

private:
    event_handler *handler;
    id_poll poll;

    void worker();

    bool finish;
    size_t threads_count;
    std::vector<std::thread> workers;

    std::mutex cache_mutex;
    std::condition_variable condition;
    lru_cache< std::string, dns_cache_node > dns_cache;

    std::mutex tasks_mutex;
    std::queue< std::pair< std::pair< uint64_t, std::string >, callback_t > > tasks;

    std::mutex resuts_mutex;
    std::queue<handler_entry> results;

};



#endif //PROXY_SERVER_DNS_RESOLVER_H