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
#include <vector>

#include "reaper/ipc.h"
#include "third_party/status/logger.h"

const int32_t kNumPolls = 3;

bool delete_ipc_file = false;
ReaperImpl* global_impl = nullptr;

namespace {
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

void on_failed_start(const std::string& ipc_file) {
  write_state(ipc_file, ReaperState::FAILED_START);
  ::exit(1);
}

void on_fail(const std::string& ipc_file) {
  write_state(ipc_file, ReaperState::FAILED);
  ::exit(1);
}

void make_parent_pollfd(const std::string& ipc_file, pollfd* poll_fd) {
  int ppid = getppid();
  int p_fd = syscall(SYS_pidfd_open, ppid, 0);
  if (p_fd < 0) {
    perror("pidfd_open syscall");
    on_fail(ipc_file);
  }

  *poll_fd = pollfd{.fd = p_fd, .events = POLLIN};
}

void make_ipc_file_pollfd(const std::string& ipc_file, pollfd* poll_fd) {
  int ipc_fd = inotify_init1(IN_CLOEXEC);
  if (ipc_fd == -1) {
    perror("inotify_init1");
    on_fail(ipc_file);
  }

  int w = inotify_add_watch(ipc_fd, ipc_file.c_str(), IN_MODIFY);
  if (w == -1) {
    perror("inotify_add_watch");
    on_fail(ipc_file);
  }

  *poll_fd = pollfd{.fd = ipc_fd, .events = POLLIN};
}

void make_sigchld_pollfd_and_block_signal(const std::string& ipc_file,
                                          pollfd* poll_fd) {
  // Block SIGCHLD and create signalfd for it
  sigset_t sigchld_mask;
  sigemptyset(&sigchld_mask);
  sigaddset(&sigchld_mask, SIGCHLD);
  if (sigprocmask(SIG_BLOCK, &sigchld_mask, nullptr) == -1) {
    perror("sigprocmask");
    on_fail(ipc_file);
  }

  int sigchld_fd = signalfd(-1, &sigchld_mask, SFD_CLOEXEC);
  if (sigchld_fd == -1) {
    perror("signalfd");
    on_fail(ipc_file);
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
  if (!std::filesystem::exists(ipc_file_)) {
    ERROR("Reaper can't find ipc_file: %s", ipc_file_.c_str());
    on_failed_start(ipc_file_);
  }

  // Set up pollfds that listen for the parent exiting and IPC changes
  pollfd poll_fds[kNumPolls];
  make_parent_pollfd(ipc_file_, &poll_fds[0]);
  make_ipc_file_pollfd(ipc_file_, &poll_fds[1]);
  files_.parent_fd = poll_fds[0].fd;
  files_.ipc_file_fd = poll_fds[1].fd;

  if (prctl(PR_SET_CHILD_SUBREAPER, 1)) {
    perror("Set child subreaper.");
    on_fail(ipc_file_);
  }

  const char* sh = "sh";
  const char* c = "-c";
  char* shell_argv[] = {const_cast<char*>(sh), const_cast<char*>(c),
                        const_cast<char*>(command_.c_str()), nullptr};
  int pid;
  int r = posix_spawnp(&pid, "sh", /*file_actions=*/nullptr, /*attr=*/nullptr,
                       shell_argv, environ);
  if (r) {
    errno = r;
    perror("posix_spawnp");
    on_failed_start(ipc_file_);
  }
  // Handle case where process launches, but fails immediately afterwards, like
  // we have when sh gives "command not found".
  std::this_thread::sleep_for(std::chrono::milliseconds(100));
  int stat_loc;
  int waited = waitpid(pid, &stat_loc, WNOHANG);
  if (waited) {
    if (stat_loc) {
      on_failed_start(ipc_file_);
    }
  }

  // Set up SIGCHLD handling after spawning the child
  make_sigchld_pollfd_and_block_signal(ipc_file_, &poll_fds[2]);
  files_.sigchld_fd = poll_fds[2].fd;
  write_state(ipc_file_, ReaperState::RUNNING);

  setup_signal_handlers();

  while (true) {
    int r = poll(poll_fds, kNumPolls, -1);
    if (r < 0) {
      perror("poll");
      write_state(ipc_file_, ReaperState::FAILED);
      ::exit(1);
    }
    if (r == 0) continue;

    if (poll_fds[0].revents) {
      // Parent exited. Exit.
      delete_ipc_file = true;
      on_exit();
    }
    if (poll_fds[1].revents) {
      // Ipc file has been updated. Check for exit request.
      // Read the ipc ionotify to clear its readable state.
      char buf[4096];
      read(poll_fds[1].fd, &buf, sizeof(buf));

      StatusOr<ReaperState> state = read_state(ipc_file_);
      if (!state.ok()) {
        write_state(ipc_file_, ReaperState::FAILED);
        ::exit(1);
      }
      if (*state == ReaperState::CLEANING_UP) {
        on_exit();
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
  write_state(ipc_file_, ReaperState::FINISHED_CLEANUP);
  if (delete_ipc_file) {
    remove(ipc_file_.c_str());
  }
  exit(0);
}

void ReaperImpl::on_sigchld() { wait_all(); }
