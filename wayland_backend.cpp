#include "wayland_backend.h"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <format>
#include <thread>

#include "display_vars.h"
#include "launch_weston.h"
#include "process.h"

namespace {
void terminate_processes(const std::vector<int>& pids) {
  for (auto pid : pids) {
    kill(pid, SIGTERM);
  }
  std::this_thread::sleep_for(std::chrono::seconds(3));
  for (auto pid : pids) {
    int r = waitpid(pid, nullptr, WNOHANG);
    if (r == 0) {
      kill(pid, SIGTERM);
    }
  }
};
}  // namespace

StatusOr<std::unique_ptr<WaylandBackend>> WaylandBackend::start_server(
    int32_t port_offset, int32_t width, int32_t height,
    const std::vector<std::string>& command) {
  int weston_pid;
  std::string instance_name;
  int port = port_offset;
  for (;; port++) {
    instance_name = std::format("vnc_{}", port);
    StatusOr<int> weston = run_weston(
        port, {"./build/export_display", instance_name}, width, height);
    if (!weston.ok() && weston.status().code() == StatusCode::UNAVAILABLE) {
      continue;
    }
    RETURN_IF_ERROR(weston);
    weston_pid = *weston;
    break;
  }

  DisplayVars dpy_vars;
  bool r = read_vars(instance_name, &dpy_vars);
  if (!r) return UnknownError("Failed to read display vars.");

  EnvVars env_vars = EnvVars::environ();
  env_vars.add_var("X_DISPLAY", dpy_vars.x_display.c_str());
  env_vars.add_var("WAYLAND_DISPLAY", dpy_vars.wayland_display.c_str());

  // TODO: Consider piping command's stdout and stderr to a file.
  ASSIGN_OR_RETURN(Process subproc, launch_process(command, &env_vars));

  return std::unique_ptr<WaylandBackend>(
      new WaylandBackend(port, weston_pid, subproc.pid));
}

WaylandBackend::~WaylandBackend() {
  terminate_processes({weston_pid_, subproc_pid_});
}
