// Test that we can create a wayland backend running factorio, and that we
// can randomly move the mouse across the screen via a client while viewing the
// desktop with the SDL viewer.

#include "client.h"
#include "process.h"
#include "sdl_viewer.h"
#include "third_party/status/status_or.h"
#include "wayland_backend.h"

const int kWidth = 800;
const int kHeight = 600;

int main(int argc, char** argv) {
  (void)argc, (void)argv;

  ProcessOutConf out_conf = ProcessOutConf{
      .stdout = StreamOutConf::File("/tmp/bounce_integration_stdout.txt")
                    .value_or_die(),
      .stderr = StreamOutConf::File("/tmp/bounce_integration_stderr.txt")
                    .value_or_die(),
  };
  auto backend =
      std::move(WaylandBackend::start_server(
                    5900, kWidth, kHeight,
                    {"/home/william/Games/factorio/bin/x64/factorio"},
                    std::move(out_conf))
                    .value_or_die());
  auto client_unique =
      std::move(BounceDeskClient::connect(backend->port()).value_or_die());
  std::shared_ptr<BounceDeskClient> client = std::move(client_unique);
  auto viewer = std::move(SDLViewer::open(client).value_or_die());

  int x = 0;
  int y = 0;
  while (!viewer->was_closed()) {
    x = (x + 70) % kWidth;
    y = (y + 20) % kHeight;
    client->move_mouse(x, y);
    std::this_thread::sleep_for(std::chrono::milliseconds(250));
  }

  return 0;
}
