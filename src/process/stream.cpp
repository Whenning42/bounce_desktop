#include "process/stream.h"

#include <errno.h>
#include <fcntl.h>

#include <format>

#include "libc_error.h"
#include "third_party/status/status_or.h"

StreamOutConf StreamOutConf::None() { return StreamOutConf(StreamKind::NONE); }

StreamOutConf StreamOutConf::Pipe() { return StreamOutConf(StreamKind::PIPE); }

StreamOutConf StreamOutConf::StdoutPipe() {
  return StreamOutConf(StreamKind::STDOUT_PIPE);
}

StreamOutConf StreamOutConf::File(Fd&& fd) {
  return StreamOutConf(StreamKind::FILE, std::move(fd));
}

StatusOr<StreamOutConf> StreamOutConf::File(const char* filename) {
  int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd == -1) {
    if (errno == ENOENT) {
      return InvalidArgumentError(
          std::format("Couldn't find file: {}", filename));
    } else {
      return InternalError(
          std::format("File open failed: {}", libc_error_name(errno).c_str()));
    }
  }
  return StreamOutConf(StreamKind::FILE, Fd::take(fd));
}

StatusOr<StreamOutConf> StreamOutConf::File(const std::string& filename) {
  return File(filename.c_str());
}

StreamOutConf StreamOutConf::DevNull() {
  return StreamOutConf(StreamKind::FILE, Fd::take(open("/dev/null", O_RDWR)));
}

StreamKind StreamOutConf::kind() const { return kind_; }

Fd&& StreamOutConf::take_fd() { return std::move(fd_); }

StreamOutConf::StreamOutConf(StreamKind kind, Fd&& fd)
    : kind_(kind), fd_(std::move(fd)) {}

StreamOut::StreamOut(StreamKind kind, Fd&& fd)
    : kind_(kind), fd_(std::move(fd)) {}

int StreamOut::fd() const { return *fd_; }

bool StreamOut::is_pipe() const { return kind_ == StreamKind::PIPE; }
