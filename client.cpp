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

#include <thread>

#include "client_loop.h"
#include "third_party/status/status_or.h"

StatusOr<std::unique_ptr<BounceDeskClient>> BounceDeskClient::connect(
    int32_t port) {
  auto client = std::make_unique<BounceDeskClient>();
  client->vnc_loop_ = std::thread(run_client, port, resize, update,
                                  /*client_data=*/client.get());
  return std::move(client);
}

void BounceDeskClient::on_resize(int width, int height) {
  std::lock_guard l(frame_.mu);
  if (frame_.pixels) {
    free(frame_.pixels);
  }
  frame_.width = width;
  frame_.height = height;
  frame_.pixels = (uint8_t*)malloc(4 * width * height);
};

void BounceDeskClient::on_update(uint8_t* fb, int x, int y, int w, int h) {
  std::lock_guard l(frame_.mu);
  int fw = frame_.width;
  size_t start_offset = 4 * (y * fw + x);
  size_t end_offset = 4 * ((y + h) * fw + (x + w));
  memcpy(frame_.pixels + start_offset, fb + start_offset,
         end_offset - start_offset);
};

BounceDeskClient::~BounceDeskClient() {
  if (vnc_loop_.joinable()) {
    vnc_loop_.join();
  }
  if (frame_.pixels) {
    free(frame_.pixels);
  }
}

const Frame& BounceDeskClient::get_frame() { return frame_; }
