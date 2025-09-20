#ifndef PROCESS_STREAM_H_
#define PROCESS_STREAM_H_

#include <fcntl.h>

#include "process/fd.h"

// Process-lib internal use only.
enum class StreamKind {
  // Don't redirect the output.
  NONE = 0,
  // Redirect the output to a new pipe.
  PIPE = 1,
  // Redirect stderr to stdout's pipe. Only settable on stderr and when stdout
  // is set to PIPE.
  STDOUT_PIPE = 2,
  // Write the stream to a file descriptor.
  FILE = 3,
};

class StreamOutConf {
 public:
  static StreamOutConf None() { return StreamOutConf(StreamKind::NONE); }
  static StreamOutConf Pipe() { return StreamOutConf(StreamKind::PIPE); }
  static StreamOutConf StdoutPipe() {
    return StreamOutConf(StreamKind::STDOUT_PIPE);
  }
  static StreamOutConf File(Fd&& fd) {
    return StreamOutConf(StreamKind::FILE, std::move(fd));
  }
  static StreamOutConf DevNull() {
    return StreamOutConf(StreamKind::FILE, Fd::take(open("/dev/null", O_RDWR)));
  }

  // Process-lib internal use only.
  StreamKind kind() const { return kind_; }
  Fd&& take_fd() { return std::move(fd_); }

 private:
  StreamOutConf(StreamKind kind, Fd&& fd = Fd())
      : kind_(kind), fd_(std::move(fd)) {}

  StreamKind kind_ = StreamKind::NONE;
  Fd fd_;
};

class StreamOut {
 public:
  StreamOut() = default;
  StreamOut(StreamKind kind, Fd&& fd = Fd())
      : kind_(kind), fd_(std::move(fd)) {}
  int fd() const { return *fd_; }
  bool is_pipe() const { return kind_ == StreamKind::PIPE; }

 private:
  StreamKind kind_ = StreamKind::NONE;
  Fd fd_;
};

#endif
