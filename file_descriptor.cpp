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

int file_descriptor::get_flags() {
    int result = fcntl(fd, F_GETFD);
    if (result == -1) {
        throw_error("Error in get_flags");
    }

    return result;
}

void file_descriptor::set_flags(uint32_t nex_flags) {
    if (fcntl(fd, F_GETFD, nex_flags) == -1) {
        throw_error("Error in get_flags");
    }
}

int file_descriptor::get_available_bytes() {
    int n;
    if (ioctl(fd, FIONREAD, &n) == -1) {
        throw_error("Error in read_some");
    }

    return n;
}

ssize_t file_descriptor::read_some(void *buffer, size_t size) {
    ssize_t res = ::read(fd, buffer, size);
    if (res == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            throw_error("Error in read_some");
        }
    }

    return res;
}

ssize_t file_descriptor::write_some(void const *buffer, size_t size) {
    ssize_t res = ::write(fd, buffer, size);
    if (res == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            throw_error("Error in read_some");
        }
    }

    return res;
}

















