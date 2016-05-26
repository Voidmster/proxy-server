#ifndef PROXY_SERVER_FILE_DESCRIPTOR_H
#define PROXY_SERVER_FILE_DESCRIPTOR_H

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "utils.h"

class file_descriptor {
public:
    file_descriptor();
    file_descriptor(int fd);
    ~file_descriptor();

    int get_flags();
    void set_flags(uint32_t nex_flags);
    int get_available_bytes();
    ssize_t read_some(void *buffer, size_t size);
    ssize_t write_some(void const *buffer, size_t size);

    void close();
    int& get_fd();
private:
    int fd;
};


#endif //PROXY_SERVER_FILE_DESCRIPTOR_H
