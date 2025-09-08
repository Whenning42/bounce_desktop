#ifndef FRAME_
#define FRAME_

#include <cstdint>
#include <mutex>

struct Frame {
  std::mutex mu;
  int32_t width;
  int32_t height;
  uint8_t* pixels;
};

#endif  // FRAME_
