#ifndef MOCK_VNC_SERVER_H_
#define MOCK_VNC_SERVER_H_

#include <rfb/rfb.h>

#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "event.h"
#include "third_party/status/status_or.h"

class MockVncServer {
 public:
  static StatusOr<std::unique_ptr<MockVncServer>> start_server(int port);
  ~MockVncServer();

  // Returns a DEADLINE_EXCEEDED error if the server doesn't receive a
  // connection in a second.
  StatusVal wait_for_connection();

  // Return by copy so that the caller's vec won't be changed out from under
  // them.
  std::vector<Event> get_events();

 private:
  MockVncServer(int port);
  void vnc_loop();

  static rfbNewClientAction call_handle_connection(rfbClientPtr client);
  static void call_handle_key(rfbBool down, rfbKeySym k, rfbClientPtr client);
  static void call_handle_ptr(int button_mask, int x, int y,
                              rfbClientPtr client);
  void handle_connection();
  void handle_key(rfbBool down, rfbKeySym k);
  void handle_ptr(int button_mask, int x, int y);

  void add_event(Event&& event);

  int port_ = 0;
  std::atomic<bool> connected_ = 0;
  bool stop_vnc_ = false;
  std::thread vnc_loop_;
  rfbScreenInfo* screen_ = nullptr;

  std::mutex events_mu_;
  std::vector<Event> events_;
};

#endif
