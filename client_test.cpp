#include "client.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include "wayland_backend.h"

const int32_t kPort = 5930;

TEST(Client, GetPixelsReturnsAFrame) {
  auto backend = WaylandBackend::start_server(kPort, 300, 200);
  sleep(1);
  auto client_result =
      BounceDeskClient::connect(kPort, ConnectionOptions{.port = kPort});
  ASSERT_TRUE(client_result.ok());
  auto& client = **client_result;
  sleep(1);

  const Frame& frame = client.get_frame();
  EXPECT_EQ(frame.width, 300);
  EXPECT_EQ(frame.height, 200);
}
