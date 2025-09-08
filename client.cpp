// How should I manage frames?
//
// libvnc FB - Controlled by libvnc loop. Doesn't expose safe sections.
// Bounce FB - Stores a copy of the FB. Updated every time libvnc's fb updates.
//             - With better synchronization, we could update this less often.
//
//
// run_loop takes two callbacks:
// - resize: (framebuffer, width, height)
// - update: (framebuffer)

#include "client.h"

#include <string.h>

#include <cassert>
#include <thread>

#include "third_party/status/status_or.h"

StatusOr<std::unique_ptr<BounceDeskClient>> BounceDeskClient::connect(
    int32_t port, ConnectionOptions options) {
  auto client = std::unique_ptr<BounceDeskClient>(new BounceDeskClient());
  client->vnc_loop_ = std::thread(&BounceDeskClient::vnc_loop, client.get());
  client->connection_options_ = std::move(options);
  return std::move(client);
}

void BounceDeskClient::resize(int width, int height) {
  std::lock_guard l(frame_.mu);
  if (frame_.pixels) {
    free(frame_.pixels);
  }
  if (client_->frameBuffer) {
    free(client_->frameBuffer);
  }
  frame_.width = width;
  frame_.height = height;
  frame_.pixels = (uint8_t*)malloc(4 * width * height);
  client_->frameBuffer = (uint8_t*)malloc(4 * width * height);
};

void BounceDeskClient::update(int x, int y, int w, int h) {
  if (!client_->frameBuffer) {
    ERROR("Framebuffer is null!");
    return;
  }
  std::lock_guard l(frame_.mu);
  // Copy the whole frame every update and ignore x, y, w, h.
  (void)x, (void)y, (void)w, (void)h;
  size_t frame_bytes = 4 * frame_.width * frame_.height;
  memcpy(frame_.pixels, client_->frameBuffer, frame_bytes);
};

BounceDeskClient::~BounceDeskClient() {
  stop_vnc_ = true;
  if (vnc_loop_.joinable()) {
    vnc_loop_.join();
  }
  if (frame_.pixels) {
    free(frame_.pixels);
  }
}

const Frame& BounceDeskClient::get_frame() { return frame_; }

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
  ERROR("Asked for credential of type: %d", credential_type);
  exit(1);
}
}  // namespace

void BounceDeskClient::vnc_loop() {
  rfbClient* client = rfbGetClient(/*bitsPerSample=*/8, /*samplesPerPixel=*/3,
                                   /*bytesPerPixel*/ 4);
  rfbClientSetClientData(client, /*tag=*/nullptr, this);
  client_ = client;

  // TODO: Consider handling clipboard.
  // TODO: Consider setting rfbClientLog to a logging function.
  rfbClientLog = rfb_client_log;
  static const char* server_host = "localhost";
  client->serverHost = strdup(server_host);
  client->serverPort = connection_options_.port;

  client->MallocFrameBuffer = call_resize;
  client->canHandleNewFBSize = TRUE;
  client->GotFrameBufferUpdate = call_update;
  client->GetCredential = unexpected_credential_error;

  int client_argc = 3;
  const char* client_argv[] = {"bounce_vnc", "-encodings", "raw"};
  if (!rfbInitClient(client, &client_argc, (char**)client_argv)) {
    ERROR("Failed to start the server");
  }

  while (!stop_vnc_) {
    int r = WaitForMessage(client, 1'000);
    if (r < 0) {
      ERROR("Wait for message error: %d", r);
    }
    if (r == 0) {
      continue;
    }
    rfbBool ret = HandleRFBServerMessage(client);
    if (ret == FALSE) {
      ERROR("Couldn't handle rfb server message");
      break;
    }

    SendFramebufferUpdateRequest(client, 0, 0, client->width, client->height,
                                 0);
  }
}
// End VNC-loop
