#ifndef WAYLAND_BACKEND_H_
#define WAYLAND_BACKEND_H_

#include <memory>
#include <string>
#include <vector>

#include "process.h"
#include "third_party/status/status_or.h"

class WaylandBackend {
 public:
  static StatusOr<std::unique_ptr<WaylandBackend>> start_server(
      int32_t port_offset, int32_t width, int32_t height,
      const std::vector<std::string>& command,
      ProcessOutConf&& command_out = ProcessOutConf());

  int port() { return port_; }

 private:
  WaylandBackend(int port, Process&& weston, Process&& subproc)
      : port_(port), weston_(std::move(weston)), subproc_(std::move(subproc)) {}

  int port_;
  Process weston_;
  Process subproc_;
};

#endif
