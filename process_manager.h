// A process reaper class. See process_manager_main.cpp for public interface.
//
// TODO: Correctly handle signals and EINTR errors.

#include <string>

inline const char* kProcessManagerExitEnv = "PROCESS_MANAGER_EXIT_FILE";

class ProcessManager {
 public:
  ProcessManager(char** argv, const std::string& exit_file);
  void run();
  void exit();

 private:
  std::string exit_file_;
};
