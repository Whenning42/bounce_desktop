#ifndef WAYLAND_BACKEND_H_
#define WAYLAND_BACKEND_H_

#include <functional>
#include <memory>
#include <string>

#include "backend.h"
#include "reaper/reaper.h"
#include "third_party/status/status_or.h"
#include "third_party/subprocess/subprocess.h"

class WaylandBackend : public Backend {
 public:
  // TODO: Improve process cleanup. Stopping the weston subprocess doesn't stop
  // its child log_displays.sh process which blocks attempts to start another
  // server on the same port.

  // Starts a Weston vnc backed server.
  static StatusOr<std::unique_ptr<WaylandBackend>> start_server(
      int32_t port, int32_t width, int32_t height, const std::string& command);
  ~WaylandBackend() override {
    printf("Requesting cleanup\n");
    if (reaper_) {
      reaper_->clean_up();
    }
  }

 private:
  WaylandBackend(std::shared_ptr<reaper::Reaper> reaper) : reaper_(reaper) {}

  std::shared_ptr<reaper::Reaper> reaper_;
};

#endif
