#include "reaper/reaper.h"

#include <gtest/gtest.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <string>
#include <thread>

#include "reaper/ipc.h"
#include "reaper/process.h"

namespace {

using std::this_thread::sleep_for;
using std::chrono::steady_clock;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;

int last_ipc_file_num = 0;


std::string get_next_ipc_file_name() {
  std::string name =
      "/tmp/reaper_test_ipc_" + std::to_string(last_ipc_file_num);
  last_ipc_file_num++;
  return name;
}

// Runs reaper with ptree app and returns process ID
int run_reaper_ptree(const std::string& args, const std::string& ipc_file) {
  std::string command = "python3 ./reaper/tests/reaper_ptree.py " + args;
  StatusOr<int> result = reaper::launch_reaper(command, ipc_file);
  // Give the reaper and the process tree time to start up.
  sleep_for(milliseconds(100));
  return result.value_or_die();
}

void killall_ptree() { system("pkill -f .*reaper_ptree.py.*"); }

void killall_reaper() { system("killall -q -9 reaper"); }

int count_ptree() {
  FILE* pipe = popen("pgrep reaper_ptree.py | wc -l", "r");
  if (!pipe) {
    std::exit(1);
  }

  int count;
  if (fscanf(pipe, "%d", &count) != 1) {
    pclose(pipe);
    std::exit(1);
  }
  pclose(pipe);
  return count;
}

int count_reaper() {
  system("echo 'Reapers:'; pgrep -l reaper");
  FILE* pipe = popen("pgrep ^reaper$ | wc -l", "r");
  if (!pipe) {
    std::exit(1);
  }

  int count;
  if (fscanf(pipe, "%d", &count) != 1) {
    pclose(pipe);
    std::exit(1);
  }
  pclose(pipe);
  return count;
}

void EXPECT_ptree_is_cleaned_up(milliseconds timeout = milliseconds(300)) {
  auto start = steady_clock::now();
  int count = -1;

  while (steady_clock::now() - start < timeout) {
    count = count_ptree();
    if (count == 0) {
      break;
    }
    sleep_for(milliseconds(10));
  }

  EXPECT_EQ(count, 0);
}

void EXPECT_reaper_closes(milliseconds timeout = milliseconds(500)) {
  auto start = steady_clock::now();
  int count;
  while (steady_clock::now() - start < timeout) {
    count = count_reaper();
    if (count == 0) {
      break;
    }
    sleep_for(milliseconds(10));
  }
  EXPECT_EQ(count, 0);
}

void send_sigint(int pid) { kill(pid, SIGINT); }

void send_sigterm(int pid) { kill(pid, SIGTERM); }

// Reads reaper state from IPC file and expects it to match expected state
void EXPECT_reaper_state(const std::string& ipc_file,
                         ReaperState expected_state) {
  auto start = steady_clock::now();
  ReaperState state;
  while (steady_clock::now() - start < milliseconds(200)) {
    StatusOr<ReaperState> state = read_state(ipc_file);
    if (!state.ok() && state.status().code() == StatusCode::UNAVAILABLE) continue;
    if (*state == expected_state) break;
  }
  EXPECT_EQ(state, expected_state);
}

void EXPECT_ipc_file_is_cleaned_up(const std::string& ipc_file) {
  auto start = steady_clock::now();
  milliseconds timeout = milliseconds(1000);
  bool file_exists = true;
  while (steady_clock::now() - start < timeout) {
    file_exists = std::filesystem::exists(ipc_file);
    if (!file_exists) break;
  }
  EXPECT_FALSE(file_exists);
}

}  // anonymous namespace

class ReaperTest : public ::testing::Test {
 protected:
  std::string ipc_file_;

  void SetUp() override {
    // Clean up any leftover processes from previous tests
    killall_ptree();
    killall_reaper();
    waitpid(-1, nullptr, WNOHANG);

    // Get unique IPC file name and remove any existing file
    ipc_file_ = get_next_ipc_file_name();
    std::remove(ipc_file_.c_str());
  }

  void TearDown() override {
    // Clean up any leftover processes after each test
    killall_ptree();
    killall_reaper();
  }
};

TEST_F(ReaperTest, ChildReapTest) {
  run_reaper_ptree("0", ipc_file_);
  EXPECT_ptree_is_cleaned_up();
}

TEST_F(ReaperTest, OrphanReapTest) {
  run_reaper_ptree("0 50", ipc_file_);
  EXPECT_ptree_is_cleaned_up();
}

TEST_F(ReaperTest, SigintExitTest) {
  int reaper_pid = run_reaper_ptree("-1 -1", ipc_file_);
  EXPECT_reaper_state(ipc_file_, ReaperState::RUNNING);

  send_sigint(reaper_pid);

  EXPECT_ptree_is_cleaned_up();
  EXPECT_reaper_state(ipc_file_, ReaperState::FINISHED);
  ASSERT_TRUE(reaper::clean_up(ipc_file_).ok());
}

TEST_F(ReaperTest, SigtermExitTest) {
  int reaper_pid = run_reaper_ptree("-1 -1", ipc_file_);
  EXPECT_reaper_state(ipc_file_, ReaperState::RUNNING);

  send_sigterm(reaper_pid);

  EXPECT_ptree_is_cleaned_up();
  EXPECT_reaper_state(ipc_file_, ReaperState::FINISHED);
  ASSERT_TRUE(reaper::clean_up(ipc_file_).ok());
}

TEST_F(ReaperTest, CleanupExitTest) {
  run_reaper_ptree("-1 -1", ipc_file_);

  EXPECT_TRUE(reaper::clean_up(ipc_file_).ok());

  // Expect that the processes are gone and the that ipc file has been cleaned up.
  EXPECT_ptree_is_cleaned_up();
  EXPECT_FALSE(std::filesystem::exists(ipc_file_));
}

TEST_F(ReaperTest, ParentExitTest) {
  // Launch a parent process, that launches the reaper, that launches descendants.
  // The parent's configured to exit after 100ms while the children run indefinitely.
  // Expect that the parent runs successfully and that after it exists, the reaper
  // cleans up the descendants.
  setenv("REAPER_IPC_FILE", ipc_file_.c_str(), 1);
  int pid = launch_process({
    "python3", "./reaper/tests/reaper_parent.py", ipc_file_, "-1", "-1"
  }).value_or_die();
  unsetenv("REAPER_IPC_FILE");
  int status;
  waitpid(pid, &status, 0);
  EXPECT_EQ(status, 0);

  // Expect that the processes are gone and the that ipc file has been cleaned up.
  EXPECT_ptree_is_cleaned_up();
  EXPECT_ipc_file_is_cleaned_up(ipc_file_);
}

TEST_F(ReaperTest, CleanupRemovesIpcFile) {
  run_reaper_ptree("0", ipc_file_);

  ASSERT_TRUE(std::filesystem::exists(ipc_file_));
  ASSERT_TRUE(reaper::clean_up(ipc_file_).ok());
  EXPECT_FALSE(std::filesystem::exists(ipc_file_));
}

TEST_F(ReaperTest, InvalidCommandError) {
  StatusOr<int> result =
      reaper::launch_reaper("/nonexistent/command/that/should/fail", ipc_file_);

  EXPECT_FALSE(result.ok());
  EXPECT_EQ(result.status().code(), StatusCode::INVALID_ARGUMENT);
}

TEST_F(ReaperTest, ExpectReaperExitsAfterChildren) {
  run_reaper_ptree("50", ipc_file_);
  ASSERT_EQ(count_reaper(), 1);
  EXPECT_reaper_closes();
}
