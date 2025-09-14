#include "reaper/impl.h"

#include <fcntl.h>
#include <poll.h>
#include <signal.h>
#include <spawn.h>
#include <sys/inotify.h>
#include <sys/prctl.h>
#include <sys/signalfd.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>
#include <thread>
#include <vector>

#include "process.h"
#include "reaper/cleanup.h"
#include "reaper/ipc.h"
#include "reaper/protocol.h"
#include "third_party/status/logger.h"

bool parent_died = false;
ReaperImpl* global_impl = nullptr;

namespace {
using std::chrono::milliseconds;
using std::this_thread::sleep_for;

const int32_t kNumPolls = 3;
enum PollSlots {
  kParentIdx = 0,
  kIpcIdx = 1,
  kSigchldIdx = 2,
};

int make_sigchld_signalfd_and_block_signal() {
  // Block SIGCHLD.
  sigset_t sigchld_mask;
  sigemptyset(&sigchld_mask);
  sigaddset(&sigchld_mask, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &sigchld_mask, nullptr) == -1) {
    FATAL("sigprocmask" + libc_error_name(errno));
  }

  // Open sigchld signalfd.
  int sigchld_fd = signalfd(-1, &sigchld_mask, SFD_CLOEXEC);
  if (sigchld_fd == -1) {
    FATAL("signalfd" + libc_error_name(errno));
  }
  return sigchld_fd;
}

void set_child_subreaper() {
  if (prctl(PR_SET_CHILD_SUBREAPER, 1)) {
    FATAL("Set child subreaper:" + libc_error_name(errno));
  }
}

void exit_handler(int) { global_impl->on_exit(); }
}  // namespace

StatusOr<ReaperImpl> ReaperImpl::create(const std::string& command,
                                        const Token& token) {
  ASSIGN_OR_RETURN(auto ipc, IPC<ReaperMessage>::connect(token));
  return ReaperImpl(command, token, std::move(ipc));
}

void ReaperImpl::run() {
  set_child_subreaper();
  setup_signal_handlers();
  int sigchld_signalfd = make_sigchld_signalfd_and_block_signal();
  int parent_pidfd = ipc_.receive_fd().value_or_die();
  owned_files_ = OwnedFds(parent_pidfd, sigchld_signalfd);

  StatusOr<Process> p = launch_process({"sh", "-c", command_});
  if (!p.ok()) {
    ipc_.send(ReaperMessage{ReaperMessageCode::INVALID_COMMAND});
    ERROR(p.to_string());
    return;
  }

  // Check if the launched process exits quickly after launch. This can happen
  // if you give the shell an invalid command and so we return an
  // INVALID_COMMAND value.
  sleep_for(milliseconds(20));
  int stat;
  int wait_pid = waitpid(p->pid, &stat, WNOHANG);
  if (wait_pid != 0 && stat != 0) {
    ipc_.send(ReaperMessage{ReaperMessageCode::INVALID_COMMAND});
    ERROR("Launched program exited shortly after launch.");
    return;
  }

  // Send launch success message
  ReaperMessage success_msg{.code = ReaperMessageCode::FINISHED_LAUNCH};
  CHECK_OK(ipc_.send(success_msg));

  pollfd poll_fds[kNumPolls];
  poll_fds[kParentIdx] =
      pollfd{.fd = parent_pidfd, .events = POLLIN, .revents = 0};
  poll_fds[kIpcIdx] =
      pollfd{.fd = ipc_.socket(), .events = POLLIN, .revents = 0};
  poll_fds[kSigchldIdx] =
      pollfd{.fd = sigchld_signalfd, .events = POLLIN, .revents = 0};

  while (true) {
    int r = poll(poll_fds, kNumPolls, -1);
    CHECK(r >= 0);
    if (r == 0) continue;

    if (poll_fds[kParentIdx].revents) {
      parent_died = true;
      on_exit();
    }

    if (poll_fds[kIpcIdx].revents) {
      StatusOr<ReaperMessage> msg = ipc_.receive(/*block=*/true);
      if (!msg.ok() && msg.status().code() == StatusCode::ABORTED) {
        on_exit();
      }
      if (msg.ok() && msg->code == ReaperMessageCode::CLEAN_UP) {
        on_exit();
      }
      CHECK_OK(msg);
    }

    if (poll_fds[kSigchldIdx].revents) {
      signalfd_siginfo si;
      read(poll_fds[2].fd, &si, sizeof(si));
      wait_all();
    }
  }
}

void ReaperImpl::setup_signal_handlers() {
  assert(global_impl == nullptr);
  global_impl = this;
  signal(SIGINT, exit_handler);
  signal(SIGTERM, exit_handler);
}

void ReaperImpl::on_exit() {
  // Block signals to make procedure async-signal-safe.
  // This is fine since we're tearing down the process anyway.
  sigset_t all_signals;
  sigfillset(&all_signals);
  sigprocmask(SIG_BLOCK, &all_signals, nullptr);

  close_all_descendants();

  if (parent_died) {
    ipc_.cleanup_from_client();
    exit(0);
  }

  ReaperMessage finished_msg{.code = ReaperMessageCode::FINISHED_CLEANING_UP};
  CHECK_OK(ipc_.send(finished_msg));
  exit(0);
}
