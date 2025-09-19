#ifndef FRAME_
#define FRAME_

#include <cstdint>
#include <memory>

struct free_data {
  void operator()(uint8_t* p) const noexcept { free(p); }
};

using UniquePtrBuf = std::unique_ptr<uint8_t[], free_data>;

struct Frame {
  int32_t width = 0;
  int32_t height = 0;
  UniquePtrBuf pixels;

  UniquePtrBuf take_pixels() { return std::move(pixels); }
};

#endif  // FRAME_
