#ifndef REAPER_IPC_H_
#define REAPER_IPC_H_

#include <string>
#include <vector>
#include "third_party/status/status_or.h"


using Token = std::string;

// Simple bi-directional 1-to-1 fixed size message passing IPC.
template <typename M>
class IPC {
 public:
  // Allow moves, disallow default construction, copies, and assignment.
  IPC(IPC&& other) noexcept;
  IPC& operator=(IPC&& other) = delete;
  IPC& operator=(const IPC&) = delete;
  IPC(const IPC&) = delete;
  ~IPC();

  static StatusOr<IPC> create(const std::string& dir, Token* token);
  static StatusOr<IPC> connect(const Token& token);

  StatusVal send(const M& m);
  StatusVal send_fd(int fd);

  // Returns:
  // - UnavailableError if block is false and there's no data to receive.
  // - AbortedError if the other side of the socket's closed.
  StatusOr<M> receive(bool block = true);
  StatusOr<int> receive_fd(bool block = true);

  // Get the underlying socket file descriptor for polling
  int socket() const;

  // Clean up socket file from client side (when server has died)
  void cleanup_from_client();

  bool connected() const { return connected_; }

 private:
  IPC() = default;
  void make_connection();
  void set_blocking(bool blocking);

  bool am_server_;
  bool connected_;
  int listen_socket_ = -1;
  int socket_ = -1;
  bool blocking_ = true;
  std::string socket_path_;
};

#ifndef REAPER_IPC_TPP_
#include "reaper/ipc.tpp"
#endif

#endif

