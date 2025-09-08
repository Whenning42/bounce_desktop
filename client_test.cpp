#include "client.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include "wayland_backend.h"

TEST(Client, GetPixelsReturnsAFrame) {
  const int32_t port = 5976;
  auto backend = WaylandBackend::start_server(port, 300, 200);
  sleep(1);
  auto client_result =
      BounceDeskClient::connect(port, ConnectionOptions{.port = port});
  ASSERT_TRUE(client_result.ok());
  auto& client = **client_result;
  sleep(1);

  const Frame& frame = client.get_frame();
  EXPECT_EQ(frame.width, 300);
  EXPECT_EQ(frame.height, 200);
}
