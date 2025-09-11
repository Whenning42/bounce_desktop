#define REAPER_IPC_TPP_

#include "reaper/ipc.h"

#include <fcntl.h>
#include <cassert>
#include <format>
#include <filesystem>
#include <random>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>

namespace {

void check(int v, const char* msg) {
  if (v == -1) {
    perror(msg);
    exit(1);
  }
}

// Try to find a variation of our hard-coded path name and given
// dir along with a 48-bit random hex val that gives us an unused
// path.
//
// May return an already in-use path if too many tries fail.
const int kTries = 10'000;
const int kBits = 48;
std::string get_likely_available_path(const std::string& dir) {
  auto d = std::filesystem::path(dir);
  std::random_device device;
  std::mt19937_64 generator(device());
  std::uniform_int_distribution<uint64_t> distribution;
  std::string path;
  for (int i = 0; i < kTries; ++i) {
    uint64_t v = distribution(generator);
    v = v % (0x1UL << kBits);
    std::stringstream hex;
    hex << std::hex << v;
    path = d / std::format("bounce_ipc_socket_{}", hex.str());
    printf("Using path: %s\n", path.c_str());
    if (!std::filesystem::exists(path)) break;
  }
  return path;
}

sockaddr_un make_addr_un(const std::string& path) {
  sockaddr_un addr;
  addr.sun_family = AF_UNIX;
  strcpy(addr.sun_path, path.c_str());
  return addr;
}

int make_server_socket(const std::string& path) {
  int sock = socket(AF_UNIX, SOCK_STREAM, 0);
  check(sock, "sock");
  sockaddr_un addr = make_addr_un(path.c_str());
  check(bind(sock, (struct sockaddr*)&addr, sizeof(sockaddr_un)), "bind");
  check(listen(sock, 1), "listen");
  return sock;
}

struct CMessage {
  struct msghdr msg = {0};
  char control[CMSG_SPACE(sizeof(int))];
  char iobuf[1];
  struct iovec iov;
  
  CMessage(int fd) {
    iov = iovec{ .iov_base = iobuf, .iov_len = sizeof(iobuf) };

    memset(control, 0, sizeof(control));
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = control;
    msg.msg_controllen = sizeof(control);

    struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type  = SCM_RIGHTS;
    cmsg->cmsg_len   = CMSG_LEN(sizeof(int));
    memcpy(CMSG_DATA(cmsg), &fd, sizeof(int));
  }
};

void set_fd_blocking(int fd, bool blocking) {
  if (fd < 0) return;
  int flags = fcntl(fd, F_GETFL, 0);
  check(flags, "fcntl get flags");
  flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
  check(fcntl(fd, F_SETFL, flags), "fcntl set flags");
}
}

template <typename M>
StatusOr<IPC<M>> IPC<M>::create(const std::string& dir, Token* token) {
  std::string path = get_likely_available_path(dir);
  *token = path;

  IPC ipc;
  ipc.am_server_ = true;
  ipc.connected_ = false;
  ipc.listen_socket_ = make_server_socket(path);
  return ipc;
}

template <typename M>
StatusOr<IPC<M>> IPC<M>::connect(const Token& token) {
  IPC ipc;
  ipc.am_server_ = false;
  ipc.connected_ = true;
  ipc.socket_ = socket(AF_UNIX, SOCK_STREAM, 0);
  check(ipc.socket_, "connect socket");
  sockaddr_un addr = make_addr_un(token.c_str());
  check(::connect(ipc.socket_, (sockaddr*)&addr, sizeof(sockaddr_un)), "connect");
  return ipc;
}

template <typename M>
StatusVal IPC<M>::send(const M& m) {
  if (!connected_) make_connection();
  set_blocking(true);

  check(::send(socket_, &m, sizeof(m), 0), "send");
  return OkStatus();
}

template <typename M>
StatusVal IPC<M>::send_fd(int fd) {
  if (!connected_) make_connection();
  set_blocking(true);

  CMessage msg = CMessage(fd);
  check(::sendmsg(socket_, &msg.msg, 0), "sendmsg");
  return OkStatus();
}

template <typename M>
StatusOr<M> IPC<M>::receive(bool block) {
  if (!connected_) make_connection();
  set_blocking(block);

  M m;
  int r = recv(socket_, &m, sizeof(m), 0);
  if (r == -1 && !block && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    return UnavailableError("receive would block");
  }
  check(r, "recv");
  return m;
}

template <typename M>
StatusOr<int> IPC<M>::receive_fd(bool block) {
  if (!connected_) make_connection();
  set_blocking(block);

  CMessage msg = CMessage(0);
  int r = recvmsg(socket_, &msg.msg, MSG_CMSG_CLOEXEC);
  if (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    return UnavailableError("receive_fd would block");
  }
  check(r, "recvmsg");
  for (struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg.msg); cmsg; cmsg = CMSG_NXTHDR(&msg.msg, cmsg)) {
    if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SCM_RIGHTS) {
      return *(int*)CMSG_DATA(cmsg);
    }
  }
  return InvalidArgumentError("Couldn't find a file descriptor in recvmsg ancillary data.");
}

template <typename M>
void IPC<M>::make_connection() {
  if (am_server_) {
    socket_ = accept(listen_socket_, nullptr, nullptr);
    check(socket_, "accept");
    connected_ = true;
  }
}

template <typename M>
void IPC<M>::set_blocking(bool blocking) {
  if (blocking_ == blocking) return;
  set_fd_blocking(socket_, blocking);
  blocking_ = blocking;
}
