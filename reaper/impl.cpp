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

#include "reaper/ipc.h"
#include "reaper/process.h"
#include "reaper/protocol.h"
#include "third_party/status/logger.h"

const int32_t kNumPolls = 3;

bool parent_died = false;
bool ipc_is_live = true;

ReaperImpl* global_impl = nullptr;

namespace {

using std::chrono::milliseconds;
using std::this_thread::sleep_for;

std::vector<int> get_pid_tids(int pid) {
  std::vector<int> tids;
  std::error_code error;
  std::filesystem::path task_dir =
      std::filesystem::path("/proc") / std::to_string(pid) / "task";

  for (const auto& entry :
       std::filesystem::directory_iterator(task_dir, error)) {
    if (error) break;
    const std::string name = entry.path().filename().string();
    tids.push_back((int)std::stoll(name));
  }
  return tids;
}

std::vector<int> get_children() {
  int pid = getpid();
  std::vector<int> out;
  std::vector<int> tids = get_pid_tids(pid);
  for (int tid : tids) {
    std::ifstream in("/proc/" + std::to_string(pid) + "/task/" +
                     std::to_string(tid) + "/children");
    if (!in) continue;

    int id;
    while (in >> id) {
      out.push_back(id);
    }
  }
  return out;
}

void sigterm(int pid) { kill(pid, SIGTERM); }

void sigkill(int pid) { kill(pid, SIGKILL); }

// wait_all is async-signal-safe.
void wait_all() {
  while (true) {
    int r = waitpid(-1, nullptr, WNOHANG);
    if (r == 0) break;
    if (r == -1) {
      break;
    }
  }
}

void on_failed_start(IPC<ReaperMessage>& ipc) {
  ReaperMessage msg{.code = ReaperMessageCode::INVALID_COMMAND};
  ipc.send(msg);
  ::exit(1);
}

void on_fail(IPC<ReaperMessage>& ipc) {
  // ReaperMessage msg{.code = ReaperMessageCode::OTHER_FAILED_LAUNCH};
  // ipc.send(msg);
  ::exit(1);
}

void make_parent_pollfd(int parent_pidfd, pollfd* poll_fd) {
  *poll_fd = pollfd{.fd = parent_pidfd, .events = POLLIN};
}

void make_ipc_pollfd(IPC<ReaperMessage>& ipc, pollfd* poll_fd) {
  *poll_fd = pollfd{.fd = ipc.socket(), .events = POLLIN};
}

void make_sigchld_pollfd_and_block_signal(IPC<ReaperMessage>& ipc,
                                          pollfd* poll_fd) {
  // Block SIGCHLD and create signalfd for it
  sigset_t sigchld_mask;
  sigemptyset(&sigchld_mask);
  sigaddset(&sigchld_mask, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &sigchld_mask, nullptr) == -1) {
    perror("sigprocmask");
    on_fail(ipc);
  }

  int sigchld_fd = signalfd(-1, &sigchld_mask, SFD_CLOEXEC);
  if (sigchld_fd == -1) {
    perror("signalfd");
    on_fail(ipc);
  }

  *poll_fd = pollfd{.fd = sigchld_fd, .events = POLLIN};
}

void exit_handler(int) {
  sigset_t all_signals;
  sigfillset(&all_signals);
  sigprocmask(SIG_BLOCK, &all_signals, nullptr);

  global_impl->on_exit();
}
}  // namespace

void ReaperImpl::run() {
  // Connect to IPC using the token
  StatusOr<IPC<ReaperMessage>> ipc_result = IPC<ReaperMessage>::connect(token_);
  if (!ipc_result.ok()) {
    ERROR("Reaper failed to connect to IPC: %s",
          ipc_result.status().to_string().c_str());
    ::exit(1);
  }
  ipc_ = std::move(*ipc_result);

  // Receive parent pidfd from Reaper
  StatusOr<int> parent_pidfd_result = ipc_.receive_fd();
  if (!parent_pidfd_result.ok()) {
    ERROR("Failed to receive parent pidfd: %s",
          parent_pidfd_result.status().to_string().c_str());
    on_fail(ipc_);
  }
  int parent_pidfd = *parent_pidfd_result;

  setup_signal_handlers();

  // Set up pollfds that listen for the parent exiting and IPC changes
  pollfd poll_fds[kNumPolls];
  make_parent_pollfd(parent_pidfd, &poll_fds[0]);
  make_ipc_pollfd(ipc_, &poll_fds[1]);
  make_sigchld_pollfd_and_block_signal(ipc_, &poll_fds[2]);
  files_.parent_fd = poll_fds[0].fd;
  files_.ipc_file_fd = poll_fds[1].fd;
  files_.sigchld_fd = poll_fds[2].fd;

  if (prctl(PR_SET_CHILD_SUBREAPER, 1)) {
    perror("Set child subreaper.");
    on_fail(ipc_);
  }

  StatusOr<int> pid = launch_process({"sh", "-c", command_});
  if (!pid.ok()) {
    on_failed_start(ipc_);
  }

  // Handle case where process launches, but fails immediately afterwards, like
  // we have when sh gives "command not found".
  sleep_for(milliseconds(20));
  int stat_loc;
  int waited = waitpid(*pid, &stat_loc, WNOHANG);
  if (waited) {
    if (stat_loc) {
      on_failed_start(ipc_);
    }
  }

  // Send launch success message
  ReaperMessage success_msg{.code = ReaperMessageCode::FINISHED_LAUNCH};
  StatusVal send_result = ipc_.send(success_msg);
  if (!send_result.ok()) {
    ERROR("Failed to send launch success message");
    ::exit(1);
  }

  while (true) {
    int r = poll(poll_fds, kNumPolls, -1);
    if (r < 0) {
      perror("poll");
      ::exit(1);
    }
    if (r == 0) continue;

    if (poll_fds[0].revents) {
      // Parent exited. Exit.
      parent_died = true;
      on_exit();
    }
    if (poll_fds[1].revents) {
      // IPC message available
      StatusOr<ReaperMessage> msg = ipc_.receive(/*block=*/true);
      if (!msg.ok() && msg.status().code() == StatusCode::ABORTED) {
        ipc_is_live = false;
        on_exit();
      }
      if (msg->code == ReaperMessageCode::CLEAN_UP) {
        on_exit();
      }
      if (!msg.ok()) {
        ERROR("Failed to receive IPC message: %s",
              msg.status().to_string().c_str());
        ::exit(1);
      }
    }
    if (poll_fds[2].revents) {
      // SIGCHLD received via signalfd. Reap children.
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
  while (true) {
    std::vector<int> children = get_children();
    if (children.size() == 0) {
      break;
    }

    for (int child : children) {
      sigterm(child);
    }
    usleep(10'000);
    wait_all();

    for (int child : get_children()) {
      auto it = std::find(children.begin(), children.end(), child);
      if (it != children.end()) {
        sigkill(child);
      }
    }
    wait_all();
  }
  files_.cleanup();

  if (parent_died) {
    // Parent died, clean up socket file
    ipc_.cleanup_from_client();
  } else if (ipc_is_live) {
    // Normal cleanup, send finished message
    ReaperMessage finished_msg{.code = ReaperMessageCode::FINISHED_CLEANING_UP};
    StatusVal send_result = ipc_.send(finished_msg);
    if (!send_result.ok()) {
      ERROR("Failed to send cleanup finished message");
    }
  }
  exit(0);
}

void ReaperImpl::on_sigchld() { wait_all(); }
