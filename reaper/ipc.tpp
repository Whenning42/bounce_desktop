#define REAPER_IPC_TPP_

#include "reaper/ipc.h"

#include <fcntl.h>
#include <unistd.h>
#include <cassert>
#include <format>
#include <filesystem>
#include <random>
#include <sstream>
#include <sys/socket.h>
#include <sys/un.h>

#include <execinfo.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unistd.h>
#include <limits.h>
#include <stdint.h>

static const char* _st_exe_path() {
    static char path[PATH_MAX];
    static bool init = false;
    if (!init) {
        ssize_t n = readlink("/proc/self/exe", path, sizeof(path) - 1);
        if (n >= 0) path[n] = '\0';
        else std::snprintf(path, sizeof(path), "%s", "/proc/self/exe");
        init = true;
    }
    return path;
}

static std::string _st_addr2line(void* addr) {
    // -C (demangle), -f (function), -p (pretty), -e exe
    char cmd[1024];
    std::snprintf(cmd, sizeof(cmd), "addr2line -Cfpe %s %p", _st_exe_path(),
                  (void*)( (uintptr_t)addr - 1 )); // -1 gets into the call site
    FILE* fp = popen(cmd, "r");
    if (!fp) return {};
    char buf[1024];
    std::string out;
    while (std::fgets(buf, sizeof(buf), fp)) out += buf;
    pclose(fp);
    while (!out.empty() && (out.back() == '\n' || out.back() == '\r')) out.pop_back();
    return out;
}

inline void print_backtrace(int skip_frames = 1, bool with_file_lines = true,
                            int max_frames = 64, FILE* out = stderr) {
    if (max_frames > 256) max_frames = 256;
    void* buffer[256];
    int n = ::backtrace(buffer, max_frames);

    std::fprintf(out, "Stack trace (most recent call first):\n");
    for (int i = skip_frames; i < n; ++i) {
        void* addr = buffer[i];
        Dl_info info{};
        const char* sym = nullptr;
        const char* image = nullptr;
        uintptr_t offset = 0;

        if (::dladdr(addr, &info) && info.dli_sname) {
            int status = 0;
            char* dem = abi::__cxa_demangle(info.dli_sname, nullptr, nullptr, &status);
            sym = (status == 0 && dem) ? dem : info.dli_sname;
            image = info.dli_fname ? info.dli_fname : "?";
            offset = (uintptr_t)addr - (uintptr_t)info.dli_saddr;
            std::fprintf(out, "  #%02d %p %s + 0x%zx (%s)\n",
                         i - skip_frames, addr, sym, (size_t)offset, image);
            std::free(dem);
        } else {
            std::fprintf(out, "  #%02d %p (no symbol)\n", i - skip_frames, addr);
        }

        if (with_file_lines) {
            std::string loc = _st_addr2line(addr);
            if (!loc.empty()) std::fprintf(out, "        at %s\n", loc.c_str());
        }
    }
    std::fflush(out);
}

namespace {

void check(int v, const char* msg) {
  if (v == -1) {
    perror(msg);
#ifdef __GLIBC__
    const char* errname = strerrorname_np(errno);
    fprintf(stderr, "Error val: %s\n", errname);
#endif
    print_backtrace();
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
  int sock = ::socket(AF_UNIX, SOCK_STREAM, 0);
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
  ipc.socket_path_ = path;
  ipc.listen_socket_ = make_server_socket(path);
  return ipc;
}

template <typename M>
StatusOr<IPC<M>> IPC<M>::connect(const Token& token) {
  IPC ipc;
  ipc.am_server_ = false;
  ipc.connected_ = true;
  ipc.socket_path_ = token;
  ipc.socket_ = ::socket(AF_UNIX, SOCK_STREAM, 0);
  check(ipc.socket_, "connect socket");
  sockaddr_un addr = make_addr_un(token.c_str());
  check(::connect(ipc.socket_, (sockaddr*)&addr, sizeof(sockaddr_un)), "connect");
  return ipc;
}

template <typename M>
StatusVal IPC<M>::send(const M& m) {
  if (!connected_) make_connection();
  set_blocking(true);

  int r = ::send(socket_, &m, sizeof(m), MSG_NOSIGNAL);
  if (r == -1 && errno == EPIPE) {
    return AbortedError("send's peer has hung up.");
  }
  if (r == -1) {
    return InternalError("Unhandled send error.");
  }
  return OkStatus();
}

template <typename M>
StatusVal IPC<M>::send_fd(int fd) {
  if (!connected_) make_connection();
  set_blocking(true);

  CMessage msg = CMessage(fd);
  int r = ::sendmsg(socket_, &msg.msg, MSG_NOSIGNAL);
  if (r == -1 && errno == EPIPE) {
    return AbortedError("send_fd's peer has hung up.");
  }
  if (r == -1) {
    return InternalError("Unhandled sendmsg error.");
  }
  return OkStatus();
}

template <typename M>
StatusOr<M> IPC<M>::receive(bool block) {
  if (!connected_) make_connection();
  set_blocking(block);

  M m;
  int r = recv(socket_, &m, sizeof(m), 0);
  if (r == -1 && !block && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    return UnavailableError("Receive would block.");
  }
  if (r == 0 || (r == -1 && errno == ECONNRESET)) {
    return AbortedError("receive's peer has hung up.");
  }
  check(r, "recv");

  if (r < (int)sizeof(m)) {
    size_t expected = sizeof(m);
    int actual = r;
    return InternalError(std::format("Unexpected read size. Expected: {}, actual {}", expected, actual));
  }

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
  if (r == 0 || (r == -1 && errno == ECONNRESET)) {
    return AbortedError("receive_fd's peer has hung up.");
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

template <typename M>
int IPC<M>::socket() const {
  return socket_;
}

template <typename M>
void IPC<M>::cleanup_from_client() {
  assert(!am_server_);
  if (!socket_path_.empty()) {
    unlink(socket_path_.c_str());
  }
}

template <typename M>
IPC<M>::IPC(IPC&& other) noexcept
  : am_server_(other.am_server_),
    connected_(other.connected_),
    listen_socket_(other.listen_socket_),
    socket_(other.socket_),
    blocking_(other.blocking_),
    socket_path_(std::move(other.socket_path_)) {
  other.listen_socket_ = -1;
  other.socket_ = -1;
  other.socket_path_.clear();
}

template <typename M>
IPC<M>& IPC<M>::operator=(IPC&& other) noexcept {
  if (this != &other) {
    // Clean up current resources
    if (listen_socket_ != -1) close(listen_socket_);
    if (socket_ != -1) close(socket_);
    if (am_server_ && !socket_path_.empty()) {
      unlink(socket_path_.c_str());
    }
    
    // Move from other
    am_server_ = other.am_server_;
    connected_ = other.connected_;
    listen_socket_ = other.listen_socket_;
    socket_ = other.socket_;
    blocking_ = other.blocking_;
    socket_path_ = std::move(other.socket_path_);
    
    // Reset other
    other.listen_socket_ = -1;
    other.socket_ = -1;
    other.socket_path_.clear();
  }
  return *this;
}

template <typename M>
IPC<M>::~IPC() {
  if (listen_socket_ != -1) close(listen_socket_);
  if (socket_ != -1) {
    close(socket_);
  }
  
  if (am_server_ && !socket_path_.empty()) {
    unlink(socket_path_.c_str());
  }
}
