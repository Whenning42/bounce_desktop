#include <sys/wait.h>

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "reaper/reaper.h"
#include "third_party/status/logger.h"
#include "third_party/status/status_or.h"

using std::chrono::milliseconds;
using std::this_thread::sleep_for;

int main(int argc, char* argv[]) {
  if (argc != 2) {
    std::cerr << "Usage: reaper_parent <ipc_dir>" << std::endl;
    return 1;
  }

  std::string ipc_dir = argv[1];

  // Create reaper and launch the ptree command
  StatusOr<reaper::Reaper> reaper = reaper::Reaper::create(
      "python3 ./reaper/tests/reaper_ptree.py -1 -1", ipc_dir);
  CHECK_OK(reaper);
  StatusOr<Process> result = reaper->launch();
  if (!result.ok()) {
    ERROR("Failed to launch the reaper: %s",
          result.status().to_string().c_str());
    return 1;
  }

  int reaper_pid = result.value().pid;
  sleep_for(milliseconds(100));

  int status;
  int wait_result = waitpid(reaper_pid, &status, WNOHANG);

  if (wait_result > 0) {
    if (WIFEXITED(status)) {
      int exit_code = WEXITSTATUS(status);
      if (exit_code != 0) {
        ERROR("Reaper exited with non-zero code %d", exit_code);
      }
    } else if (WIFSIGNALED(status)) {
      int signal = WTERMSIG(status);
      ERROR("Reaper was killed by signal: %d", signal);
    }
  }

  return 0;
}
