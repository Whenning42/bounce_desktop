#include "client.h"

#include <gtest/gtest.h>
#include <unistd.h>

#include "wayland_backend.h"

const int32_t kPortOffset = 5900;

TEST(Client, GetPixelsReturnsAFrame) {
  auto backend =
      WaylandBackend::start_server(kPortOffset, 300, 200, {"sleep", "1000"});
  sleep(1);
  auto client_result = BounceDeskClient::connect((*backend)->port());
  ASSERT_TRUE(client_result.ok());
  auto& client = **client_result;

  const Frame& frame = client.get_frame();
  EXPECT_EQ(frame.width, 300);
  EXPECT_EQ(frame.height, 200);
}
