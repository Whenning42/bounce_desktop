#include <memory>

#include "backend.h"
#include "third_party/status/status_or.h"
#include "third_party/subprocess/subprocess.h"

class WaylandBackend : public Backend {
 public:
  // Starts a Weston vnc backed server.
  static StatusOr<std::unique_ptr<WaylandBackend>> start_server(int32_t port);
  ~WaylandBackend() override { subprocess_destroy(&server_process_); }

 private:
  WaylandBackend(subprocess_s& server_process)
      : server_process_(server_process) {}
  subprocess_s server_process_;
};
