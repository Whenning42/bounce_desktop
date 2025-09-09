#include "wayland_backend.h"

#include <time.h>

#include <chrono>
#include <format>
#include <fstream>
#include <string>
#include <thread>
#include <utility>

#include "third_party/status/status_or.h"
#include "third_party/subprocess/subprocess.h"

namespace {
bool read_display_file_with_timeout(const char* filepath, std::string* x_display,
                                     std::string* wayland_display,
                                     std::chrono::seconds timeout) {
  const auto start_time = std::chrono::steady_clock::now();
  while (std::chrono::steady_clock::now() - start_time < timeout) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    std::ifstream infile(filepath);
    if (!infile.is_open() || !(infile >> *x_display >> *wayland_display)) {
      continue;
    }
    if (x_display->empty() || wayland_display->empty()) {
      continue;
    }
    return true;
  }
  return false;
}
}  // namespace

StatusOr<std::unique_ptr<WaylandBackend>> WaylandBackend::start_server(
    int32_t port, int32_t width, int32_t height) {
  // TODO: Vendor weston auth changes.
  // TODO: Add subchild reaping.
  // TODO: Better manage child stderr/stdout.

  char tempfile[] = "/tmp/display_log_XXXXXX";
  int file = mkstemp(tempfile);
  if (file == -1) {
    perror("mkstemp");
    return StatusVal(StatusCode::INTERNAL);
  }
  close(file);

  setenv("LD_LIBRARY_PATH", "/usr/local/lib", true);
  setenv("LOG_DISPLAYS_PATH", tempfile, true);
  const std::string port_str = std::format("--port={}", port);
  const std::string width_str = std::format("--width={}", width);
  const std::string height_str = std::format("--height={}", height);
  const char* command_line[] = {"weston",
                                "--xwayland",
                                "--backend=vnc",
                                "--disable-transport-layer-security",
                                port_str.c_str(),
                                width_str.c_str(),
                                height_str.c_str(),
                                "--",
                                "./log_displays.sh",
                                nullptr};
  subprocess_s server_process;
  int result = subprocess_create(command_line,
                                 subprocess_option_inherit_environment |
                                     subprocess_option_enable_async |
                                     subprocess_option_search_user_path,
                                 &server_process);
  if (0 != result) {
    return StatusVal(StatusCode::INTERNAL);
  }

  std::string x_display;
  std::string wayland_display;
  if (!read_display_file_with_timeout(tempfile, &x_display, &wayland_display,
                                      std::chrono::seconds(3))) {
    return StatusVal(StatusCode::INTERNAL);
  }

  return std::unique_ptr<WaylandBackend>(
      new WaylandBackend(server_process, x_display, wayland_display));
}
