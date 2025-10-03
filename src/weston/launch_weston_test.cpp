#include "weston/launch_weston.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thread>

#include "paths.h"
#include "third_party/status/status_gtest.h"

void close_proc(int pid) {
  kill(pid, SIGTERM);
  waitpid(pid, nullptr, 0);
  // Give any children time to exit in case they do so promptly and
  // asynchronously.
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

TEST(LaunchWeston, launch_succeeds) {
  auto r = launch_weston(5950, {get_export_display_path(), "test"});
  EXPECT_OK(r)
  if (r.ok()) {
    close_proc(r->pid);
  }
}

TEST(LaunchWeston, port_taken_gives_unavailable_error) {
  auto a = launch_weston(5951, {get_export_display_path(), "test"});
  auto b = launch_weston(5951, {get_export_display_path(), "test"});
  EXPECT_THAT(b, StatusIs(StatusCode::UNAVAILABLE)) << b.status().to_string();
  if (a.ok()) {
    close_proc(a->pid);
  }
}

TEST(LaunchWeston, launch_failure_gives_unknown_error) {
  auto r = launch_weston(5952, {"a_command_that_doesnt_exist"});
  EXPECT_FALSE(r.ok());
  EXPECT_THAT(r, StatusIs(StatusCode::UNKNOWN)) << r.to_string();
}
