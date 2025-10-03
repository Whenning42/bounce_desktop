#ifndef DESKTOP_WESTON_BACKEND_H_
#define DESKTOP_WESTON_BACKEND_H_

#include <memory>
#include <string>
#include <vector>

#include "process/process.h"
#include "third_party/status/status_or.h"

class WestonBackend {
 public:
  static StatusOr<std::unique_ptr<WestonBackend>> start_server(
      int32_t port_offset, int32_t width, int32_t height,
      const std::vector<std::string>& command,
      ProcessOutConf&& command_out = ProcessOutConf());

  int port() { return port_; }

 private:
  WestonBackend(int port, Process&& weston, Process&& subproc)
      : port_(port), weston_(std::move(weston)), subproc_(std::move(subproc)) {}

  int port_;
  Process weston_;
  Process subproc_;
};

#endif  // DESKTOP_WESTON_BACKEND_H_
