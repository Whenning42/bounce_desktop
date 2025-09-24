#include "process.h"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include "time_aliases.h"

namespace {
void terminate_process(int pid) {
  if (pid == -1) return;
  kill(pid, SIGTERM);
  auto start = sc_now();
  while (sc_now() - start < 1000ms) {
    sleep_for(10ms);
    int r = waitpid(pid, nullptr, WNOHANG);
    if (r == -1 && errno != ECHILD) perror("terminate_process waitpid");
    if (r == 0) continue;
    if (r > 0) return;
  }
  kill(pid, SIGKILL);
}
}  // namespace

Process::~Process() { terminate_process(pid); }

Process::Process(Process&& other) {
  pid = other.pid;
  other.pid = -1;

  stdout = std::move(other.stdout);
  stderr = std::move(other.stderr);
}

Process& Process::operator=(Process&& other) {
  pid = other.pid;
  other.pid = -1;

  stdout = std::move(other.stdout);
  stderr = std::move(other.stderr);
  return *this;
}

StatusOr<Process> launch_process(const std::vector<std::string>& args,
                                 EnvVars* env_vars,
                                 ProcessOutConf&& process_out) {
  RETURN_IF_ERROR(validate_process_out_conf(process_out));

  char** argv = static_cast<char**>(malloc(sizeof(char*) * (args.size() + 1)));
  for (size_t i = 0; i < args.size(); ++i) {
    argv[i] = strdup(args[i].c_str());
  }
  argv[args.size()] = nullptr;

  int pid;
  char** env = env_vars ? env_vars->vars() : environ;

  PrelaunchOut prelaunch;
  process_streams_prelaunch(std::move(process_out), &prelaunch);
  int r =
      posix_spawnp(&pid, argv[0], &prelaunch.file_actions, nullptr, argv, env);
  if (r != 0) {
    return InvalidArgumentError("Failed to launch process: " +
                                libc_error_name(r));
  }
  Process p;
  p.pid = pid;
  posix_spawn_file_actions_destroy(&prelaunch.file_actions);
  p.stdout = std::move(prelaunch.stdout);
  p.stderr = std::move(prelaunch.stderr);
  prelaunch.close_after_spawn.clear();

  for (size_t i = 0; i < args.size(); ++i) {
    free(argv[i]);
  }
  free(argv);
  return p;
}
