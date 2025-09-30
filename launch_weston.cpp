#include "launch_weston.h"

#include <fcntl.h>

#include <fstream>
#include <sstream>
#include <thread>

#include "process.h"
#include "project_root.h"

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

  if (out.find(kCompositorFailed) != std::string::npos) {
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
  return OkStatus();
}
}  // namespace

StatusOr<Process> run_weston(int port, const std::vector<std::string>& command,
                             int width, int height) {
  EnvVars env = EnvVars::environ();
  std::string weston_exe_path =
      project_root() + "/build/subprojects/weston-fork/frontend/weston";

  printf("Launching file: %s\n", weston_exe_path.c_str());
  std::vector<std::string> weston_command = {
      weston_exe_path,
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
  ASSIGN_OR_RETURN(
      Process p, launch_process(weston_command, &env, std::move(stream_conf)));
  LOG(kLogVnc, "Launched weston as process: %d", p.pid);
  auto start = std::chrono::steady_clock::now();
  auto timeout = std::chrono::seconds(5000);
  std::string output;
  set_fd_nonblocking(p.stdout.fd());
  int hits = 0;
  while (std::chrono::steady_clock::now() - start < timeout) {
    read_fd(p.stdout.fd(), &output);
    RETURN_IF_ERROR(search_for_error(output));
    if (has_child(p.pid)) {
      hits++;
      if (hits > 20) {
        printf("Weston output: %s\n", output.c_str());
        return p;
      }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  }
  return UnknownError(
      "run_weston() never found weston's child. Maybe the command exited "
      "without weston reporting a failure, weston is hanging, or the "
      "executed command ran as a daemon. run_weston() verifies that weston "
      "successfully launched a child in a poll loop and so to correctly handle "
      "quickly exiting daemons, consider running them under a child "
      "subreaper.");
}
