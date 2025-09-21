#include "process/env_vars.h"

#include <string.h>
#include <unistd.h>

#include <string>

namespace {
std::string get_varname(const char* var_val) {
  std::string s = std::string(var_val);
  std::string::size_type pos = s.find('=');
  if (pos != std::string::npos) {
    s = s.substr(0, pos);
  }
  return s;
}
}  // namespace

EnvVars::EnvVars(char** env) {
  size_t i = 0;
  while (true) {
    if (!env[i]) break;
    vars_.push_back(strdup(env[i]));
    i++;
  }
  vars_.push_back(nullptr);
}

EnvVars::~EnvVars() {
  for (size_t i = 0; i < vars_.size(); ++i) {
    free(vars_[i]);
    vars_[i] = nullptr;
  }
}

void EnvVars::set_var(const char* var, const char* val) {
  size_t i = 0;
  for (; i < vars_.size(); ++i) {
    if (!vars_[i]) break;
    if (get_varname(vars_[i]) == std::string(var)) break;
  }

  std::string c_val = std::string(var) + "=" + std::string(val);
  vars_[i] = strdup(c_val.c_str());
  if (i == vars_.size() - 1) vars_.push_back(nullptr);
}

EnvVars EnvVars::environ() { return EnvVars(::environ); }

char** EnvVars::vars() { return &vars_[0]; }
