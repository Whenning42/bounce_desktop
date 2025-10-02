#ifndef PROCESS_ENV_VARS_H_
#define PROCESS_ENV_VARS_H_

#include <stdio.h>

#include <map>
#include <string>
#include <vector>

class EnvVars {
 public:
  // Copies the given env vars into a new EnvVars instance.
  EnvVars(char** env = nullptr);
  ~EnvVars() { clean_up_vars_arr(); }

  // Adds the given variable and value to env vars.
  void set_var(const std::string& var, const std::string& val);

  // Prepends the given value to the given variable.
  // If the variable isn't already in env_vars, it's initialized to an empty
  // string.
  void prepend_var(const std::string& var, const std::string& val);

  // Returns a copy of the process's environment.
  static EnvVars environ();

  // Returns the env vars as a char**.
  char** vars();

  std::string to_string();

 private:
  void clean_up_vars_arr();

  std::map<std::string, std::string> vars_;
  char** vars_arr_ = nullptr;
};

#endif
