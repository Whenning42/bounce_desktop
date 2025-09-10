// A subprocess reaper.
// clang-format off
// Example usage:
// $ REAPER_IPC_FILE=/tmp/my_ipc reaper "weston --xwayland -- my_app" &
// clang-format on

#include "impl.h"
#include "ipc.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <command>\n", argv[0]);
    return -1;
  }

  // Join the non-program values of argv into a single command string.
  std::string command;
  for (int i = 1; i < argc; ++i) {
    if (i > 1) command += " ";
    command += argv[i];
  }

  char* ipc_file_var = getenv(kReaperIpcFileEnvVar);
  if (!ipc_file_var) {
    fprintf(stderr, "A %s env var is required.\n", kReaperIpcFileEnvVar);
    return -1;
  }

  ReaperImpl p = ReaperImpl(command, std::string(ipc_file_var));
  p.run();
}
