#include "process.h"

#include <gtest/gtest.h>

#include <fstream>

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

TEST(ProcessTest, no_redirect) {
  ASSERT_OK_AND_ASSIGN(Process p,
                       run_test(StreamOutConf::None(), StreamOutConf::None()));

  EXPECT_FALSE(p.stdout.is_pipe());
  EXPECT_FALSE(p.stderr.is_pipe());
}

TEST(ProcessTest, pipe_stdout) {
  ASSERT_OK_AND_ASSIGN(Process p,
                       run_test(StreamOutConf::Pipe(), StreamOutConf::None()));

  EXPECT_TRUE(p.stdout.is_pipe());
  EXPECT_EQ(read_stdout(p), "test_out");
  EXPECT_FALSE(p.stderr.is_pipe());
}

TEST(ProcessTest, pipe_stderr) {
  ASSERT_OK_AND_ASSIGN(Process p,
                       run_test(StreamOutConf::None(), StreamOutConf::Pipe()));

  EXPECT_TRUE(p.stderr.is_pipe());
  EXPECT_EQ(read_stderr(p), "test_err");
  EXPECT_FALSE(p.stdout.is_pipe());
}

TEST(ProcessTest, pipe_merged_stdout_stderr) {
  ASSERT_OK_AND_ASSIGN(
      Process p, run_test(StreamOutConf::Pipe(), StreamOutConf::StdoutPipe()));

  EXPECT_TRUE(p.stdout.is_pipe());
  EXPECT_EQ(read_stdout(p), "test_outtest_err");
}

TEST(ProcessTest, redirect_to_filename) {
  char out_name[L_tmpnam];
  char err_name[L_tmpnam];
  tmpnam(out_name);
  tmpnam(err_name);

  ASSERT_OK_AND_ASSIGN(
      Process p,
      run_test(std::move(StreamOutConf::File(out_name).value_or_die()),
               std::move(StreamOutConf::File(err_name).value_or_die())));
  std::ifstream out_file(out_name);
  std::ifstream err_file(err_name);
  EXPECT_TRUE(out_file.is_open());
  EXPECT_TRUE(err_file.is_open());
  std::string out;
  std::string err;
  out_file >> out;
  err_file >> err;
  EXPECT_EQ(out, "test_out");
  EXPECT_EQ(err, "test_err");
}

TEST(ProcessTest, redirect_to_fds) {
  char out_name[L_tmpnam];
  char err_name[L_tmpnam];
  tmpnam(out_name);
  tmpnam(err_name);
  int out_fd = open(out_name, O_WRONLY | O_CREAT, 0644);
  ASSERT_NE(out_fd, -1);
  int err_fd = open(err_name, O_WRONLY | O_CREAT, 0644);
  ASSERT_NE(err_fd, -1);
  ASSERT_OK_AND_ASSIGN(Process p,
                       run_test(StreamOutConf::File(Fd::take(out_fd)),
                                StreamOutConf::File(Fd::take(err_fd))));

  std::ifstream out_file(out_name);
  std::ifstream err_file(err_name);
  EXPECT_TRUE(out_file.is_open());
  EXPECT_TRUE(err_file.is_open());
  std::string out;
  std::string err;
  out_file >> out;
  err_file >> err;
  EXPECT_EQ(out, "test_out");
  EXPECT_EQ(err, "test_err");
}

TEST(ProcessTest, devnull) {
  ASSERT_OK_AND_ASSIGN(
      Process p, run_test(StreamOutConf::DevNull(), StreamOutConf::DevNull()));

  // We don't have a way to check the process's stdout and stderr directly to
  // verify that they're not being written to, but since DevNull redirect works
  // by opening an fd to devnull and then doing redirect to an fd, we just
  // verify that we output that we're redirecting stdout and stderr to files in
  // this test.
  EXPECT_NE(p.stdout.fd(), -1);
  EXPECT_NE(p.stderr.fd(), -1);
}
