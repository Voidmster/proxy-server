#ifndef PROXY_ADDRESS_H
#define PROXY_ADDRESS_H

#include <sys/socket.h>
#include <cstdint>
#include <string>
#include <ostream>

struct ipv4_address
{
    ipv4_address();
    ipv4_address(uint32_t addr_net);
    ipv4_address(std::string const& text);

    static ipv4_address any();

private:
    uint32_t addr_net;

    friend struct ipv4_endpoint;
};

struct ipv4_endpoint
{
    ipv4_endpoint();
    ipv4_endpoint(uint16_t port_host, ipv4_address);

    uint16_t port() const;
    uint32_t addrnet() const;
    std::string to_string() const;

    ipv4_endpoint(uint16_t port_net, uint32_t addr_net);
private:

    uint16_t port_net;
    uint32_t addr_net;
    friend std::ostream& operator<<(std::ostream& os, ipv4_endpoint const& endpoint);

};

std::ostream& operator<<(std::ostream& os, ipv4_endpoint const& endpoint);

#endif //PROXY_ADDRESS_H
