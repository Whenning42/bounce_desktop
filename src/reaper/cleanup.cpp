#include "reaper/cleanup.h"

#include <sys/wait.h>
#include <unistd.h>

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
}  // namespace

void wait_all() {
  while (true) {
    int r = waitpid(-1, nullptr, WNOHANG);
    if (r == 0) break;
    if (r == -1) {
      break;
    }
  }
}

void close_all_descendants() {
  while (true) {
    std::vector<int> children = get_children();
    if (children.size() == 0) {
      break;
    }

    for (int child : children) {
      sigterm(child);
    }
    usleep(100'000);
    wait_all();

    for (int child : get_children()) {
      auto it = std::find(children.begin(), children.end(), child);
      if (it != children.end()) {
        sigkill(child);
      }
    }
    wait_all();
  }
}
