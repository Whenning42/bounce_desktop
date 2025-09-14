// Calls write_vars(argv[1]) and then runs until its parent exits.
//
// This program enables us to launch processes under weston instances without
// having to pass those programs as weston's launch command, which gives us
// better access to those processes' exit codes, stdout/stderr, etc.
//
// Note: This binary is used by the tests in launch_weston_test, since it wants
// a simple binary that runs forever and sets PR_SET_PDEATHSIG both of which are
// done by this binary.

#include <linux/prctl.h>
#include <signal.h>
#include <sys/prctl.h>
#include <unistd.h>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <thread>

#include "display_vars.h"
#include "third_party/status/status_or.h"

int main(int argc, char* argv[]) {
  printf("Starting .\n");
  if (argc != 2) {
    std::cerr << "Usage: " << argv[0] << " <instance_name>" << std::endl;
    return 1;
  }

  CHECK(prctl(PR_SET_PDEATHSIG, SIGTERM) == 0);

  std::string instance_name = argv[1];
  write_vars(instance_name);

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(100));
  }
}
