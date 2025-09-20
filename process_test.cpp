#include "process.h"

#include <gtest/gtest.h>

#include "third_party/status/status_gtest.h"

StatusOr<Process> run_test(StreamOutConf&& stdout, StreamOutConf&& stderr) {
  auto p = launch_process(
      {"sh", "-c", R"(printf "test_out"; printf "test_err" 1>&2;)"},
      /*env=*/nullptr,
      ProcessOutConf{.stdout = std::move(stdout), .stderr = std::move(stderr)});
  std::this_thread::sleep_for(std::chrono::milliseconds(50));
  return p;
}

std::string read_stdout(const Process& p) {
  char buf[64];
  int r = read(p.stdout.fd(), &buf, 64);
  buf[r] = '\0';
  return std::string(buf);
}

std::string read_stderr(const Process& p) {
  char buf[64];
  int r = read(p.stderr.fd(), &buf, 64);
  buf[r] = '\0';
  return std::string(buf);
}

TEST(ProcessTest, no_capture_test) {
  ASSERT_OK_AND_ASSIGN(Process p,
                       run_test(StreamOutConf::None(), StreamOutConf::None()));

  EXPECT_FALSE(p.stdout.is_pipe());
  EXPECT_FALSE(p.stderr.is_pipe());
}

TEST(ProcessTest, capture_stdout) {
  ASSERT_OK_AND_ASSIGN(Process p,
                       run_test(StreamOutConf::Pipe(), StreamOutConf::None()));

  EXPECT_TRUE(p.stdout.is_pipe());
  EXPECT_EQ(read_stdout(p), "test_out");
  EXPECT_FALSE(p.stderr.is_pipe());
}

TEST(ProcessTest, capture_stderr) {
  ASSERT_OK_AND_ASSIGN(Process p,
                       run_test(StreamOutConf::None(), StreamOutConf::Pipe()));

  EXPECT_TRUE(p.stderr.is_pipe());
  EXPECT_EQ(read_stderr(p), "test_err");
  EXPECT_FALSE(p.stdout.is_pipe());
}

TEST(ProcessTest, capture_merged_out) {
  ASSERT_OK_AND_ASSIGN(
      Process p, run_test(StreamOutConf::Pipe(), StreamOutConf::StdoutPipe()));

  EXPECT_TRUE(p.stdout.is_pipe());
  EXPECT_EQ(read_stdout(p), "test_outtest_err");
}
