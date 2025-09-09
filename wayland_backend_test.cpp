#include "wayland_backend.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unistd.h>

const int32_t kPort = 5930;

TEST(WaylandBackend, FetchesDisplayNames) {
  auto backend_status = WaylandBackend::start_server(kPort, 300, 200);
  ASSERT_TRUE(backend_status.ok());
  auto backend = std::move(backend_status.value_or_die());
  sleep(1);
  EXPECT_THAT(backend->get_x_display(), testing::MatchesRegex(R"(:[0-9]+)"));
  EXPECT_THAT(backend->get_wayland_display(),
              testing::MatchesRegex(R"(wayland-[0-9]+)"));
}
