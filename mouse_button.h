#ifndef MOUSE_BUTTON_H_
#define MOUSE_BUTTON_H_

#include <vector>

inline int set_button_mask(int button_mask, int button, bool pressed) {
  if (pressed) {
    return button_mask | (1 << button);
  } else {
    return button_mask & ~(1 << button);
  }
}

inline int make_button_mask(const std::vector<int>& pressed_buttons) {
  int mask = 0;
  for (int b : pressed_buttons) {
    mask = set_button_mask(mask, b, /*pressed=*/true);
  }
  return mask;
}

#endif
