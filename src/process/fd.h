// A class that manages the lifetimes of fds.

#ifndef PROCESS_FD_H_
#define PROCESS_FD_H_

#include <unistd.h>

class Fd {
 public:
  Fd();
  static Fd take(int fd);
  static Fd dup(int fd);

  ~Fd();

  // Movable
  Fd(Fd&& other);
  Fd& operator=(Fd&& other);

  // Not copyable
  Fd(const Fd& other) = delete;
  Fd& operator=(const Fd& other) = delete;

  int operator*() const { return fd_; }

 private:
  Fd(int fd) : fd_(fd) {}
  int fd_ = -1;
};

#endif
