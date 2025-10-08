#ifndef DESKTOP_SDL_VIEWER_H_
#define DESKTOP_SDL_VIEWER_H_

#include <memory>
#include <string>
#include <thread>

#include "desktop/client.h"
#include "third_party/status/status_or.h"

#define MOVEABLE_NOT_COPYABLE_CUSTOM(cls) \
  cls(cls&& other);                       \
  cls& operator=(cls&& other) = delete;   \
  cls(const cls& other) = delete;         \
  cls& operator=(const cls& other) = delete;

class SDLViewer {
 public:
  MOVEABLE_NOT_COPYABLE_CUSTOM(SDLViewer);

  // Create a viewer by calling open().
  static StatusOr<std::unique_ptr<SDLViewer>> open(
      std::shared_ptr<BounceDeskClient> client,
      std::string window_name = "Bounce Viewer", bool allow_unsafe = false);

  // Closes the viewer if it's still open.
  ~SDLViewer();

  // Close the viewer, closing its connection and its viewing window.
  void close();

  // Returns whether the window's been closed by any of: user, application
  // error, or close() call.
  bool was_closed() { return was_closed_; }

 private:
  SDLViewer() = default;

  std::string window_name_;

  std::atomic<bool> exit_loop_ = false;
  std::atomic<bool> was_closed_ = false;
  void app_loop();

  std::shared_ptr<BounceDeskClient> client_;
  std::thread app_loop_;
};

#endif  // DESKTOP_SDL_VIEWER_H_
