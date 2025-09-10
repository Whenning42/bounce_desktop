#ifndef REAPER_REAPER_
#define REAPER_REAPER_

#include <chrono>
#include <string>

#include "third_party/status/status_or.h"

namespace reaper {

// TODO: Make reaper exit when it doesn't have any children left.

// Runs the given 'command' in a process manager reaper.
// 'ipc_file' is a file the process manager will listen to for cleanup requests.
// Returns the PID of the launched reaper process once it has started 'command'.
// Returns INVALID_ARGUMENT if the reaper can't launch 'command'.
//
// This function can be extended with optional arguments to support stdin/stdout
// redirection for processes running under the reaper.
StatusOr<int> launch_reaper(const std::string& command,
                            const std::string& ipc_file);

// Request that the process manager listening to 'ipc_file' clean up all of its
// descendants and blocks until 'ipc_file' is updated with the cleanup
// confirmation. Returns DEADLINE_EXCEEDED if the cleanup doesn't finish before
// the timeout.
StatusVal clean_up(const std::string& ipc_file,
                   std::chrono::nanoseconds timeout = std::chrono::seconds(1));

}  // namespace reaper

#endif
