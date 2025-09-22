#include "launch_weston.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>

MATCHER_P(StatusIs, code, "") { return get_status_(arg).code() == code; }

void close_proc(int pid) {
  kill(pid, SIGTERM);
  waitpid(pid, nullptr, 0);
  // Give any children time to exit in case they do so promptly and
  // asynchronously.
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

TEST(LaunchWeston, launch_succeeds) {
  auto r = run_weston(5950, {"./build/export_display", "test"});
  EXPECT_TRUE(r.ok()) << r.status().to_string();
  if (r.ok()) {
    close_proc(r->pid);
  }
}

TEST(LaunchWeston, port_taken_gives_unavailable_error) {
  auto a = run_weston(5951, {"./build/export_display", "test"});
  auto b = run_weston(5951, {"./build/export_display", "test"});
  EXPECT_THAT(b, StatusIs(StatusCode::UNAVAILABLE));
  if (a.ok()) {
    close_proc(a->pid);
  }
}

TEST(LaunchWeston, launch_failure_gives_unknown_error) {
  auto r = run_weston(5952, {"a_command_that_doesnt_exist"});
  EXPECT_FALSE(r.ok());
  EXPECT_THAT(r, StatusIs(StatusCode::UNKNOWN)) << r.to_string();
}
