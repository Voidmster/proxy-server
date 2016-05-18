#ifndef PROXY_SERVER_FILE_DESCRIPTOR_H
#define PROXY_SERVER_FILE_DESCRIPTOR_H

#include <unistd.h>
#include <errno.h>
#include "utils.h"

class file_descriptor {
public:
    file_descriptor();
    file_descriptor(int fd);
    ~file_descriptor();

    void close();
    int& get_fd();
private:
    int fd;
};


#endif //PROXY_SERVER_FILE_DESCRIPTOR_H
