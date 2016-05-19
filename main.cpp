#include <iostream>
#include "proxy_server.h"

int main() {
    proxy_server proxy(ipv4_endpoint(36895, ipv4_address::any()));
    proxy.run();
    return 0;
}