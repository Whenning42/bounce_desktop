#include "wayland_backend.h"

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <filesystem>
#include <format>
#include <thread>

#include "bdisplay_manager.h"
#include "third_party/status/logger.h"
#include "third_party/status/status_or.h"
#include "reaper/process.h"
#include "reaper/libc_error.h"

namespace {

StatusOr<int> launch_command_with_display(const std::string& command,
                                          const InstanceInfo& info) {
  // Set environment variables
  EnvVars env = EnvVars::environ();
  if (!info.x_display.empty()) {
    env.add_var("DISPLAY", info.x_display.c_str());
  }
  if (!info.wayland_display.empty()) {
    env.add_var("WAYLAND_DISPLAY", info.wayland_display.c_str());
  }

  std::vector<std::string> args = {"sh", "-c", command};
  return launch_process(args, &env);
}

}  // namespace

StatusOr<std::unique_ptr<WaylandBackend>> WaylandBackend::start_server(
    int32_t port, int32_t width, int32_t height, const std::string& command) {

  // Generate a unique instance name using port and timestamp
  auto now = std::chrono::system_clock::now().time_since_epoch();
  auto timestamp =
      std::chrono::duration_cast<std::chrono::milliseconds>(now).count();
  std::string instance_name = std::format("vnc_{}_{}", port, timestamp);

  const std::string port_str = std::format("--port={}", port);
  const std::string width_str = std::format("--width={}", width);
  const std::string height_str = std::format("--height={}", height);

  std::string bdisplay_manager_path;
  if (std::filesystem::exists("build/bdisplay_manager_main")) {
    bdisplay_manager_path =
        std::filesystem::absolute("build/bdisplay_manager_main").string();
  } else {
    // Fall back to looking in system path
    bdisplay_manager_path = "bdisplay_manager_main";
  }

  const std::string weston_command = std::format(
      "weston --xwayland --backend=vnc --disable-transport-layer-security {} "
      "{} {} -- {} {}",
      port_str, width_str, height_str, bdisplay_manager_path, instance_name);

  // Launch weston with bdisplay_manager
  LOG(kLogSubprocess, "Starting weston with command: %s",
      weston_command.c_str());

  // Set environment variables
  EnvVars env = EnvVars::environ();
  env.add_var("LD_LIBRARY_PATH", "/usr/local/lib");

  std::vector<std::string> weston_args = {
      "/usr/local/bin/weston",
      "--xwayland",
      "--backend=vnc",
      "--disable-transport-layer-security",
      port_str,
      width_str,
      height_str,
      "--",
      bdisplay_manager_path,
      instance_name
  };

  ASSIGN_OR_RETURN(int weston_pid, launch_process(weston_args, &env));
  LOG(kLogSubprocess, "Weston started with PID: %d", weston_pid);

  // Wait for display manager to write info with timeout
  const int timeout_seconds = 5;
  const int check_interval_ms = 500;
  const int max_attempts = (timeout_seconds * 1000) / check_interval_ms;

  InstanceInfo info;
  bool info_available = false;

  LOG(kLogSubprocess, "Waiting for display manager info for instance: %s",
      instance_name.c_str());

  for (int attempt = 0; attempt < max_attempts; ++attempt) {
    if (read_info(instance_name, &info)) {
      LOG(kLogSubprocess, "Display manager info available for instance: %s",
          instance_name.c_str());
      info_available = true;
      break;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(check_interval_ms));
  }

  if (!info_available) {
    LOG(kLogError, "Display manager failed to start for instance: %s",
        instance_name.c_str());
    // Kill weston process
    kill(weston_pid, SIGTERM);
    return InternalError();
  }

  // Launch the command with display environment variables set
  ASSIGN_OR_RETURN(int command_pid, launch_command_with_display(command, info));

  return std::unique_ptr<WaylandBackend>(
      new WaylandBackend(instance_name, command_pid));
}

WaylandBackend::~WaylandBackend() {
  // Close display manager
  close_display_manager(instance_name_);

  // Send SIGTERM to the command process
  if (pid_ > 0) {
    kill(pid_, SIGTERM);

    // Wait for process with 2 second timeout
    const int timeout_seconds = 2;
    const int check_interval_ms = 100;
    const int max_attempts = (timeout_seconds * 1000) / check_interval_ms;

    bool process_exited = false;
    for (int attempt = 0; attempt < max_attempts; ++attempt) {
      int status;
      pid_t result = waitpid(pid_, &status, WNOHANG);
      if (result == pid_ || result == -1) {
        process_exited = true;
        break;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(check_interval_ms));
    }

    // If process didn't exit, send SIGKILL
    if (!process_exited) {
      kill(pid_, SIGKILL);
      waitpid(pid_, nullptr, 0);
    }
  }
}
