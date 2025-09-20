#ifndef PROCESS_H_
#define PROCESS_H_

#include <spawn.h>
#include <string.h>

#include <string>
#include <vector>

#include "libc_error.h"
#include "process/env_vars.h"
#include "process/process_helpers.h"
#include "process/stream.h"
#include "third_party/status/status_or.h"

class Process {
 public:
  int pid;
  StreamOut stdout;
  StreamOut stderr;
};

// If no environment is passed in. Defaults to using the parent process's
// environment. To get an empty environment, default construct an EnvVars
// instance.
//
// ProcessOutConf must be moved into launch_process, since it may own FDs and
// launch process takes ownership of any specified passed in FDs.
StatusOr<Process> launch_process(
    const std::vector<std::string>& args, EnvVars* env_vars = nullptr,
    ProcessOutConf&& process_out = ProcessOutConf());

#endif
