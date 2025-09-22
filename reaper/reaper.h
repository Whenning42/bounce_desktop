#ifndef REAPER_REAPER_H_
#define REAPER_REAPER_H_

#include <chrono>
#include <string>

#include "process.h"
#include "reaper/ipc.h"
#include "reaper/protocol.h"
#include "third_party/status/status_or.h"

namespace reaper {

class Reaper {
 public:
  // Create a Reaper instance with the given command and IPC directory.
  static StatusOr<Reaper> create(const std::string& command,
                                 const std::string& ipc_dir);

  // Disallow default construction, copying and assignment
  Reaper() = delete;
  Reaper(const Reaper&) = delete;
  Reaper& operator=(const Reaper&) = delete;

  // Allow moving
  Reaper(Reaper&&) = default;
  Reaper& operator=(Reaper&&) = default;

  // Runs the given 'command' under the reaper.
  //
  // Returns an INVALID_ARGUMENT error if the process fails to launch, or if it
  // exits quickly after launching.
  StatusVal launch();

  Process& process() { return reaper_; }

  // Requests that the reqper stop all of its descendants and waits for a
  // confirmation from the reaper that is succeeded.
  //
  // Returns 'false' if the launcher's unable to receive a confirmation from the
  // reaper that the shutdown happened successfully. In this case, processes
  // will still have been reaped as long as the reaper hasn't crashed or been
  // sigkilled.
  bool clean_up();

 private:
  Reaper(const std::string& command, const std::string& ipc_dir,
         IPC<ReaperMessage> ipc, Token token)
      : command_(command),
        ipc_dir_(ipc_dir),
        ipc_(std::move(ipc)),
        ipc_token_(std::move(token)) {}
  std::string command_;
  std::string ipc_dir_;
  IPC<ReaperMessage> ipc_;
  Token ipc_token_;
  Process reaper_;
};

}  // namespace reaper

#endif
