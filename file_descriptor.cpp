#include "file_descriptor.h"

file_descriptor::file_descriptor() :fd(-1) {}

file_descriptor::file_descriptor(int fd) : fd(fd) {}

file_descriptor::~file_descriptor() {
    close();
}

void file_descriptor::close() {
    if (fd == -1) {
        return;
    } else {
        int r = ::close(fd);
        if (r == -1 && errno != EAGAIN) {
            throw_error("file_descriptor::close()");
        }
        fd = -1;
    }
}

int &file_descriptor::get_fd() {
    return fd;
}









