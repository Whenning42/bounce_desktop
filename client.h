#ifndef CLIENT_H_
#define CLIENT_H_

#include <rfb/rfbclient.h>
#include <stdint.h>

#include <memory>
#include <mutex>
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

  Frame get_frame();

  // Key press and releases expect X11 keysyms.
  void key_press(int keysym);
  void key_release(int keysym);
  void move_mouse(int x, int y);

  // Button mapping:
  // 1: left mouse
  // 2. middle mouse
  // 3. right mouse
  void mouse_press(int button);
  void mouse_release(int button);

  // Exposed to simplify vnc_loop() implementation. Not part of the public API.
  void resize(int w, int h);
  void update(int x, int y, int w, int h);

 protected:
  StatusVal connect_impl(int32_t port);
  BounceDeskClient() = default;

 private:
  void vnc_loop();
  void send_pointer_event();

  int port_;
  std::mutex client_mu_;
  rfbClient* client_;
  std::atomic<bool> stop_vnc_ = false;
  std::thread vnc_loop_;
  Frame frame_;

  int mouse_x_ = 10;
  int mouse_y_ = 10;
  int button_mask_ = 0;
};

#endif
