#ifndef PROCESS_ENV_VARS_H_
#define PROCESS_ENV_VARS_H_

#include <stdio.h>

#include <vector>

inline void print_vars(char** vars) {
  for (int i = 0;; ++i) {
    char* v = vars[i];
    if (!v) return;
    printf("VAR: %s\n", v);
  }
}

class EnvVars {
 public:
  // Copies the given env vars into a new EnvVars instance.
  EnvVars(char** env = nullptr);
  ~EnvVars();

  // Adds the given variable and value to env vars.
  void set_var(const char* var, const char* val);

  // Returns a copy of the process's environment.
  static EnvVars environ();

  // Returns the env vars as a char**.
  char** vars();

 public:
  std::vector<char*> vars_;
};

#endif
