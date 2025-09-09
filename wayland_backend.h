#ifndef WAYLAND_BACKEND_H_
#define WAYLAND_BACKEND_H_

#include <memory>
#include <string>

#include "backend.h"
#include "third_party/status/status_or.h"
#include "third_party/subprocess/subprocess.h"

class WaylandBackend : public Backend {
 public:
  // TODO: Improve process cleanup. Stopping the weston subprocess doesn't stop
  // its child log_displays.sh process which blocks attempts to start another
  // server on the same port.

  // Starts a Weston vnc backed server.
  static StatusOr<std::unique_ptr<WaylandBackend>> start_server(int32_t port,
                                                                int32_t width,
                                                                int32_t height);
  ~WaylandBackend() override { subprocess_terminate(&server_process_); }

  const std::string& get_x_display() { return x_display_; }
  const std::string& get_wayland_display() { return wayland_display_; };

 private:
  WaylandBackend(subprocess_s& server_process, const std::string& x_display,
                 const std::string& wayland_display)
      : server_process_(server_process),
        x_display_(x_display),
        wayland_display_(wayland_display) {}
  subprocess_s server_process_;
  std::string x_display_;
  std::string wayland_display_;
};

#endif
