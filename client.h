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
  const Frame& get_frame();

  void on_resize(int w, int h);
  void on_update(uint8_t* fb, int x, int y, int w, int h);

  // TODO: Input support
  // Mouse:
  //   Move, press, release
  //
  // Keyboard:
  //   Press key, Release key

 private:
  std::thread vnc_loop_;
  Frame frame_;
};

inline rfbBool resize(rfbClient* client) {
  int w = client->width;
  int h = client->height;
  void* client_data = rfbClientGetClientData(client, nullptr);
  ((BounceDeskClient*)client_data)->on_resize(w, h);
  return TRUE;
}
inline void update(rfbClient* client, int x, int y, int w, int h) {
  void* client_data = rfbClientGetClientData(client, nullptr);
  ((BounceDeskClient*)client_data)->on_update(client->frameBuffer, x, y, w, h);
}
