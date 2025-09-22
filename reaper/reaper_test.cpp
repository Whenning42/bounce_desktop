#include "reaper/reaper.h"

#include <assert.h>
#include <gmock/gmock.h>
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

#include "process.h"
#include "reaper/ipc.h"
#include "third_party/status/status_gtest.h"

namespace {

using std::chrono::milliseconds;
using std::chrono::nanoseconds;
using std::chrono::steady_clock;
using std::this_thread::sleep_for;

std::string get_ipc_dir() {
  return "/run/user/" + std::to_string(getuid()) + "/bounce_ipc_test";
}

// Runs reaper with ptree app and returns Reaper instance
// Optionally outputs the PID via the pid parameter
reaper::Reaper run_reaper_ptree(const std::string& args,
                                const std::string& ipc_dir,
                                int* pid = nullptr) {
  std::string command = "python3 ./reaper/tests/reaper_ptree.py " + args;
  reaper::Reaper reaper =
      std::move(reaper::Reaper::create(command, ipc_dir).value_or_die());
  assert(reaper.launch().code() == StatusCode::OK);
  if (pid) *pid = reaper.process().pid;

  // Give the reaper and the process tree time to start up.
  sleep_for(milliseconds(100));
  return reaper;
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
    sleep_for(milliseconds(50));
  }

  EXPECT_EQ(count, 0);
}

void send_sigint(int pid) { kill(pid, SIGINT); }

void send_sigterm(int pid) { kill(pid, SIGTERM); }

}  // anonymous namespace

class ReaperTest : public ::testing::Test {
 protected:
  std::string ipc_dir_;

  void SetUp() override {
    // Clean up any leftover processes from previous tests
    killall_ptree();
    killall_reaper();
    waitpid(-1, nullptr, WNOHANG);

    // Create IPC directory
    ipc_dir_ = get_ipc_dir();
    std::filesystem::remove_all(ipc_dir_);
    std::filesystem::create_directories(ipc_dir_);
  }

  void TearDown() override {
    // Clean up any leftover processes after each test
    killall_ptree();
    killall_reaper();
    // Remove IPC directory
    std::filesystem::remove_all(ipc_dir_);
  }
};

TEST_F(ReaperTest, ChildReapTest) {
  reaper::Reaper reaper = run_reaper_ptree("0", ipc_dir_);
  EXPECT_ptree_is_cleaned_up();
}

TEST_F(ReaperTest, OrphanReapTest) {
  reaper::Reaper reaper = run_reaper_ptree("0 50", ipc_dir_);
  EXPECT_ptree_is_cleaned_up();
}

TEST_F(ReaperTest, SigintExitTest) {
  int reaper_pid;
  reaper::Reaper reaper = run_reaper_ptree("-1 -1", ipc_dir_, &reaper_pid);

  send_sigint(reaper_pid);

  EXPECT_ptree_is_cleaned_up();
  // When reaper exits via a signal, we can't guarantee a clean clean-up. but
  // clean-up should proceed without any fatal errors.
  reaper.clean_up();
}

TEST_F(ReaperTest, SigtermExitTest) {
  int reaper_pid;
  reaper::Reaper reaper = run_reaper_ptree("-1 -1", ipc_dir_, &reaper_pid);

  send_sigterm(reaper_pid);

  EXPECT_ptree_is_cleaned_up();
  // When reaper exits via a signal, we can't guarantee a clean clean-up. but
  // clean-up should proceed without any fatal errors.
  reaper.clean_up();
}

TEST_F(ReaperTest, CleanupExitTest) {
  reaper::Reaper reaper = run_reaper_ptree("-1 -1", ipc_dir_);
  EXPECT_TRUE(reaper.clean_up());
  EXPECT_ptree_is_cleaned_up();
}

TEST_F(ReaperTest, ParentExitTest) {
  Process p = launch_process({"./build/reaper_tests_reaper_parent", ipc_dir_})
                  .value_or_die();

  int status;
  int r = waitpid(p.pid, &status, 0);
  EXPECT_GE(r, 1);
  EXPECT_EQ(WEXITSTATUS(status), 0);

  EXPECT_ptree_is_cleaned_up();
}

TEST_F(ReaperTest, InvalidCommandError) {
  reaper::Reaper reaper = std::move(
      reaper::Reaper::create("/nonexistent/command/that/should/fail", ipc_dir_)
          .value_or_die());
  StatusOr<Process> p = reaper.launch();

  EXPECT_THAT(p, StatusIs(StatusCode::INVALID_ARGUMENT));
}

TEST_F(ReaperTest, ReaperStaysOpenAfterChildren) {
  reaper::Reaper reaper = run_reaper_ptree("50", ipc_dir_);
  sleep_for(milliseconds(1000));
  EXPECT_EQ(count_reaper(), 1);
  EXPECT_TRUE(reaper.clean_up());
}
