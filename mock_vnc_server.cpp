#include "mock_vnc_server.h"

#include <thread>

StatusOr<std::unique_ptr<MockVncServer>> MockVncServer::start_server(int port) {
  auto server = std::unique_ptr<MockVncServer>(new MockVncServer(port));
  server->vnc_loop_ = std::thread(&MockVncServer::vnc_loop, server.get());
  return server;
}

MockVncServer::MockVncServer(int port) : port_(port) {}

MockVncServer::~MockVncServer() {
  stop_vnc_ = true;
  if (vnc_loop_.joinable()) {
    vnc_loop_.join();
  }
  if (screen_ && screen_->frameBuffer) {
    free(screen_->frameBuffer);
    screen_->frameBuffer = nullptr;
  }
  if (screen_) {
    rfbScreenCleanup(screen_);
  }
}

rfbNewClientAction MockVncServer::call_handle_connection(rfbClientPtr client) {
  MockVncServer* inst = (MockVncServer*)client->screen->screenData;
  inst->handle_connection();
  return RFB_CLIENT_ACCEPT;
}

void MockVncServer::call_handle_key(rfbBool down, rfbKeySym k,
                                    rfbClientPtr client) {
  MockVncServer* inst = (MockVncServer*)client->screen->screenData;
  inst->handle_key(down, k);
}

void MockVncServer::call_handle_ptr(int button_mask, int x, int y,
                                    rfbClientPtr client) {
  MockVncServer* inst = (MockVncServer*)client->screen->screenData;
  inst->handle_ptr(button_mask, x, y);
}

void MockVncServer::handle_connection() { connected_ = true; }

void MockVncServer::handle_key(rfbBool down, rfbKeySym k) {
  if (down) {
    add_event(Event::key_press(k));
  } else {
    add_event(Event::key_release(k));
  }
}

void MockVncServer::handle_ptr(int button_mask, int x, int y) {
  add_event(Event::mouse_event(x, y, button_mask));
}

std::vector<Event> MockVncServer::get_events() {
  std::vector<Event> events_cpy;
  {
    std::lock_guard l(events_mu_);
    events_cpy = events_;
  }
  return events_cpy;
}

void MockVncServer::add_event(Event&& event) {
  std::lock_guard l(events_mu_);
  events_.push_back(std::move(event));
}

StatusVal MockVncServer::wait_for_connection() {
  auto start = std::chrono::steady_clock::now();
  auto timeout = std::chrono::seconds(1);
  while (std::chrono::steady_clock::now() - start < timeout) {
    if (connected_) return OkStatus();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
  return DeadlineExceededError(
      "Timed out waiting for MockVncServer connection");
}

void MockVncServer::vnc_loop() {
  const int32_t width = 300;
  const int32_t height = 200;

  uint8_t* fb = (uint8_t*)calloc(4 * width * height, 1);
  int argc = 0;
  char** argv = nullptr;
  rfbScreenInfo* s = rfbGetScreen(&argc, argv, width, height,
                                  /*bitsPerSample=*/8, /*samplesPerPixel=*/3,
                                  /*bytesPerPixel*/ 4);

  s->port = port_;
  static const char* server_host = "localhost";
  strcpy(s->thisHost, server_host);
  std::string addr = "127.0.0.1";
  rfbStringToAddr((char*)addr.c_str(), &s->listenInterface);
  s->ipv6port = -1;
  s->httpPort = -1;

  s->frameBuffer = (char*)fb;
  s->newClientHook = call_handle_connection;
  s->kbdAddEvent = call_handle_key;
  s->ptrAddEvent = call_handle_ptr;
  s->screenData = this;
  screen_ = s;

  rfbInitServer(s);

  while (!stop_vnc_) {
    rfbProcessEvents(s, 999'999);
  }
}
