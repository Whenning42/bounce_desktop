#ifndef EVENT_H_
#define EVENT_H_

struct Event {
  enum class Type {
    NONE = 0,
    KEYBOARD = 1,
    MOUSE = 2,
  };

  enum class Direction {
    NONE = 0,
    PRESS = 1,
    RELEASE = 2,
  };

  Type type = Type::NONE;
  int keysym = 0;
  Direction key_direction = Direction::NONE;
  int mouse_x = 0;
  int mouse_y = 0;
  int button_mask = 0;

  bool operator==(const Event& other) const {
    return type == other.type && keysym == other.keysym &&
           key_direction == other.key_direction && mouse_x == other.mouse_x &&
           mouse_y == other.mouse_y && button_mask == other.button_mask;
  }

  static Event key_press(int keysym) {
    return Event{.type = Type::KEYBOARD,
                 .keysym = keysym,
                 .key_direction = Direction::PRESS};
  }

  static Event key_release(int keysym) {
    return Event{.type = Type::KEYBOARD,
                 .keysym = keysym,
                 .key_direction = Direction::RELEASE};
  }

  static Event mouse_event(int x, int y, int button_mask) {
    return Event{.type = Type::MOUSE,
                 .mouse_x = x,
                 .mouse_y = y,
                 .button_mask = button_mask};
  }
};

#endif
