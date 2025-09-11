#include "reaper/ipc.h"

#include <gtest/gtest.h>
#include <filesystem>
#include <fcntl.h>
#include <format>
#include <unistd.h>

#define CAT(a, b) a##b
#define CAT_(a, b) CAT(a, b)
#define UNIQ(v) CAT_(v, __COUNTER__)

#define ASSERT_OK_AND_ASSIGN(lhs, rhs) \
  ASSERT_OK_AND_ASSIGN_IMPL(UNIQ(v), lhs, rhs)

#define ASSERT_OK_AND_ASSIGN_IMPL(var, lhs, rhs) \
  auto var = rhs;                            \
  ASSERT_TRUE(var.ok());                           \
  lhs = var.value(); 


struct M {
  int32_t v = 0;
};

// Create an IPC directory at /run/user/uid/bounce_ipc_test/
// Create an IPC using this directory
// Connect to the IPC using the create returned token
//
// Test send and receiving a test message
// Test send and receiving a test message along with an fd

class IpcTest : public testing::Test {
 protected:
  std::string ipc_dir_;

  IpcTest() {
    ipc_dir_ = std::format("/run/user/{}/bounce_ipc_test", getuid());
    std::filesystem::create_directory(ipc_dir_);
  }

  ~IpcTest() override {
    std::filesystem::remove_all(ipc_dir_);
  }
};

TEST_F(IpcTest, SendAndReceive) {
  Token conn;
  ASSERT_OK_AND_ASSIGN(IPC<M> a, IPC<M>::create(ipc_dir_, &conn));
  ASSERT_OK_AND_ASSIGN(IPC<M> b, IPC<M>::connect(conn));
  M m_0 = M{.v = 0};
  M m_1 = M{.v = 1};

  ASSERT_TRUE(a.send(m_0).ok());
  ASSERT_TRUE(b.send(m_1).ok());
  ASSERT_OK_AND_ASSIGN(M m_0_other, b.receive(true));
  ASSERT_OK_AND_ASSIGN(M m_1_other, a.receive(true));

  EXPECT_EQ(m_0.v, m_0_other.v);
  EXPECT_EQ(m_1.v, m_1_other.v);
}

TEST_F(IpcTest, NonBlockingReceive) {
  Token conn;
  ASSERT_OK_AND_ASSIGN(IPC<M> a, IPC<M>::create(ipc_dir_, &conn));
  ASSERT_OK_AND_ASSIGN(IPC<M> b, IPC<M>::connect(conn));
  (void)b;

  StatusOr<M> recv = a.receive(/*block=*/false);
  EXPECT_EQ(recv.status().code(), StatusCode::UNAVAILABLE);
}

TEST_F(IpcTest, SendReceiveFds) {
  Token conn;
  ASSERT_OK_AND_ASSIGN(IPC<M> a, IPC<M>::create(ipc_dir_, &conn));
  ASSERT_OK_AND_ASSIGN(IPC<M> b, IPC<M>::connect(conn));

  int fd_0 = open("/dev/null", O_RDONLY);
  int fd_1 = open("/dev/null", O_RDONLY);
  ASSERT_TRUE(a.send_fd(fd_0).ok());
  ASSERT_TRUE(a.send_fd(fd_1).ok());
  ASSERT_OK_AND_ASSIGN(int fd_0_other, b.receive_fd(true));
  ASSERT_OK_AND_ASSIGN(int fd_1_other, b.receive_fd(true));

  EXPECT_GE(fd_0_other, 0);
  EXPECT_GE(fd_1_other, 0);
  close(fd_0);
  close(fd_1);
  close(fd_0_other);
  close(fd_1_other);
}
