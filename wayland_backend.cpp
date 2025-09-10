#include "wayland_backend.h"

#include <time.h>

#include <chrono>
#include <format>
#include <fstream>
#include <string>
#include <thread>
#include <utility>

#include "reaper/ipc.h"
#include "reaper/reaper.h"
#include "third_party/status/status_or.h"
#include "third_party/subprocess/subprocess.h"

StatusOr<std::unique_ptr<WaylandBackend>> WaylandBackend::start_server(
    int32_t port, int32_t width, int32_t height, const std::string& command) {
  // TODO: Vendor weston auth changes.
  // TODO: Better manage child stderr/stdout.

  char tempfile[] = "/tmp/display_log_XXXXXX";
  int file = mkstemp(tempfile);
  if (file == -1) {
    perror("mkstemp");
    return InternalError();
  }
  close(file);

  const std::string ipc_file = std::format("/tmp/reaper_vnc_{}", port);
  setenv("LD_LIBRARY_PATH", "/usr/local/lib", true);
  setenv("LOG_DISPLAYS_PATH", tempfile, true);

  const std::string port_str = std::format("--port={}", port);
  const std::string width_str = std::format("--width={}", width);
  const std::string height_str = std::format("--height={}", height);

  const std::string weston_command = std::format(
      "weston --xwayland --backend=vnc --disable-transport-layer-security {} "
      "{} {} -- {}",
      port_str, width_str, height_str, command);

  RETURN_IF_ERROR(reaper::launch_reaper(weston_command, ipc_file));
  auto cleanup = [ipc_file]() {
    reaper::clean_up(ipc_file, std::chrono::seconds(5));
  };

  return std::unique_ptr<WaylandBackend>(new WaylandBackend(cleanup));
}
