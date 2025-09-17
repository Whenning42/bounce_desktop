#include "process.h"

#include <gtest/gtest.h>

#include "third_party/status/status_gtest.h"

StatusOr<Process> run_test(ProcessOut stdout, ProcessOut stderr) {
  auto p = launch_process(
      {"sh", "-c", R"(printf "test_out"; printf "test_err" 1>&2;)"},
      /*env=*/nullptr, /*stdout=*/stdout, /*stderr=*/stderr);
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  return p;
}

std::string read_stdout(const Process& p) {
  char buf[64];
  int r = read(p.stdout_pipe, &buf, 64);
  buf[r] = '\0';
  return std::string(buf);
}

std::string read_stderr(const Process& p) {
  char buf[64];
  int r = read(p.stderr_pipe, &buf, 64);
  buf[r] = '\0';
  return std::string(buf);
}

TEST(ProcessTest, no_capture_test) {
  ASSERT_OK_AND_ASSIGN(Process p, run_test(ProcessOut::NONE, ProcessOut::NONE));

  EXPECT_EQ(p.stdout_pipe, -1);
  EXPECT_EQ(p.stderr_pipe, -1);
}

TEST(ProcessTest, capture_stdout) {
  ASSERT_OK_AND_ASSIGN(Process p, run_test(ProcessOut::PIPE, ProcessOut::NONE));

  ASSERT_NE(p.stdout_pipe, -1);
  EXPECT_EQ(read_stdout(p), "test_out");
  EXPECT_EQ(p.stderr_pipe, -1);
}

TEST(ProcessTest, capture_stderr) {
  ASSERT_OK_AND_ASSIGN(Process p, run_test(ProcessOut::NONE, ProcessOut::PIPE));

  ASSERT_NE(p.stderr_pipe, -1);
  EXPECT_EQ(read_stderr(p), "test_err");
  EXPECT_EQ(p.stdout_pipe, -1);
}

TEST(ProcessTest, capture_merged_out) {
  ASSERT_OK_AND_ASSIGN(Process p,
                       run_test(ProcessOut::PIPE, ProcessOut::STDOUT));

  ASSERT_NE(p.stdout_pipe, -1);
  EXPECT_EQ(read_stdout(p), "test_outtest_err");
  EXPECT_EQ(p.stderr_pipe, -1);
}
