#include "process.h"

#include <unistd.h>

namespace {

struct Fds {
  int r_out;
  int w_out;
  int r_err;
  int w_err;

  void close_all() {
    if (r_out != -1) close(r_out);
    if (r_err != -1) close(r_err);
    if (w_out != -1) close(w_out);
    if (w_err != -1) close(w_err);
  }
};

StatusVal validate_process_out(ProcessOut stdout, ProcessOut stderr) {
  if (stdout == ProcessOut::STDOUT) {
    return InvalidArgumentError(
        "Process out stdout can't be ProcessOut::STDOUT. Use ProcessOut::PIPE "
        "for pipeing stdout to a pipe.");
  }

  if (stderr == ProcessOut::STDOUT && stdout != ProcessOut::PIPE) {
    return InvalidArgumentError(
        "Process out stderr can only be ProcessOut::STDOUT if stdout was set "
        "to ProcessOut::PIPE.");
  }
  return OkStatus();
}

std::tuple<Fds, posix_spawn_file_actions_t> subproc_fds_prelaunch(
    ProcessOut stdout, ProcessOut stderr) {
  bool pipe_stdout = stdout == ProcessOut::PIPE;

  int r_out = -1;
  int w_out = -1;
  int r_err = -1;
  int w_err = -1;

  if (pipe_stdout) {
    int p_out[2];
    CHECK(pipe(p_out) != -1);
    r_out = p_out[0];
    w_out = p_out[1];
  }

  if (stderr == ProcessOut::PIPE) {
    int p_err[2];
    CHECK(pipe(p_err) != -1);
    r_err = p_err[0];
    w_err = p_err[1];
  }

  posix_spawn_file_actions_t actions;
  CHECK(posix_spawn_file_actions_init(&actions) == 0);
  if (stdout == ProcessOut::PIPE) {
    CHECK(posix_spawn_file_actions_adddup2(&actions, w_out, STDOUT_FILENO) ==
          0);
  }
  if (stderr == ProcessOut::PIPE) {
    CHECK(posix_spawn_file_actions_adddup2(&actions, w_err, STDERR_FILENO) ==
          0);
  }
  if (stderr == ProcessOut::STDOUT) {
    CHECK(posix_spawn_file_actions_adddup2(&actions, w_out, STDERR_FILENO) ==
          0);
  }

  if (stdout == ProcessOut::PIPE) {
    CHECK(posix_spawn_file_actions_addclose(&actions, r_out) == 0);
    CHECK(posix_spawn_file_actions_addclose(&actions, w_out) == 0);
  }
  if (stderr == ProcessOut::PIPE) {
    CHECK(posix_spawn_file_actions_addclose(&actions, r_err) == 0);
    CHECK(posix_spawn_file_actions_addclose(&actions, w_err) == 0);
  }

  return {Fds{.r_out = r_out, .w_out = w_out, .r_err = r_err, .w_err = w_err},
          actions};
}

void subproc_fds_postlaunch(const Fds& fds, Process* p) {
  if (fds.r_out != -1) p->stdout_pipe = fds.r_out;
  if (fds.r_err != -1) p->stderr_pipe = fds.r_err;
  if (fds.w_out != -1) close(fds.w_out);
  if (fds.w_err != -1) close(fds.w_err);
}

}  // namespace

// Process special members
Process::Process() : pid(-1), stdout_pipe(-1), stderr_pipe(-1) {}

Process::~Process() {
  if (stdout_pipe != -1) close(stdout_pipe);
  if (stderr_pipe != -1) close(stderr_pipe);
}

Process::Process(Process&& other) noexcept {
  pid = other.pid;
  stdout_pipe = other.stdout_pipe;
  stderr_pipe = other.stderr_pipe;

  other.pid = -1;
  other.stdout_pipe = -1;
  other.stderr_pipe = -1;
}

Process& Process::operator=(Process&& other) noexcept {
  if (this != &other) {
    if (stdout_pipe != -1) close(stdout_pipe);
    if (stderr_pipe != -1) close(stderr_pipe);

    pid = other.pid;
    stdout_pipe = other.stdout_pipe;
    stderr_pipe = other.stderr_pipe;

    other.pid = -1;
    other.stdout_pipe = -1;
    other.stderr_pipe = -1;
  }
  return *this;
}

EnvVars::EnvVars(char** env) {
  if (!env) return;
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

StatusOr<Process> launch_process(const std::vector<std::string>& args,
                                 EnvVars* env_vars, ProcessOut stdout,
                                 ProcessOut stderr) {
  RETURN_IF_ERROR(validate_process_out(stdout, stderr));

  char** argv = static_cast<char**>(malloc(sizeof(char*) * (args.size() + 1)));
  for (size_t i = 0; i < args.size(); ++i) {
    argv[i] = strdup(args[i].c_str());
  }
  argv[args.size()] = nullptr;

  int pid;
  char** env = env_vars ? env_vars->vars() : environ;

  auto [fds, file_actions] = subproc_fds_prelaunch(stdout, stderr);
  int r = posix_spawnp(&pid, argv[0], &file_actions, nullptr, argv, env);
  if (r != 0) {
    fds.close_all();
    return InvalidArgumentError("Failed to launch process: " +
                                libc_error_name(r));
  }
  Process p;
  subproc_fds_postlaunch(fds, &p);
  p.pid = pid;

  for (size_t i = 0; i < args.size(); ++i) {
    free(argv[i]);
  }
  return p;
}
