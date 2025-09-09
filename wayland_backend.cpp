#include "wayland_backend.h"

#include <time.h>

#include <chrono>
#include <format>
#include <fstream>
#include <string>
#include <thread>
#include <utility>

#include "process_manager.h"
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
    return StatusVal(StatusCode::INTERNAL);
  }
  close(file);

  const std::string exit_file = std::format("/tmp/exit_vnc_{}", port);
  setenv("LD_LIBRARY_PATH", "/usr/local/lib", true);
  setenv("LOG_DISPLAYS_PATH", tempfile, true);
  setenv(kProcessManagerExitEnv, exit_file.c_str(), true);
  const std::string port_str = std::format("--port={}", port);
  const std::string width_str = std::format("--width={}", width);
  const std::string height_str = std::format("--height={}", height);
  const char* command_line[] = {"build/process_manager",
                                "weston",
                                "--xwayland",
                                "--backend=vnc",
                                "--disable-transport-layer-security",
                                port_str.c_str(),
                                width_str.c_str(),
                                height_str.c_str(),
                                "--",
                                command.c_str(),
                                nullptr};

  int pid;
  posix_spawnp(&pid, command_line[0], nullptr, nullptr, (char**)command_line,
               environ);

  auto cleanup = [exit_file]() {
    system(("touch " + exit_file).c_str());
    sleep(1);
  };

  return std::unique_ptr<WaylandBackend>(new WaylandBackend(cleanup));
}
