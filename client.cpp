#include "client.h"

#include <gvnc-1.0/gvnc.h>
#include <string.h>

#include <cassert>
#include <future>
#include <thread>

#include "mouse_button.h"
#include "third_party/status/status_or.h"
#include "time_aliases.h"

const char* kPtrKey = "inst";
const uint32_t kUnusedScancode = 0;

namespace {
VncPixelFormat* local_format() {
  static bool init = false;
  static VncPixelFormat* fmt = vnc_pixel_format_new();
  if (!init) {
    init = true;
    fmt->depth = 24;
    fmt->bits_per_pixel = 32;
    fmt->red_max = 255;
    fmt->blue_max = 255;
    fmt->green_max = 255;
    // TODO: Figure out exactly how pixel formats work?
    // Buffer seems to come back BGRA regardless of what we
    // set for these shifts?
    fmt->red_shift = 16;
    fmt->green_shift = 8;
    fmt->blue_shift = 0;
    fmt->true_color_flag = 1;
  }
  return fmt;
}

const VncPixelFormat* remote_format(VncConnection* c) {
  const VncPixelFormat* r = vnc_connection_get_pixel_format(c);
  return r ? r : local_format();
}

void on_connected(VncConnection* c, void* data) { (void)c, (void)data; }

void on_initialized(VncConnection* c, void* data) {
  (void)data;
  auto client = (BounceDeskClient*)g_object_get_data(G_OBJECT(c), kPtrKey);
  int width = vnc_connection_get_width(c);
  int height = vnc_connection_get_height(c);
  client->resize(width, height);
  CHECK(
      vnc_connection_framebuffer_update_request(c, false, 0, 0, width, height));
  client->initialized_ = true;
}

void on_resize(VncConnection* c, uint16_t width, uint16_t height, void* data) {
  (void)data;
  auto client = (BounceDeskClient*)g_object_get_data(G_OBJECT(c), kPtrKey);
  client->resize(width, height);
}

void on_framebuffer_update(VncConnection* c, uint16_t x, uint16_t y,
                           uint16_t width, uint16_t height, void* data) {
  (void)c, (void)x, (void)y, (void)data;
  CHECK(
      vnc_connection_framebuffer_update_request(c, false, 0, 0, width, height));
}

void on_error(VncConnection* c, const char* msg, void* data) {
  (void)c, (void)data;
  fprintf(stderr, "================= VNC ERROR: %s ===============\n", msg);
}

}  // namespace

StatusOr<std::unique_ptr<BounceDeskClient>> BounceDeskClient::connect(
    int32_t port) {
  auto client = std::unique_ptr<BounceDeskClient>(new BounceDeskClient());
  RETURN_IF_ERROR(client->connect_impl(port));

  // Block until the client's finished start up so that subsequent member
  // functions don't race with the start up.
  auto start = sc_now();
  while (sc_now() - start < 5s) {
    if (client->initialized_) break;
    sleep_for(50us);
  }
  if (!client->initialized_) {
    return InternalError(
        "Failed to initialize vnc client connection to server.");
  }
  return client;
}

StatusVal BounceDeskClient::connect_impl(int32_t port) {
  port_ = port;
  vnc_loop_ = std::thread(&BounceDeskClient::vnc_loop, this);
  return OkStatus();
}

static gboolean shutdown(void* loop) {
  g_main_loop_quit((GMainLoop*)(loop));
  return G_SOURCE_REMOVE;
}

BounceDeskClient::~BounceDeskClient() {
  if (main_loop_) {
    g_main_context_invoke(NULL, shutdown, main_loop_);
  }
  while (!exited_) {
    sleep_for(50ms);
  }
  if (fb_) {
    auto* buf = vnc_framebuffer_get_buffer(fb_);
    if (buf) free(buf);
    g_object_unref(fb_);
    fb_ = nullptr;
  }
  if (c_) {
    g_object_unref(c_);
    c_ = nullptr;
  }
  if (vnc_loop_.joinable()) {
    vnc_loop_.join();
  }
}

void BounceDeskClient::resize(int width, int height) {
  int old_width = -1;
  int old_height = -1;
  uint8_t* buffer;
  if (fb_) {
    old_width = vnc_framebuffer_get_width(fb_);
    old_height = vnc_framebuffer_get_height(fb_);
    if (old_width == width && old_height == height) {
      return;
    }

    buffer = vnc_framebuffer_get_buffer(fb_);
    if (buffer) {
      free(buffer);
    }
    g_object_unref(fb_);
  }

  buffer = (uint8_t*)malloc(width * height * 4);
  fb_ = VNC_FRAMEBUFFER(vnc_base_framebuffer_new(
      buffer, width, height, 4 * width, local_format(), remote_format(c_)));
  CHECK(vnc_connection_set_framebuffer(c_, fb_));
  CHECK(vnc_connection_framebuffer_update_request(c_, false, 0, 0, width,
                                                  height));
}

