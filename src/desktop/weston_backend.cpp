#include "desktop/weston_backend.h"

#include <unistd.h>

#include <chrono>
#include <format>
#include <thread>

#include "weston/display_vars.h"
#include "weston/launch_weston.h"
#include "paths.h"

StatusOr<std::unique_ptr<WestonBackend>> WestonBackend::start_server(
    int32_t port_offset, int32_t width, int32_t height,
    const std::vector<std::string>& command, ProcessOutConf&& command_out) {
  Process weston;
  std::string instance_name;
  int port = port_offset;
  for (;; port++) {
    instance_name = std::format("vnc_{}", port);
    StatusOr<Process> weston_or = launch_weston(
        port, {get_export_display_path(), instance_name}, width, height);
    if (!weston_or.ok() &&
        weston_or.status().code() == StatusCode::UNAVAILABLE) {
      continue;
    }
    RETURN_IF_ERROR(weston_or);
    weston = std::move(weston_or.value());
    break;
  }
  LOG(kLogVnc, "Weston started on port: %d", port);

  DisplayVars dpy_vars;
  bool r = read_vars(instance_name, &dpy_vars);
  if (!r) return UnknownError("Failed to read display vars.");

  EnvVars env_vars = EnvVars::environ();
  env_vars.set_var("DISPLAY", dpy_vars.x_display.c_str());
  env_vars.set_var("WAYLAND_DISPLAY", dpy_vars.wayland_display.c_str());
  printf(
      "===================== Running on DISPLAY: %s, WAYLAND_DISPLAY: %s "
      "==============\n",
      dpy_vars.x_display.c_str(), dpy_vars.wayland_display.c_str());

  ASSIGN_OR_RETURN(Process subproc,
                   launch_process(command, &env_vars, std::move(command_out)));

  return std::unique_ptr<WestonBackend>(
      new WestonBackend(port, std::move(weston), std::move(subproc)));
}
