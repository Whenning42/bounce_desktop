#include "client.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include "wayland_backend.h"

TEST(Client, GetPixelsReturnsAFrame) {
  const int32_t port = 5976;
  auto backend = WaylandBackend::start_server(port);
  sleep(1);
  auto client_result = BounceDeskClient::connect(port);
  ASSERT_TRUE(client_result.ok());
  auto& client = **client_result;
  sleep(1);

  const Frame& frame = client.get_frame();
  EXPECT_GT(frame.width, 50);
  EXPECT_LT(frame.width, 8000);
  EXPECT_GT(frame.height, 50);
  EXPECT_LT(frame.height, 8000);
}
