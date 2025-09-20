#include "process/env_vars.h"

#include <string.h>
#include <unistd.h>

#include <string>

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

void EnvVars::add_var(const char* var, const char* val) {
  std::string c_val = std::string(var) + "=" + std::string(val);
  vars_.back() = strdup(c_val.c_str());
  vars_.push_back(nullptr);
}

EnvVars EnvVars::environ() { return EnvVars(::environ); }

char** EnvVars::vars() { return &vars_[0]; }
