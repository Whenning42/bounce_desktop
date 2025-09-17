#include <rfb/rfbclient.h>
#include <stdint.h>

#include <memory>
#include <thread>

#include "frame.h"
#include "third_party/status/status_or.h"

class BounceDeskClient {
 public:
  static StatusOr<std::unique_ptr<BounceDeskClient>> connect(int32_t port);
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
  BounceDeskClient(int port) : port_(port) {}
  void vnc_loop();

  int port_;
  rfbClient* client_;
  bool stop_vnc_ = false;
  std::thread vnc_loop_;
  Frame frame_;
};
