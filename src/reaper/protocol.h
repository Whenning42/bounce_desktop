#ifndef REAPER_PROTOCOL_H_
#define REAPER_PROTOCOL_H_

// The reaper launcher passes its IPC connection token to the reaper
// through this env var.
inline const char* const kReaperIpcFileEnvVar = "REAPER_IPC_FILE";

enum class ReaperMessageCode {
  // Requests
  CLEAN_UP = 0,

  // Responses
  FINISHED_LAUNCH = 1,
  INVALID_COMMAND = 2,
  OTHER_FAILED_LAUNCH = 3,
  FINISHED_CLEANING_UP = 4,
};

struct ReaperMessage {
  ReaperMessageCode code;
};

#endif  // REAPER_PROTOCOL_H_
