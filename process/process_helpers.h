#ifndef PROCESS_PROCESS_HELPERS_H_
#define PROCESS_PROCESS_HELPERS_H_

#include <spawn.h>

#include <vector>

#include "process/stream.h"
#include "third_party/status/status_or.h"

struct ProcessOutConf {
  StreamOutConf stdout = StreamOutConf::None();
  StreamOutConf stderr = StreamOutConf::None();
};

struct PrelaunchOut {
  posix_spawn_file_actions_t file_actions;
  StreamOut stdout;
  StreamOut stderr;
  std::vector<Fd> close_after_spawn;
};

StatusVal validate_process_out_conf(const ProcessOutConf& conf);

PrelaunchOut process_streams_prelaunch(ProcessOutConf&& out_conf);

#endif
