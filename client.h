#ifndef CLIENT_H_
#define CLIENT_H_

#include <gvnc-1.0/gvnc.h>
#include <stdint.h>

#include <atomic>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>

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

  // Note: Any of these public API calls can only be called outside
  // of our internal glib main thread. They'll deadlock if called from
  // the glib thread.
  Frame get_frame();
  // Shouldn't be called directly.
  Frame get_frame_impl();

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
  void fb_update();
  std::atomic<bool> initialized_ = false;

 protected:
  StatusVal connect_impl(int32_t port);
  BounceDeskClient() = default;

 private:
  void vnc_loop();
  void send_pointer_event();

  int port_;
  std::thread vnc_loop_;
  Frame frame_;

  VncConnection* c_ = nullptr;
  VncFramebuffer* fb_ = nullptr;
  GMainLoop* main_loop_ = nullptr;
  std::atomic<bool> exited_ = false;

  std::mutex pending_requests_mu_;
  std::vector<std::promise<Frame>*> pending_requests_;

  int mouse_x_ = 10;
  int mouse_y_ = 10;
  int button_mask_ = 0;
};

#endif
