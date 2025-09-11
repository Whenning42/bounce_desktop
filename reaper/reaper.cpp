#include "reaper/reaper.h"

#include <spawn.h>

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

StatusOr<int> launch_reaper(const std::string& command,
                            const std::string& ipc_file) {
  std::ofstream of(ipc_file);
  write_state(ipc_file, ReaperState::LAUNCHING);

  std::string reaper_env_var;
  char** reaper_env = make_reaper_env(ipc_file, &reaper_env_var);

  // Launch reaper executable with the command
  const char* reaper_path = "./build/reaper";
  char* reaper_argv[] = {const_cast<char*>("reaper"),
                         const_cast<char*>(command.c_str()), nullptr};
  int pid;
  int r = posix_spawnp(&pid, reaper_path, /*file_actions=*/nullptr,
                       /*attr=*/nullptr, reaper_argv, reaper_env);

  delete[] reaper_env;

  if (r) {
    errno = r;
    perror("posix_spawnp");
    return StatusOr<int>(StatusCode::INVALID_ARGUMENT);
  }

  StatusOr<ReaperState> state = wait_for_state(
      ipc_file, {ReaperState::RUNNING, ReaperState::FAILED_START});
  RETURN_IF_ERROR(state);
  if (*state != ReaperState::RUNNING) {
    return InvalidArgumentError();
  }
  return pid;
}

StatusVal clean_up(const std::string& ipc_file,
                   std::chrono::nanoseconds timeout) {
  // If the reaper's already been closed by being directly signaled, then
  // it's state will be FINISHED_CLEANING_UP and we can exit without doing
  // anything.
  StatusOr<ReaperState> read_status = read_state(ipc_file);
  RETURN_IF_ERROR(read_status);
  if (*read_status == ReaperState::FINISHED) {
    return OkStatus();
  }

  StatusVal write_status = write_state(ipc_file, ReaperState::CLEANING_UP);
  RETURN_IF_ERROR(write_status);

  StatusOr<ReaperState> wait_status = wait_for_state(
      ipc_file, {ReaperState::FINISHED}, /*timeout=*/timeout);
  RETURN_IF_ERROR(wait_status);
  printf("Reaper-launcher deleting ipc file: %s\n", ipc_file.c_str());
  remove(ipc_file.c_str());
  return OkStatus();
}

}  // namespace reaper
