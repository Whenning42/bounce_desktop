#ifndef BINDINGS_CLIENT_EXT_H_
#define BINDINGS_CLIENT_EXT_H_

#include <memory>

#include "desktop/client.h"
#include "third_party/status/status_or.h"
#include "desktop/weston_backend.h"

class Desktop : public BounceDeskClient {
 public:
  static std::unique_ptr<Desktop> create(
      int32_t width, int32_t height, const std::vector<std::string>& command);

 private:
  Desktop() {};

  // Hide BounceDeskClient methods that don't belong in Desktop's interface.
  using BounceDeskClient::connect;
  using BounceDeskClient::get_frame_impl;
  using BounceDeskClient::resize;

  std::unique_ptr<WestonBackend> backend_;
};

#endif  // BINDINGS_CLIENT_EXT_H_
