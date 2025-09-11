#ifndef REAPER_IPC_
#define REAPER_IPC_

#include <algorithm>
#include <cassert>
#include <chrono>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include "third_party/status/status_or.h"

const char* const kReaperIpcFileEnvVar = "REAPER_IPC_FILE";

// Launcher inits at 0. Reaper goes from 0 to 1. Launcher goes from 1 to 2.
// Reaper goes from 2 to 3.
//
// Writes can only move a state forward. E.g. running can't override CleaningUp.
//
// Statuses are stored separately from states. Only OK status can be overridden
// by error statuses.
// 
// We need an IPC mutex to synchronize reads and writes to statuses and failures.
enum class ReaperState {
  LAUNCHING = 0,
  RUNNING = 1,
  CLEANING_UP = 2,
  FINISHED = 3,
};

enum class ReaperStatus {
  OK = 0,
  FAILED_LAUNCH = 1,
  OTHER_FAILURE = 2,
};

template <typename T>
class Ipc {
 public:
  Ipc(const std::string& name);
  T read();
  void write(const T& t);

 private:
};

void lock(const std::string& ipc_file) {

}

void unlock(const std::string& ipc_file) {

}

inline StatusOr<ReaperState> read_state(const std::string& ipc_file) {
  lock(ipc_file);
  std::ifstream file(ipc_file); assert(file.is_open());

  int state_int;
  bool wrote = bool(file >> state_int); assert(wrote);
  unlock(ipc_file);
  return static_cast<ReaperState>(state_int);
}

inline StatusVal write_state(const std::string& ipc_file, ReaperState state) {
  ASSIGN_OR_RETURN(ReaperState old_state, read_state(ipc_file));
  if (old_state >= state) return OkStatus();

  lock(ipc_file);
  std::ofstream file(ipc_file); assert(file.is_open());
  file << static_cast<int>(state); assert(file.good());
  unlock(ipc_file);
  return OkStatus();
}

inline StatusOr<ReaperStatus> read_status(const std::string& ipc_file) {
  lock(ipc_file);
  unlock(ipc_file);
}

inline StatusOr<ReaperStatus> write_status(const std::string& ipc_file, ReaperStatus status) {
  lock(ipc_file);
  unlock(ipc_file);
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
  lock(ipc_file);
  int r = std::remove(ipc_file.c_str());
  if (r != 0) {
    return InvalidArgumentError();
  }
  return OkStatus();
  unlock(ipc_file);
}

static bool ipc_exists(const std::string& ipc_file) {
  lock(ipc_file);
  // TODO: Return whether the ipc exists.
  unlock(ipc_file);
}

#endif
