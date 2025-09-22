#include "reaper/reaper.h"

#include <spawn.h>
#include <sys/syscall.h>
#include <unistd.h>

#include <cstdlib>
#include <cstring>
#include <fstream>

#include "process.h"
#include "reaper/ipc.h"

namespace reaper {
namespace {

using std::chrono::seconds;
using std::chrono::steady_clock;

template <typename T>
void log_error(T v) {
  ERROR("%s", to_string(v).c_str());
}
}  // namespace

StatusOr<Reaper> Reaper::create(const std::string& command,
                                const std::string& ipc_dir) {
  // Open the IPC connection.
  Token token;
  ASSIGN_OR_RETURN(auto ipc, IPC<ReaperMessage>::create(ipc_dir, &token));
  return Reaper(command, ipc_dir, std::move(ipc), std::move(token));
}

StatusVal Reaper::launch() {
  // Launch the reaper.
  EnvVars env = EnvVars::environ();
  env.set_var(kReaperIpcFileEnvVar, ipc_token_.c_str());
  ASSIGN_OR_RETURN(Process p,
                   launch_process({"./build/reaper", command_}, &env));

  // Open this process's pidfd to send to the reaper as the reaper's parent.
  int pidfd = syscall(SYS_pidfd_open, getpid(), 0);
  if (pidfd < 0) {
    return InternalError("Failed to create pidfd: " + libc_error_name(errno));
  }

  // Send this process's pidfd as the parent to the reaper.
  StatusVal s = ipc_.send_fd(pidfd);
  fprintf(stderr, "Launcher connected at pidfd: %d\n", ipc_.connected());
  close(pidfd);
  RETURN_IF_ERROR(s);

  // Verify launch ran successfully.
  ASSIGN_OR_RETURN(ReaperMessage message, ipc_.receive());
  fprintf(stderr, "Launcher connected at state receive: %d\n",
          ipc_.connected());
  if (message.code != ReaperMessageCode::FINISHED_LAUNCH) {
    return InvalidArgumentError("Reaper failed to launch the subcommand");
  }
  reaper_ = std::move(p);
  return OkStatus();
}

bool Reaper::clean_up() {
  fprintf(stderr, "Launcher connected at clean up receive: %d\n",
          ipc_.connected());
  StatusOr<ReaperMessage> r = ipc_.receive(/*blocking=*/false);
  if (!r.ok() && r.status().code() != StatusCode::UNAVAILABLE) {
    log_error(r);
    return false;
  }
  if (r.ok() && r->code == ReaperMessageCode::FINISHED_CLEANING_UP) {
    // If the reaper's already cleaned up, return OK.
    return true;
  }

  StatusVal s = ipc_.send(ReaperMessage{.code = ReaperMessageCode::CLEAN_UP});
  if (!s.ok()) {
    log_error(s);
    return false;
  }

  auto start = steady_clock::now();
  auto timeout = seconds(10);
  StatusOr<ReaperMessage> m =
      ReaperMessage{.code = ReaperMessageCode::FINISHED_CLEANING_UP};
  while (steady_clock::now() - start < timeout) {
    m = ipc_.receive(/*blocking=*/false);
    if (m.ok() || m.status().code() != StatusCode::UNAVAILABLE) {
      break;
    }
  }
  if (!m.ok()) {
    log_error(m);
    return false;
  }

  return true;
}

}  // namespace reaper
