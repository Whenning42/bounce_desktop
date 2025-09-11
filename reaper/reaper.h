#ifndef REAPER_REAPER_
#define REAPER_REAPER_

#include <chrono>
#include <string>

#include "reaper/ipc.h"
#include "reaper/protocol.h"
#include "third_party/status/status_or.h"

namespace reaper {

class Reaper {
 public:
  Reaper(const std::string& command, const std::string& ipc_dir)
      : command_(command), ipc_dir_(ipc_dir) {}

  // Runs the given 'command' in a process manager reaper.
  // 'ipc_file' is a file the process manager will listen to for cleanup
  // requests. Returns the PID of the launched reaper process once it has
  // started 'command'. Returns INVALID_ARGUMENT if the reaper can't launch
  // 'command'.
  //
  // This function can be extended with optional arguments to support
  // stdin/stdout redirection for processes running under the reaper.
  StatusOr<int> launch();

  // Request that the process manager listening to 'ipc_file' clean up all of
  // its descendants and block until 'ipc_file' is updated with the cleanup
  // confirmation.
  //
  // Returns DEADLINE_EXCEEDED if the cleanup doesn't finish before the timeout.
  StatusVal clean_up(
      std::chrono::nanoseconds timeout = std::chrono::seconds(1));

 private:
  std::string command_;
  std::string ipc_dir_;
  IPC<ReaperMessage> ipc_;
  Token ipc_token_;
};

}  // namespace reaper

#endif
