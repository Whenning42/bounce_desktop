#include "process.h"

#include <unistd.h>

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

  auto prelaunch_out = process_streams_prelaunch(std::move(process_out));
  int r = posix_spawnp(&pid, argv[0], &prelaunch_out.file_actions, nullptr,
                       argv, env);
  if (r != 0) {
    return InvalidArgumentError("Failed to launch process: " +
                                libc_error_name(r));
  }
  Process p;
  p.pid = pid;
  posix_spawn_file_actions_destroy(&prelaunch_out.file_actions);
  p.stdout = std::move(prelaunch_out.stdout);
  p.stderr = std::move(prelaunch_out.stderr);
  prelaunch_out.close_after_spawn.clear();

  for (size_t i = 0; i < args.size(); ++i) {
    free(argv[i]);
  }
  return p;
}
