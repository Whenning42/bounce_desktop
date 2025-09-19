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
  auto var = rhs;                                \
  ASSERT_TRUE(var.ok());                         \
  lhs = std::move(var.value());

struct M {
  int32_t v = 0;
};

int test_num = 0;
class IpcTest : public testing::Test {
 protected:
  std::string ipc_dir_;
  int fd_0_;
  int fd_1_;

  M m_0_ = M{.v = 0};
  M m_1_ = M{.v = 1};

  IpcTest() {
    ipc_dir_ =
        std::format("/run/user/{}/bounce_ipc_test_{}", getuid(), test_num);
    test_num++;
    std::filesystem::create_directory(ipc_dir_);
    fd_0_ = open("/dev/null", O_RDONLY);
    fd_1_ = open("/dev/null", O_RDONLY);
  }

  ~IpcTest() override {
    std::filesystem::remove_all(ipc_dir_);

    close(fd_0_);
    close(fd_1_);
  }

  std::tuple<IPC<M>, IPC<M>> make_ipcs() {
    Token token;
    IPC<M> a = std::move(IPC<M>::create(ipc_dir_, &token).value_or_die());
    IPC<M> b = std::move(IPC<M>::connect(token).value_or_die());
    return {std::move(a), std::move(b)};
  }

  IPC<M> keep_just_server() {
    auto [a, b] = make_ipcs();
    return std::move(a);
  };

  IPC<M> keep_just_client() {
    auto [a, b] = make_ipcs();
    return std::move(b);
  };
};

TEST_F(IpcTest, SendAndReceive) {
  auto [a, b] = make_ipcs();

  ASSERT_TRUE(a.send(m_0_).ok());
  ASSERT_TRUE(b.send(m_1_).ok());

  ASSERT_OK_AND_ASSIGN(M m_0_other, b.receive(true));
  ASSERT_OK_AND_ASSIGN(M m_1_other, a.receive(true));

  EXPECT_EQ(m_0_.v, m_0_other.v);
  EXPECT_EQ(m_1_.v, m_1_other.v);
}

TEST_F(IpcTest, SendAndReceiveFds) {
  auto [a, b] = make_ipcs();

  ASSERT_TRUE(a.send_fd(fd_0_).ok());
  ASSERT_TRUE(b.send_fd(fd_1_).ok());

  ASSERT_OK_AND_ASSIGN(int fd_0_other, b.receive_fd(true));
  ASSERT_OK_AND_ASSIGN(int fd_1_other, a.receive_fd(true));

  EXPECT_GE(fd_0_other, 0);
  EXPECT_GE(fd_1_other, 0);
}

TEST_F(IpcTest, NonBlockingReceive) {
  auto [a, b] = make_ipcs();
  StatusOr<M> recv = a.receive(/*block=*/false);
  EXPECT_EQ(recv.status().code(), StatusCode::UNAVAILABLE);
}

TEST_F(IpcTest, NonBlockingReceiveFd) {
  auto [a, b] = make_ipcs();
  StatusOr<int> recv_fd = a.receive_fd(/*block=*/false);
  EXPECT_EQ(recv_fd.status().code(), StatusCode::UNAVAILABLE);
}

TEST_F(IpcTest, SendToHungUpPeer) {
  {
    auto a = keep_just_server();
    EXPECT_EQ(a.send(m_0_).status().code(), StatusCode::ABORTED);
  }
  {
    auto b = keep_just_client();
    EXPECT_EQ(b.send(m_1_).status().code(), StatusCode::ABORTED);
  }
}

TEST_F(IpcTest, SendFdToHungUpPeer) {
  {
    auto a = keep_just_server();
    EXPECT_EQ(a.send_fd(fd_0_).status().code(), StatusCode::ABORTED);
  }
  {
    auto b = keep_just_client();
    EXPECT_EQ(b.send_fd(fd_1_).status().code(), StatusCode::ABORTED);
  }
}

TEST_F(IpcTest, ReceiveFromHungUpPeer) {
  {
    auto a = keep_just_server();
    EXPECT_EQ(a.receive().status().code(), StatusCode::ABORTED);
  }
  {
    auto b = keep_just_client();
    EXPECT_EQ(b.receive().status().code(), StatusCode::ABORTED);
  }
}

TEST_F(IpcTest, ReceiveFdFromHungUpPeer) {
  {
    auto a = keep_just_server();
    EXPECT_EQ(a.receive_fd().status().code(), StatusCode::ABORTED);
  }
  {
    auto b = keep_just_client();
    EXPECT_EQ(b.receive_fd().status().code(), StatusCode::ABORTED);
  }
}
