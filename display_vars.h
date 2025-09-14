#ifndef DISPLAY_VARS_H
#define DISPLAY_VARS_H

#include <string>

struct DisplayVars {
  std::string x_display;
  std::string wayland_display;
};

// Writes the values of environment variables X_DISPLAY and WAYLAND_DISPLAY
// to /run/user/UID/instance_{instance_name}/x_display
// and /run/user/UID/instance_{instance_name}/wayland_display
void write_vars(const std::string& instance_name);

// Read the display vars from /run/user/UID/instance_{instance_name}/...
// and return them in 'display_vars' if it's not null. Returns false
// and makes no changes to DisiplayVars if any of the expected display
// var files are missing.
bool read_vars(const std::string& instance_name, DisplayVars* display_vars);

// Deletes the display vars stored on disk for the given instance_name.
void clean_up_vars(const std::string& instance_name);

#endif
