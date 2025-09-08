#include "wayland_backend.h"

#include <time.h>

#include <format>
#include <string>
#include <utility>

#include "third_party/status/status_or.h"
#include "third_party/subprocess/subprocess.h"

StatusOr<std::unique_ptr<WaylandBackend>> WaylandBackend::start_server(
    int32_t port) {
  // TODO: Add subchild reaping.
  // TODO: Better manage child stderr/stdout.
  setenv("LD_LIBRARY_PATH", "/usr/local/lib", true);
  const std::string port_str = std::format("--port={}", port);
  const char* command_line[] = {
      "weston",         "--xwayland",
      "--backend=vnc",  "--disable-transport-layer-security",
      port_str.c_str(), nullptr};
  subprocess_s server_process;
  int result = subprocess_create(command_line,
                                 subprocess_option_inherit_environment |
                                     subprocess_option_enable_async |
                                     subprocess_option_search_user_path,
                                 &server_process);
  if (0 != result) {
    return StatusVal(StatusCode::INTERNAL);
  }
  sleep(1);

  return std::unique_ptr<WaylandBackend>(new WaylandBackend(server_process));
}
