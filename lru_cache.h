#ifndef PROXY_SERVER_LRU_CACHE_H
#define PROXY_SERVER_LRU_CACHE_H

#include <utility>
#include <cstddef>
#include <list>
#include <unordered_map>

template<typename key_t, typename value_t>
class lru_cache {
    typedef typename std::pair<key_t, value_t> pair_t;
public:
    lru_cache(std::string message, size_t _size) : message(message) ,max_size(_size) {};
    ~lru_cache() {};

    void put(const key_t &key, const value_t &value) {
        if (size() % 10 == 0) {
            std::cerr << "> " << size()  << message << '\n';
        }

        remove(key);
        cached_items_list.push_front(pair_t(key, value));
        cached_items_map[key] = cached_items_list.begin();

        if (cached_items_map.size() > max_size) {
            auto last = cached_items_list.end();
            last--;
            cached_items_map.erase(last->first);
            cached_items_list.pop_back();
        }
    }

    const value_t &get(const key_t &key) {
        auto it = cached_items_map.find(key);
        if (it == cached_items_map.end()) {
            throw new std::range_error("key not found");
        } else {
            cached_items_list.splice(cached_items_list.begin(), cached_items_list, it->second);
            return it->second->second;
        }
    }

    void remove(const key_t &key) {
        auto it = cached_items_map.find(key);
        if (it != cached_items_map.end()) {
            cached_items_list.erase(it->second);
            cached_items_map.erase(it);
        }
    }

    bool contains(const key_t &key) const {
        return (cached_items_map.find(key) != cached_items_map.end());
    }

    size_t size() const {
        return cached_items_map.size();
    }
private:
    size_t  max_size;
    std::string message;
    std::list<pair_t> cached_items_list;
    std::unordered_map<key_t, decltype(cached_items_list.begin())> cached_items_map;
};


#endif //PROXY_SERVER_LRU_CACHE_H
