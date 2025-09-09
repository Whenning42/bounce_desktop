// A subprocess reaper.
// clang-format off
// Example usage:
// $ PROCESS_MANAGER_EXIT_FILE=/tmp/my_exit process_manager weston --xwayland -- my_app &
// $ touch /tmp/my_exit
// clang-format on

#include "process_manager.h"

int main(int argc, char** argv) {
  char** subproc_argv = &argv[1];

  char* exit_var = getenv("PROCESS_MANAGER_EXIT_FILE");
  if (!exit_var) {
    fprintf(stderr, "A PROCESS_MANAGER_EXIT_FILE env var is required.\n");
    return -1;
  }

  ProcessManager p = ProcessManager(subproc_argv, std::string(exit_var));
  p.run();
}
