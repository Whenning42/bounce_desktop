#include "third_party/status/status_or.h"

#include <vector>
#include <string>

#include <string.h>
#include <spawn.h>

inline StatusOr<int> launch_process(const std::vector<std::string>& args) {
  char** argv = static_cast<char**>(malloc(sizeof(char*) * (args.size() + 1)));
  for (size_t i = 0; i < args.size(); ++i) {
    argv[i] = strdup(args[i].c_str());
  }
  argv[args.size()] = nullptr;

  int pid;
  int r = posix_spawnp(&pid, argv[0], nullptr, nullptr, argv, environ);
  if (r != 0) {
    return InvalidArgumentError();
  }

  for (size_t i = 0; i < args.size(); ++i) {
    free(argv[i]);
  }
  return pid;
}
