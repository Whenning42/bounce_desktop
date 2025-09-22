#ifndef WESTON_H_
#define WESTON_H_

#include <string>
#include <vector>

#include "process.h"
#include "third_party/status/status_or.h"

// Try running a Weston VNC backend display that runs the given
// command and uses the given port. We parse Weson's stdout to
// try to determine what state Weston ends up in and return any
// errors as statuses.
//
// Note: Weston doesn't reap the child command on exit, and weston's vnc backend
// leaks the port to the child command, so if you want to get the port back when
// exiting weston, run a command that exits when its parent does.
//
// Returns:
//  - UNAVAILABLE_ERROR if the chosen port is taken.
//  - UNKNOWN_ERROR if weston fails with any non-port related error.
StatusOr<Process> run_weston(int port, const std::vector<std::string>& command,
                             int width = 800, int height = 600);

#endif
