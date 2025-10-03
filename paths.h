#ifndef PATHS_
#define PATHS_

#include <dlfcn.h>
#include <stdio.h>

#include <filesystem>
#include <string>

#include "time_aliases.h"

inline std::string get_package_path() {
  // To find our path to vendored weston, we find the path to our build/install
  // directory and then walk from there to the weston build/install directory.
  //
  // We find our build directory by dladdr'ing an arbitrary function, in this
  // case 'sc_now', and then walking from there.
  Dl_info dl_info;
  if (!dladdr((void*)&sc_now, &dl_info)) {
    return "";
  }

  std::filesystem::path dl_path(dl_info.dli_fname);
  std::string path_a = dl_path.parent_path().string();
  if (std::filesystem::exists(path_a + "/__init__.py")) {
    return path_a;
  }
  return dl_path.parent_path().string() + "/bounce_desktop";
}

inline std::string get_export_display_path() {
  return get_package_path() + "/bin/export_display";
}

inline std::string get_weston_bin() {
  return get_package_path() + "/_vendored/weston/bin/weston";
}

#endif
