#include "process/fd.h"

#include <stdio.h>

Fd::Fd() : fd_(-1) {}

Fd Fd::take(int fd) { return Fd(fd); }

Fd Fd::dup(int fd) {
  int new_fd = ::dup(fd);
  if (new_fd == -1) {
    perror("Fd::dup's dup call.");
    return Fd(-1);
  }
  return Fd(new_fd);
}

Fd::~Fd() {
  if (fd_ != -1) {
    int r = close(fd_);
    if (r == -1) {
      perror("Fd::~Fd close");
    }
  }
}

Fd::Fd(Fd&& other) {
  fd_ = other.fd_;
  other.fd_ = -1;
}

Fd& Fd::operator=(Fd&& other) {
  if (fd_ != -1) {
    int r = close(fd_);
    if (r == -1) {
      perror("Fd::operator= close");
    }
  }
  fd_ = other.fd_;
  other.fd_ = -1;
  return *this;
}
