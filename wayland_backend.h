#ifndef WAYLAND_BACKEND_H_
#define WAYLAND_BACKEND_H_

#include <memory>
#include <string>
#include <vector>

#include "backend.h"
#include "third_party/status/status_or.h"

class WaylandBackend : public Backend {
 public:
  static StatusOr<std::unique_ptr<WaylandBackend>> start_server(
      int32_t port, int32_t width, int32_t height,
      const std::vector<std::string>& command);

  ~WaylandBackend() override;

 private:
  WaylandBackend(int weston_pid, int subproc_pid)
      : weston_pid_(weston_pid), subproc_pid_(subproc_pid) {}

  int weston_pid_;
  int subproc_pid_;
};

#endif
