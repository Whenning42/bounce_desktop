#ifndef PROCESS_H_
#define PROCESS_H_

#include <spawn.h>
#include <string.h>

#include <string>
#include <vector>

#include "libc_error.h"
#include "third_party/status/status_or.h"

class Process {
 public:
  int pid;
  int stdout_pipe;
  int stderr_pipe;

  // Moveable, not copyable.
  Process();
  ~Process();
  Process(Process&& other) noexcept;
  Process& operator=(Process&& other) noexcept;
  Process(Process& other) = delete;
  Process& operator=(Process& other) = delete;
};

enum class ProcessOut {
  // Don't redirect the output.
  NONE = 0,
  // Redirect the output to a pipe.
  PIPE = 1,
  // Only settable for stderr and when stdout is PIPE.
  // Requests merging stdout and stderr on the same pipe.
  STDOUT = 2,
};

class EnvVars {
 public:
  // Copies the given env vars into a new EnvVars instance.
  EnvVars(char** env = nullptr);
  ~EnvVars();

  // Adds the given variable and value to env vars.
  void add_var(const char* var, const char* val);

  // Returns a copy of the process's environment.
  static EnvVars environ();

  // Returns the env vars as a char**.
  char** vars();

 public:
  std::vector<char*> vars_;
};

// If no environment is passed in. Defaults to using the parent process's
// environment. To get an empty environment, default construct an EnvVars
// instance.
StatusOr<Process> launch_process(const std::vector<std::string>& args,
                                 EnvVars* env_vars = nullptr,
                                 ProcessOut stdout = ProcessOut::NONE,
                                 ProcessOut stderr = ProcessOut::NONE);

#endif
