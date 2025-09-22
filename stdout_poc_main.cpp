#include <errno.h>
#include <fcntl.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include <string>
#include <vector>

#include "process.h"

void check(int v, const char* loc) {
  if (v != 0) {
    perror(loc);
    exit(1);
  }
}

void test_0(int argc, char** argv) {
  int devnull = open("/dev/null", O_RDWR);
  posix_spawn_file_actions_t actions;
  check(posix_spawn_file_actions_init(&actions), "init");
  check(posix_spawn_file_actions_adddup2(&actions, devnull, STDOUT_FILENO),
        "add dup");
  check(posix_spawn_file_actions_addclose(&actions, devnull), "add close");

  int pid;
  char* empty_argv[1] = {nullptr};
  check(posix_spawnp(&pid, argv[1], &actions, nullptr,
                     argc >= 2 ? &argv[2] : empty_argv, environ),
        "spawn");
}

void test_1(int argc, char** argv) {
  EnvVars env = EnvVars::environ();
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.push_back(std::string(argv[i]));
  }
  ProcessOutConf conf = ProcessOutConf{.stdout = StreamOutConf::DevNull(),
                                       .stderr = StreamOutConf::DevNull()};
  auto p = launch_process(args, &env, std::move(conf));

  while (true) {
    sleep(1);
  }
}

int main(int argc, char** argv) {
  // test_0(argc, argv);
  test_1(argc, argv);
  return 0;
}
