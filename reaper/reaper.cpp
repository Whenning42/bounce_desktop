#include "reaper/reaper.h"

#include <spawn.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>

#include "reaper/ipc.h"

namespace reaper {

// Creates environment array with REAPER_IPC_FILE set to ipc_file
char** make_reaper_env(const std::string& ipc_file,
                       std::string* reaper_env_var_out) {
  *reaper_env_var_out = std::string(kReaperIpcFileEnvVar) + "=" + ipc_file;

  // Count existing environment variables
  int env_count = 0;
  while (environ[env_count]) env_count++;

  // Create new environment array with additional IPC variable
  char** new_environ = new char*[env_count + 2];
  for (int i = 0; i < env_count; i++) {
    new_environ[i] = environ[i];
  }
  new_environ[env_count] = const_cast<char*>(reaper_env_var_out->c_str());
  new_environ[env_count + 1] = nullptr;

  return new_environ;
}

StatusOr<int> Reaper::launch() {
  Token token;
  ASSIGN_OR_RETURN(ipc_, IPC<ReaperMessage>::create(ipc_dir_, &token));

  // Launch reaper executable with the command and IPC env var.
  std::string reaper_env_var;
  char** reaper_env = make_reaper_env(token, &reaper_env_var);
  const char* reaper_path = "./build/reaper";
  char* reaper_argv[] = {const_cast<char*>("reaper"),
                         const_cast<char*>(command_.c_str()), nullptr};
  int pid;
  int r = posix_spawnp(&pid, reaper_path, /*file_actions=*/nullptr,
                       /*attr=*/nullptr, reaper_argv, reaper_env);
  delete[] reaper_env;

  if (r) {
    errno = r;
    perror("posix_spawnp");
    return InternalError("Failed to launch the reaper");
  }

  // Create pidfd for the current process and send it to ReaperImpl
  int pidfd = syscall(SYS_pidfd_open, getpid(), 0);
  if (pidfd < 0) {
    perror("pidfd_open syscall in Reaper");
    return InternalError("Failed to create pidfd");
  }

  StatusVal send_fd_result = ipc_.send_fd(pidfd);
  close(pidfd);  // Close it in Reaper, ReaperImpl has its own copy now
  if (!send_fd_result.ok()) {
    return InternalError("Failed to send pidfd to reaper");
  }

  ASSIGN_OR_RETURN(ReaperMessage message, ipc_.receive());
  if (message.code != ReaperMessageCode::FINISHED_LAUNCH) {
    return InvalidArgumentError("Reaper failed to launch the subcommand");
  }
  return pid;
}

StatusVal Reaper::clean_up(std::chrono::nanoseconds timeout) {
  // If the reaper's already been closed by being directly signaled, then
  // its state will be FINISHED_CLEANING_UP and we can exit without doing
  // anything.
  StatusOr<ReaperMessage> r = ipc_.receive(/*blocking=*/false);
  if (r.ok() && r->code == ReaperMessageCode::FINISHED_CLEANING_UP) {
    return OkStatus();
  }
  if (r.ok() && (int)r->code > 4) {
    return InternalError(
        std::format("Invalid reply from reaper? {}", (int)r->code));
  }

  // The reaper can already have exited by now either cleanly, via
  // sigint/sigterm, or dirtily, via sigkill. We don't have a nice way to
  // distinguish the two cases, and neither is expected normal use. We just
  // return a NotFound error when this happens.
  StatusVal status =
      ipc_.send(ReaperMessage{.code = ReaperMessageCode::CLEAN_UP});
  if (status.code() == StatusCode::ABORTED) {
    return NotFoundError(
        "Called clean_up on a reaper that's no longer running");
  } else {
    RETURN_IF_ERROR(status);
  }

  StatusOr<ReaperMessage> m = ipc_.receive(/*blocking=*/true);
  RETURN_IF_ERROR(m);

  if (m->code != ReaperMessageCode::FINISHED_CLEANING_UP) {
    return UnknownError("Reaper returned seemingly impossible message.");
  }
  return OkStatus();
}

}  // namespace reaper
