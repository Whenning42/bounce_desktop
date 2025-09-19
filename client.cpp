#include "client.h"

#include <string.h>

#include <cassert>
#include <thread>

#include "mouse_button.h"
#include "third_party/status/status_or.h"

namespace {
int32_t fb_size(int w, int h) { return w * h * 4; }
}  // namespace

StatusOr<std::unique_ptr<BounceDeskClient>> BounceDeskClient::connect(
    int32_t port) {
  auto client = std::unique_ptr<BounceDeskClient>(new BounceDeskClient());
  RETURN_IF_ERROR(client->connect_impl(port));
  return client;
}

StatusVal BounceDeskClient::connect_impl(int32_t port) {
  port_ = port;
  vnc_loop_ = std::thread(&BounceDeskClient::vnc_loop, this);
  return OkStatus();
}

void BounceDeskClient::resize(int width, int height) {
  if (client_->frameBuffer) {
    free(client_->frameBuffer);
    client_->frameBuffer = nullptr;
  }
  client_->width = width;
  client_->height = height;
  client_->frameBuffer = (uint8_t*)malloc(4 * width * height);
};

void BounceDeskClient::update(int x, int y, int w, int h) {
  (void)x, (void)y, (void)w, (void)h;
  if (!client_->frameBuffer) {
    ERROR("Framebuffer is null!");
    return;
  }
};

BounceDeskClient::~BounceDeskClient() {
  stop_vnc_ = true;
  if (vnc_loop_.joinable()) {
    vnc_loop_.join();
  }
  if (client_ && client_->frameBuffer) {
    free(client_->frameBuffer);
    client_->frameBuffer = nullptr;
  }
  if (client_) {
    rfbClientCleanup(client_);
  }
}

Frame BounceDeskClient::get_frame() {
  std::lock_guard l(client_mu_);
  int w = client_->width;
  int h = client_->height;
  size_t size = fb_size(w, h);
  uint8_t* fb_copy = (uint8_t*)malloc(size);
  memcpy(fb_copy, client_->frameBuffer, size);
  return Frame{.width = w, .height = h, .pixels = UniquePtrBuf(fb_copy)};
}

// Begin VNC-loop
namespace {
rfbBool call_resize(rfbClient* client) {
  BounceDeskClient* desk =
      (BounceDeskClient*)rfbClientGetClientData(client, nullptr);
  assert(desk);
  desk->resize(client->width, client->height);
  return TRUE;
}

void call_update(rfbClient* client, int x, int y, int w, int h) {
  BounceDeskClient* desk =
      (BounceDeskClient*)rfbClientGetClientData(client, nullptr);
  assert(desk);
  desk->update(x, y, w, h);
}

void rfb_client_log(const char* fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
}

rfbCredential* unexpected_credential_error(rfbClient* client,
                                           int credential_type) {
  (void)client;
  ERROR("Asked for credential of type: %d", credential_type);
  exit(1);
}
}  // namespace

void BounceDeskClient::vnc_loop() {
  client_ = rfbGetClient(/*bitsPerSample=*/8, /*samplesPerPixel=*/3,
                         /*bytesPerPixel*/ 4);
  rfbClientSetClientData(client_, /*tag=*/nullptr, this);

  // Note: We don't support any clipboard behavior.
  // TODO: Consider setting rfbClientLog to a logging function.
  rfbClientLog = rfb_client_log;
  static const char* server_host = "localhost";
  client_->serverHost = strdup(server_host);
  client_->serverPort = port_;

  client_->MallocFrameBuffer = call_resize;
  client_->canHandleNewFBSize = TRUE;
  client_->GotFrameBufferUpdate = call_update;
  client_->GetCredential = unexpected_credential_error;

  int client_argc = 3;
  const char* client_argv[] = {"bounce_vnc", "-encodings", "raw"};
  if (!rfbInitClient(client_, &client_argc, (char**)client_argv)) {
    ERROR("Failed to start the server");
  }

  while (!stop_vnc_) {
    int r;
    {
      std::lock_guard l(client_mu_);
      r = WaitForMessage(client_, 1'000);
    }
    if (r < 0) ERROR("Wait for message error: %d", r);
    if (r == 0) continue;

    rfbBool ret;
    {
      std::lock_guard l(client_mu_);
      ret = HandleRFBServerMessage(client_);
    }
    if (ret == FALSE) {
      ERROR("Couldn't handle rfb server message");
      break;
    }
  }
}
// End VNC-loop

void BounceDeskClient::key_press(int keysym) {
  std::lock_guard l(client_mu_);
  SendKeyEvent(client_, keysym, /*down=*/true);
}

void BounceDeskClient::key_release(int keysym) {
  std::lock_guard l(client_mu_);
  SendKeyEvent(client_, keysym, /*down=*/false);
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

void BounceDeskClient::send_pointer_event() {
  std::lock_guard l(client_mu_);
  SendPointerEvent(client_, mouse_x_, mouse_y_, button_mask_);
}
