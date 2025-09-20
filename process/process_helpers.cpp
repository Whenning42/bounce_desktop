#include "process/process_helpers.h"

#include <spawn.h>

StatusVal validate_process_out_conf(const ProcessOutConf& conf) {
  if (conf.stderr.kind() == StreamKind::STDOUT_PIPE &&
      conf.stdout.kind() != StreamKind::PIPE) {
    return InvalidArgumentError(
        "ProcessOutConf with stderr = StdoutPipe and stdout != Pipe isn't "
        "valid.");
  }
  if (conf.stdout.kind() == StreamKind::STDOUT_PIPE) {
    return InvalidArgumentError(
        "ProcessOutConf with stdout = StdoutPipe isn't valid.");
  }
  return OkStatus();
}

PrelaunchOut process_streams_prelaunch(ProcessOutConf&& out_conf) {
  StreamOut stdout;
  StreamOut stderr;
  std::vector<Fd> close_after_spawn;
  std::vector<int> subproc_close;

  // Each input can be one of NONE, PIPE, STDOUT_PIPE, or FILE.
  posix_spawn_file_actions_t actions;
  CHECK(posix_spawn_file_actions_init(&actions) == 0);

  int stdout_write_fd = -1;
  switch (out_conf.stdout.kind()) {
    case StreamKind::PIPE: {
      int p[2];
      CHECK(pipe(p) == 0);

      CHECK(posix_spawn_file_actions_adddup2(&actions, p[1], STDOUT_FILENO) ==
            0);
      subproc_close.push_back(p[0]);
      subproc_close.push_back(p[1]);

      stdout = StreamOut(StreamKind::PIPE, Fd::take(p[0]));
      close_after_spawn.push_back(Fd::take(p[1]));

      stdout_write_fd = p[1];
      break;
    }
    case StreamKind::FILE: {
      stdout = StreamOut(StreamKind::FILE, out_conf.stdout.take_fd());
      break;
    }
    case StreamKind::NONE:
    case StreamKind::STDOUT_PIPE: {
      // Nothing to do.
      break;
    }
  }

  switch (out_conf.stderr.kind()) {
    case StreamKind::PIPE: {
      int p[2];
      CHECK(pipe(p) == 0);

      CHECK(posix_spawn_file_actions_adddup2(&actions, p[1], STDERR_FILENO) ==
            0);
      subproc_close.push_back(p[0]);
      subproc_close.push_back(p[1]);

      stderr = StreamOut(StreamKind::PIPE, Fd::take(p[0]));
      close_after_spawn.push_back(Fd::take(p[1]));
      break;
    }
    case StreamKind::STDOUT_PIPE: {
      CHECK(stdout_write_fd != -1);
      CHECK(posix_spawn_file_actions_adddup2(&actions, stdout_write_fd,
                                             STDERR_FILENO) == 0);
      break;
    }
    case StreamKind::FILE: {
      stderr = StreamOut(StreamKind::FILE, out_conf.stderr.take_fd());
      break;
    }
    case StreamKind::NONE: {
      // Nothing to do.
      break;
    }
  }

  for (int fd : subproc_close) {
    CHECK(posix_spawn_file_actions_addclose(&actions, fd) == 0);
  }

  return PrelaunchOut{.file_actions = std::move(actions),
                      .stdout = std::move(stdout),
                      .stderr = std::move(stderr),
                      .close_after_spawn = std::move(close_after_spawn)};
}
