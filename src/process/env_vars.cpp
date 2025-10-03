#include "process/env_vars.h"

#include <assert.h>
#include <string.h>
#include <unistd.h>

#include <string>
#include <tuple>

namespace {
std::tuple<std::string, std::string> split_var(const std::string& str) {
  std::string var;
  std::string val;

  std::string::size_type pos = str.find('=');
  assert(pos != std::string::npos);
  var = str.substr(0, pos);
  val = str.substr(pos + 1, std::string::npos);

  return {var, val};
}
}  // namespace

EnvVars::EnvVars(char** env) {
  size_t i = 0;
  while (env && env[i]) {
    auto [var, val] = split_var(env[i]);
    vars_.insert({var, val});
    i++;
  }
}

void EnvVars::set_var(const std::string& var, const std::string& val) {
  vars_[var] = val;
}

void EnvVars::prepend_var(const std::string& var, const std::string& val) {
  vars_[var] = val + vars_[var];
}

EnvVars EnvVars::environ() { return EnvVars(::environ); }

char** EnvVars::vars() {
  clean_up_vars_arr();

  vars_arr_ = (char**)malloc(sizeof(char*) * (vars_.size() + 1));
  int i = 0;
  for (const auto& [k, v] : vars_) {
    vars_arr_[i] = strdup((k + "=" + v).c_str());
    i++;
  }
  vars_arr_[i] = nullptr;
  return vars_arr_;
}

std::string EnvVars::to_string() {
  std::string out;
  for (const auto& [k, v] : vars_) {
    out += k + "=" + "v";
    out += "\n";
  }
  return out;
}

void EnvVars::clean_up_vars_arr() {
  int i = 0;
  while (vars_arr_ && vars_arr_[i]) {
    free(vars_arr_[i]);
    i++;
  }
  free(vars_arr_);
  vars_arr_ = nullptr;
}
