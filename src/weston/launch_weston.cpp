#include "weston/launch_weston.h"

#include <fcntl.h>
#include <stdlib.h>

#include <fstream>
#include <sstream>
#include <thread>

#include "paths.h"
#include "process/process.h"
#include "time_aliases.h"

namespace {
void set_fd_nonblocking(int fd) {
  if (fd < 0) return;
  int flags = fcntl(fd, F_GETFL, 0);
  CHECK(flags != -1);
  flags = flags | O_NONBLOCK;
  CHECK(fcntl(fd, F_SETFL, flags) != -1);
}

bool read_fd(int fd, std::string* out) {
  char buf[1024];
  int r = read(fd, buf, 1023);
  if (r == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
    return true;
  }
  if (r == -1) {
    perror("run weston read");
    return false;
  }
  buf[r] = '\0';
  *out += std::string(buf);
  return true;
}

bool has_child(int pid) {
  const std::string path = std::format("/proc/{}/task/{}/children", pid, pid);
  std::ifstream file(path);
  if (!file) {
    return false;
  }

  std::stringstream r;
  r << file.rdbuf();
  return !r.str().empty();
}

StatusVal search_for_error(const std::string& out) {
  const std::string kCompositorFailed =
      "fatal: failed to create compositor backend";
  const std::string kWaylandPipeFailed =
      "Failed to process Wayland connection: Broken pipe";
  const std::string kDisplayPipeFailed =
      "failed to create display: Broken pipe";
  const std::string kSharedLibraryFailure1 =
      "error while loading shared libraries";
  const std::string kSharedLibraryFailure2 = "cannot open shared object file";

  if (out.find(kCompositorFailed) != std::string::npos) {
    printf("Port unavailable message: %s\n", out.c_str());
    return UnavailableError("Port already in use.");
  }
  if (out.find(kWaylandPipeFailed) != std::string::npos) {
    return UnknownError(std::format(
        "Weston launch failed to process the wayland connection because "
        "of a broken pipe.\nWeston log: {}",
        out));
  }
  if (out.find(kDisplayPipeFailed) != std::string::npos) {
    return UnknownError(
        "Weston launch failed to create display because of a broken pipe.");
  }
  if ((out.find(kSharedLibraryFailure1) != std::string::npos) ||
      (out.find(kSharedLibraryFailure2) != std::string::npos)) {
    printf("Shared library stdout: %s\n", out.c_str());
    return UnknownError("Couldn't find weston shared libraries.");
  }
  return OkStatus();
}
}  // namespace

StatusOr<Process> launch_weston(int port,
                                const std::vector<std::string>& command,
                                int width, int height) {
  std::vector<std::string> weston_command = {
      get_weston_bin(),
      "--xwayland",
      "--backend=vnc",
      "--disable-transport-layer-security",
      "--renderer=gl",
      std::format("--width={}", width),
      std::format("--height={}", height),
      std::format("--port={}", port),
      "--"};
  weston_command.insert(weston_command.end(), command.begin(), command.end());

  auto stream_conf = ProcessOutConf{
      .stdout = StreamOutConf::Pipe(),
      .stderr = StreamOutConf::StdoutPipe(),
  };
  ASSIGN_OR_RETURN(Process p, launch_process(weston_command, nullptr,
                                             std::move(stream_conf)));
  LOG(kLogVnc, "Launched weston as process: %d", p.pid);
  auto start = sc_now();
  std::string output;
  set_fd_nonblocking(p.stdout.fd());
  int hits = 0;
  int iter = 0;
  while (sc_now() - start < 5s) {
    read_fd(p.stdout.fd(), &output);
    RETURN_IF_ERROR(search_for_error(output));
    if (has_child(p.pid)) {
      hits++;
      if (hits > 20) {
        printf("Weston output: %s\n", output.c_str());
        return p;
      }
    }

    iter++;
    if (iter % 5 == 0) {
      printf("Waiting for weston launch into a known state.\n");
      std::chrono::duration<double> duration =
          std::chrono::duration_cast<std::chrono::duration<double>>(sc_now() -
                                                                    start);
      printf("Waited for %f\n", duration.count());
    }
    sleep_for(50ms);
  }
  return UnknownError(
      "run_weston() never found weston's child. Maybe the command exited "
      "without weston reporting a failure, weston is hanging, or the "
      "executed command ran as a daemon. run_weston() verifies that weston "
      "successfully launched a child in a poll loop and so to correctly handle "
      "quickly exiting daemons, consider running them under a child "
      "subreaper.\n\n"
      "Weston output:\n" +
      output);
}
