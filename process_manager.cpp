#include "process_manager.h"

#include <signal.h>
#include <spawn.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <time.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <vector>

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

void wait_all() {
  while (true) {
    int r = waitpid(-1, nullptr, WNOHANG);
    if (r == 0) break;
    if (r == -1) {
      if (errno != ECHILD) {
        perror("waitpid");
      }
      break;
    }
  }
}
}  // namespace

ProcessManager::ProcessManager(char** argv, const std::string& exit_file) {
  exit_file_ = exit_file;
  if (prctl(PR_SET_CHILD_SUBREAPER, 1)) {
    perror("Set child subreaper.");
  }

  const char* sh = "sh";
  const char* c = "-c";
  std::string command = "";
  int i = 0;
  while (true) {
    if (!argv[i]) break;
    command += argv[i];
    command += " ";
    i++;
  }

  char* shell_argv[] = {(char*)sh, (char*)c, (char*)command.c_str(), nullptr};
  int pid;
  int r = posix_spawnp(&pid, "sh", /*file_actions=*/nullptr, /*attr=*/nullptr,
                       shell_argv, environ);
  if (r) {
    errno = r;
    perror("posix_spawnp");
  }
}

void ProcessManager::run() {
  while (true) {
    sleep(1);
    if (std::filesystem::exists(exit_file_)) {
      break;
    }
    wait_all();
  }
  exit();
}

void ProcessManager::exit() {
  printf("Exiting process manager.\n");
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
  remove(exit_file_.c_str());
}
