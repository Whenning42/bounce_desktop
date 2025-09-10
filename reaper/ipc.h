#ifndef REAPER_IPC_
#define REAPER_IPC_

#include <algorithm>
#include <chrono>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "third_party/status/status_or.h"

const char* const kReaperIpcFileEnvVar = "REAPER_IPC_FILE";

enum class ReaperState {
  STARTING = 0,
  RUNNING = 1,
  FAILED_START = 2,
  CLEANING_UP = 3,
  FINISHED_CLEANUP = 4,
  FAILED = 5,
};

inline StatusVal write_state(const std::string& ipc_file, ReaperState state) {
  std::ofstream file(ipc_file);
  if (!file.is_open()) return InvalidArgumentError();

  file << static_cast<int>(state);
  if (!file.good()) return InvalidArgumentError();
  return OkStatus();
}

inline StatusOr<ReaperState> read_state(const std::string& ipc_file) {
  std::ifstream file(ipc_file);
  if (!file.is_open()) return InvalidArgumentError();

  int state_int;
  if (!(file >> state_int)) return InvalidArgumentError();
  return static_cast<ReaperState>(state_int);
}

inline StatusOr<ReaperState> wait_for_state(
    const std::string& ipc_file, std::vector<ReaperState> states,
    std::chrono::nanoseconds timeout = std::chrono::seconds(5)) {
  auto start = std::chrono::steady_clock::now();

  while (std::chrono::steady_clock::now() - start < timeout) {
    StatusOr<ReaperState> current_state = read_state(ipc_file);
    RETURN_IF_ERROR(current_state);

    if (std::find(states.begin(), states.end(), *current_state) !=
        states.end()) {
      return current_state;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
  }
  return DeadlineExceededError();
}

static inline StatusVal clean_up_ipc(const std::string& ipc_file) {
  int r = std::remove(ipc_file.c_str());
  if (r != 0) {
    return InvalidArgumentError();
  }
  return OkStatus();
}

#endif
