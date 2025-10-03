#ifndef PROCESS_STREAM_H_
#define PROCESS_STREAM_H_

#include <fcntl.h>

#include "process/fd.h"
#include "third_party/status/status_or.h"

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
  static StreamOutConf None();
  static StreamOutConf Pipe();
  static StreamOutConf StdoutPipe();
  static StreamOutConf File(Fd&& fd);
  // Tries to create the file if it doesn't already exist.
  static StatusOr<StreamOutConf> File(const char* filename);
  // Tries to create the file if it doesn't already exist.
  static StatusOr<StreamOutConf> File(const std::string& filename);
  static StreamOutConf DevNull();

  // Process-lib internal use only.
  StreamKind kind() const;
  Fd&& take_fd();

 private:
  StreamOutConf(StreamKind kind, Fd&& fd = Fd());

  StreamKind kind_ = StreamKind::NONE;
  Fd fd_;
};

class StreamOut {
 public:
  StreamOut() = default;
  StreamOut(StreamKind kind, Fd&& fd = Fd());
  int fd() const;
  bool is_pipe() const;

 private:
  StreamKind kind_ = StreamKind::NONE;
  Fd fd_;
};

#endif
