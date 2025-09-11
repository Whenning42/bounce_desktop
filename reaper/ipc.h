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
  static StatusOr<IPC> create(const std::string& dir, Token* token);
  static StatusOr<IPC> connect(const Token& token);

  StatusVal send(const M& m);
  StatusVal send_fd(int fd);

  // receive returns UnavailableError if block is false and there's
  // nothiing to read.
  StatusOr<M> receive(bool block);
  StatusOr<int> receive_fd(bool block);

 private:
  void make_connection();
  void set_blocking(bool blocking);

  bool am_server_;
  bool connected_;
  int listen_socket_;
  int socket_;
  bool blocking_ = true;
};

#ifndef REAPER_IPC_TPP_
#include "reaper/ipc.tpp"
#endif

#endif

