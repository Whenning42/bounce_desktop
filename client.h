#include <rfb/rfbclient.h>
#include <stdint.h>

#include <memory>
#include <thread>

#include "frame.h"
#include "third_party/status/status_or.h"

struct ConnectionOptions {
  // Required.
  int port = 0;
};

class BounceDeskClient {
 public:
  static StatusOr<std::unique_ptr<BounceDeskClient>> connect(
      int32_t port, ConnectionOptions options);
  ~BounceDeskClient();

  // Delete copy and move operators, since we rely on pointer stability when
  // invoking methods through user data passed to c-style callbacks.
  BounceDeskClient(const BounceDeskClient&) = delete;
  BounceDeskClient& operator=(const BounceDeskClient&) = delete;
  BounceDeskClient(BounceDeskClient&&) = delete;
  BounceDeskClient& operator=(BounceDeskClient&&) = delete;

  const Frame& get_frame();

  void resize(int w, int h);
  void update(int x, int y, int w, int h);

  // TODO: Input support
  // Mouse:
  //   Move, press, release
  //
  // Keyboard:
  //   Press key, Release key

 private:
  BounceDeskClient() = default;
  void vnc_loop();

  rfbClient* client_;
  bool stop_vnc_ = false;
  std::thread vnc_loop_;
  Frame frame_;
  ConnectionOptions connection_options_;
};