struct GetFrameData {
  std::promise<Frame> frame;
  BounceDeskClient* ptr;
};
int invoke_get_frame(void* data) {
  GetFrameData* f = (GetFrameData*)data;
  f->frame.set_value(f->ptr->get_frame_impl());
  return G_SOURCE_REMOVE;
}
Frame BounceDeskClient::get_frame() {
  GetFrameData f;
  f.ptr = this;
  g_main_context_invoke(NULL, invoke_get_frame, &f);
  auto fr = f.frame.get_future().get();
  return fr;
}

Frame BounceDeskClient::get_frame_impl() {
  int w = vnc_framebuffer_get_width(fb_);
  int h = vnc_framebuffer_get_height(fb_);
  const uint8_t* buffer = vnc_framebuffer_get_buffer(fb_);
  size_t size = 4 * w * h;
  uint8_t* fb_copy = (uint8_t*)malloc(size);
  memcpy(fb_copy, buffer, size);
  return Frame{.width = w, .height = h, .pixels = UniquePtrBuf(fb_copy)};
}

void BounceDeskClient::vnc_loop() {
  c_ = vnc_connection_new();
  g_object_set_data(G_OBJECT(c_), kPtrKey, this);

  int enc[] = {VNC_CONNECTION_ENCODING_RAW,
               VNC_CONNECTION_ENCODING_EXTENDED_DESKTOP_RESIZE,
               VNC_CONNECTION_ENCODING_DESKTOP_RESIZE};
  CHECK(vnc_connection_set_encodings(c_, sizeof(enc) / sizeof(enc[0]), enc));
  CHECK(vnc_connection_set_pixel_format(c_, local_format()));
  CHECK(vnc_connection_set_auth_type(c_, VNC_CONNECTION_AUTH_NONE));
  g_signal_connect(c_, "vnc-connected", G_CALLBACK(on_connected), NULL);
  g_signal_connect(c_, "vnc-initialized", G_CALLBACK(on_initialized), this);
  g_signal_connect(c_, "vnc-desktop-resize", G_CALLBACK(on_resize), NULL);
  g_signal_connect(c_, "vnc-framebuffer-update",
                   G_CALLBACK(on_framebuffer_update), NULL);
  g_signal_connect(c_, "vnc-error", G_CALLBACK(on_error), NULL);

  std::string port_str = std::to_string(port_);
  CHECK(vnc_connection_open_host(c_, "127.0.0.1", port_str.c_str()));

  main_loop_ = g_main_loop_new(NULL, false);
  g_main_loop_run(main_loop_);
  g_main_loop_unref(main_loop_);
  exited_ = true;
}

struct DoKeyEvent {
  VncConnection* c;
  bool down;
  int keysym;
  std::promise<bool> ret = std::promise<bool>();
};
static int do_key_event(void* data) {
  DoKeyEvent* ke = (DoKeyEvent*)(data);
  CHECK(vnc_connection_key_event(ke->c, ke->down, ke->keysym, kUnusedScancode));
  ke->ret.set_value(true);
  return G_SOURCE_REMOVE;
}

void BounceDeskClient::key_press(int keysym) {
  DoKeyEvent ke = DoKeyEvent{.c = c_, .down = true, .keysym = keysym};
  g_main_context_invoke(NULL, do_key_event, &ke);
  ke.ret.get_future().get();
}

void BounceDeskClient::key_release(int keysym) {
  DoKeyEvent ke = DoKeyEvent{.c = c_, .down = false, .keysym = keysym};
  g_main_context_invoke(NULL, do_key_event, &ke);
  ke.ret.get_future().get();
}

void BounceDeskClient::move_mouse(int x, int y) {
  mouse_x_ = x;
  mouse_y_ = y;
  send_pointer_event();
}

void BounceDeskClient::mouse_press(int button) {
  button_mask_ = set_button_mask(button_mask_, button, /*pressed=*/true);
  send_pointer_event();
}

void BounceDeskClient::mouse_release(int button) {
  button_mask_ = set_button_mask(button_mask_, button, /*pressed=*/false);
  send_pointer_event();
}

struct DoPointerEvent {
  VncConnection* c;
  int mask;
  int x;
  int y;
  std::promise<bool> ret = std::promise<bool>();
};
static int do_pointer_event(void* data) {
  DoPointerEvent* pe = (DoPointerEvent*)(data);
  CHECK(vnc_connection_pointer_event(pe->c, pe->mask, pe->x, pe->y));
  pe->ret.set_value(true);
  return G_SOURCE_REMOVE;
}

void BounceDeskClient::send_pointer_event() {
  DoPointerEvent pe{
      .c = c_,
      .mask = button_mask_,
      .x = mouse_x_,
      .y = mouse_y_,
  };
  g_main_context_invoke(NULL, do_pointer_event, &pe);
  pe.ret.get_future().get();
}
