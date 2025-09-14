#ifndef WAYLAND_BACKEND_H_
#define WAYLAND_BACKEND_H_

#include <functional>
#include <memory>
#include <string>

#include "backend.h"
#include "third_party/status/status_or.h"

class WaylandBackend : public Backend {
 public:
  static StatusOr<std::unique_ptr<WaylandBackend>> start_server(
      int32_t port, int32_t width, int32_t height, const std::string& command);

  ~WaylandBackend() override;

 private:
  WaylandBackend(const std::string& instance_name, int pid)
      : instance_name_(instance_name), pid_(pid) {}

  std::string instance_name_;
  int pid_;
};

#endif
