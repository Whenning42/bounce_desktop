// A process reaper class. See process_manager_main.cpp for public interface.
//
// TODO: Correctly handle signals, EINTR errors, and unexpected parent exits.

#ifndef REAPER_IMPL_H_
#define REAPER_IMPL_H_

#include <unistd.h>

#include <string>

#include "reaper/ipc.h"
#include "reaper/protocol.h"

class OwnedFds {
 public:
  OwnedFds() = default;
  OwnedFds(int parent_fd, int sigchld_fd)
      : parent_fd_(parent_fd), sigchld_fd_(sigchld_fd) {}
  ~OwnedFds() {
    if (parent_fd_ != -1) close(parent_fd_);
    if (sigchld_fd_ != -1) close(sigchld_fd_);
  }

  // Delete copies.
  OwnedFds(const OwnedFds&) = delete;
  OwnedFds& operator=(const OwnedFds&) = delete;

  // Expected move operators.
  OwnedFds(OwnedFds&& other) noexcept
      : parent_fd_(other.parent_fd_), sigchld_fd_(other.sigchld_fd_) {
    other.parent_fd_ = -1;
    other.sigchld_fd_ = -1;
  }
  OwnedFds& operator=(OwnedFds&& other) noexcept {
    if (this != &other) {
      if (parent_fd_ != -1) close(parent_fd_);
      if (sigchld_fd_ != -1) close(sigchld_fd_);
      parent_fd_ = other.parent_fd_;
      sigchld_fd_ = other.sigchld_fd_;
      other.parent_fd_ = -1;
      other.sigchld_fd_ = -1;
    }
    return *this;
  }

 private:
  int parent_fd_ = -1;
  int sigchld_fd_ = -1;
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

 private:
  void setup_signal_handlers();

  std::string command_;
  Token token_;
  IPC<ReaperMessage> ipc_;
  OwnedFds owned_files_;
};

#endif
