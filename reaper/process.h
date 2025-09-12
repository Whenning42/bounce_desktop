#ifndef REAPER_PROCESS_H_
#define REAPER_PROCESS_H_

#include "third_party/status/status_or.h"

#include <vector>
#include <string>

#include <string.h>
#include <spawn.h>

class EnvVars {
 public:
  // Copy the given env vars into a new EnvVars instance.
  EnvVars(char** env = nullptr) {
    if (!env) return;
    size_t i = 0;
    while (true) {
      if (!env[i]) break;
      vars_.push_back(strdup(env[i]));
      i++;
    }
    vars_.push_back(nullptr);
  }

  ~EnvVars() {
    for (size_t i = 0; i < vars_.size(); ++i) {
      free(vars_[i]);
      vars_[i] = nullptr;
    }
  }

  void add_var(const char* var, const char* val) {
    std::string c_val = std::string(var) + "=" + std::string(val);
    vars_.back() = strdup(c_val.c_str());
    vars_.push_back(nullptr);
  }

  // Returns a copy of the process's environment.
  static EnvVars environ() { return EnvVars(::environ); }

  char** vars() { return &vars_[0]; }

 public:
  std::vector<char*> vars_;
};

// If no environment is passed in. Defaults to using the parent process's
// environment. To get an empty environment, default construct an EnvVars
// instance.
inline StatusOr<int> launch_process(const std::vector<std::string>& args,
                                    EnvVars* env_vars = nullptr) {
  char** argv = static_cast<char**>(malloc(sizeof(char*) * (args.size() + 1)));
  for (size_t i = 0; i < args.size(); ++i) {
    argv[i] = strdup(args[i].c_str());
  }
  argv[args.size()] = nullptr;

  int pid;
  char** env = env_vars ? env_vars->vars() : environ;
  int r = posix_spawnp(&pid, argv[0], nullptr, nullptr, argv, env);
  if (r != 0) {
    return InvalidArgumentError("Failed to launch process: " + libc_error_name(r));
  }

  for (size_t i = 0; i < args.size(); ++i) {
    free(argv[i]);
  }
  return pid;
}

#endif
