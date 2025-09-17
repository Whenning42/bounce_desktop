#ifndef FRAME_
#define FRAME_

#include <cstdint>
#include <mutex>

struct Frame {
  std::mutex mu;
  int32_t width = 0;
  int32_t height = 0;
  uint8_t* pixels = nullptr;
};

#endif  // FRAME_
