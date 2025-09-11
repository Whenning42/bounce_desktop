// A process reaper class. See process_manager_main.cpp for public interface.
//
// TODO: Correctly handle signals, EINTR errors, and unexpected parent exits.

#ifndef REAPER_IMPL_H_
#define REAPER_IMPL_H_

#include <unistd.h>

#include <string>

#include "reaper/ipc.h"
#include "reaper/protocol.h"

struct Files {
  int parent_fd = -1;
  int sigchld_fd = -1;
  void cleanup() {
    if (parent_fd != -1) close(parent_fd);
    if (sigchld_fd != -1) close(sigchld_fd);
  }
};

class ReaperImpl {
 public:
  // Launch the reaper with the given 'command' running under it.
  // Use 'token' to connect to IPC, report launch results and receive cleanup
  // requests. Blocks until the process manager exits.
  ReaperImpl(const std::string& command, const Token& token)
      : command_(command), token_(token) {}

  // Run the reaper's main loop. Start by launching the 'command' given in the
  // constructor. Then poll for any reapable children, changes to ipc_file, or
  // the parent exiting, and resond accordingly.
  void run();

  // Exits all of the reaper's descendants, first by trying SIGTERM, and then by
  // SIGKILL if they don't respond to the SIGTERM. Also deletes the 'ipc_file'
  // if the reaper is exiting because the parent has exited without calling
  // clean_up.
  void on_exit();

  // Reap any zombie children.
  void on_sigchld();

 private:
  void setup_signal_handlers();

  std::string command_;
  Token token_;
  IPC<ReaperMessage> ipc_;
  Files files_;
};

#endif
