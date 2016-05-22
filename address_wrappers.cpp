#include "address_wrappers.h"

#include <arpa/inet.h>

#include <sstream>
#include <stdexcept>

ipv4_address::ipv4_address():addr_net{0} {}

ipv4_address::ipv4_address(uint32_t addr_net) : addr_net(addr_net) {}

ipv4_address::ipv4_address(const std::string &text) {
    in_addr tmp{};
    int res = inet_pton(AF_INET, text.c_str(), &tmp);
    if (res == 0) {
        throw std::runtime_error(text + "is not a valid ip address");
    }
    addr_net = tmp.s_addr;
}

ipv4_address ipv4_address::any() {
    return ipv4_address(INADDR_ANY);
}

ipv4_endpoint::ipv4_endpoint() : port_net(), addr_net() {}

ipv4_endpoint::ipv4_endpoint(uint16_t port_host, ipv4_address addr)
        : port_net(htons(port_host)),
          addr_net(addr.addr_net) {}

uint16_t ipv4_endpoint::port() const {
    return port_net;
}


uint32_t ipv4_endpoint::addrnet() const {
    return addr_net;
}

std::string ipv4_endpoint::to_string() const {
    std::stringstream ss;
    ss << *this;
    return ss.str();
}

ipv4_endpoint::ipv4_endpoint(uint16_t port_net, uint32_t addr_net)
        : port_net(port_net),
          addr_net(addr_net) {}


std::ostream& operator<<(std::ostream& os, ipv4_endpoint const& endpoint) {
    in_addr tmp;
    tmp.s_addr = endpoint.addr_net;
    os << inet_ntoa(tmp) << ':' << ntohs(endpoint.port());
    return os;
}
