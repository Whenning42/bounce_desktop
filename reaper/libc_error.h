#ifndef REAPER_LIBC_ERROR_H_
#define REAPER_LIBC_ERROR_H_

#include <string.h>

#include <string>

inline std::string libc_error_name(int error_num) {
  std::string s = std::string(strerror(error_num));
  #ifdef __GLIBC__
  const char* enum_s = strerrorname_np(error_num);
  if (enum_s) {
    s += " (" + std::string(enum_s) + ")";
  }
  #endif
  return s;
}

#endif
