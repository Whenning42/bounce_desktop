#include "display_vars.h"

#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>

namespace {
const char* kXDisplay = "DISPLAY";
const char* kWaylandDisplay = "WAYLAND_DISPLAY";
const char* kDefaultXDisplay = "";
const char* kDefaultWaylandDisplay = "";

std::string get_instance_dir(const std::string& instance_name) {
  uid_t uid = getuid();
  return std::format("/run/user/{}/instance_{}", uid, instance_name);
}
}  // namespace

void write_vars(const std::string& instance_name) {
  const std::string instance_dir = get_instance_dir(instance_name);
  const char* x_display_var = getenv(kXDisplay);
  const char* wayland_display_var = getenv(kWaylandDisplay);
  const char* x_display = x_display_var ? x_display_var : kDefaultXDisplay;
  const char* wayland_display =
      wayland_display_var ? wayland_display_var : kDefaultWaylandDisplay;

  std::filesystem::create_directories(instance_dir);

  const std::filesystem::path x_path =
      std::filesystem::path(instance_dir) / "x_display";
  const std::filesystem::path w_path =
      std::filesystem::path(instance_dir) / "wayland_display";

  std::ofstream write_x(x_path);
  write_x << x_display;
  std::ofstream write_wayland(w_path);
  write_wayland << wayland_display;
}

bool read_vars(const std::string& instance_name, DisplayVars* display_vars) {
  const std::filesystem::path instance_dir = get_instance_dir(instance_name);
  const std::filesystem::path x_path = instance_dir / "x_display";
  const std::filesystem::path w_path = instance_dir / "wayland_display";

  if (!std::filesystem::exists(x_path) || !std::filesystem::exists(w_path)) {
    return false;
  }

  std::ifstream x_infile(x_path);
  std::ifstream w_infile(w_path);
  if (!x_infile.is_open() || !w_infile.is_open()) {
    return false;
  }

  std::string x_val;
  std::string w_val;
  std::getline(x_infile, x_val);
  std::getline(w_infile, w_val);

  if (display_vars) {
    display_vars->x_display = std::move(x_val);
    display_vars->wayland_display = std::move(w_val);
  }
  return true;
}

void clean_up_vars(const std::string& instance_name) {
  std::error_code error_code;
  std::filesystem::remove_all(get_instance_dir(instance_name), error_code);
}
