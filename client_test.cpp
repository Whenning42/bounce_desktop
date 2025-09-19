#include "client.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unistd.h>

#include "mock_vnc_server.h"
#include "mouse_button.h"
#include "third_party/status/status_gtest.h"
#include "wayland_backend.h"

const int32_t kPortOffset = 5900;

TEST(Client, get_frame_returns_a_frame) {
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

TEST(Client, sends_input_events_correctly) {
  ASSERT_OK_AND_ASSIGN(auto server, MockVncServer::start_server(5967));
  ASSERT_OK_AND_ASSIGN(auto client, BounceDeskClient::connect(5967));
  EXPECT_OK(server->wait_for_connection());

  client->key_press(63);
  client->key_release(63);
  client->move_mouse(50, 50);
  client->mouse_press(1);
  client->mouse_press(2);
  client->move_mouse(70, 70);
  client->mouse_release(1);
  client->mouse_release(2);
  // Wait for server to receive and process the client event requests.
  std::this_thread::sleep_for(std::chrono::milliseconds(100));

  std::vector<Event> expected_events = {
      Event::key_press(63),
      Event::key_release(63),
      Event::mouse_event(50, 50, make_button_mask({})),
      Event::mouse_event(50, 50, make_button_mask({1})),
      Event::mouse_event(50, 50, make_button_mask({1, 2})),
      Event::mouse_event(70, 70, make_button_mask({1, 2})),
      Event::mouse_event(70, 70, make_button_mask({2})),
      Event::mouse_event(70, 70, make_button_mask({})),
  };

  EXPECT_THAT(server->get_events(), testing::ContainerEq(expected_events));
}
