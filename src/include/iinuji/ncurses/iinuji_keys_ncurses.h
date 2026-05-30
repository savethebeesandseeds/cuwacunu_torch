/* iinuji_keys_ncurses.h */
#pragma once

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_keys.h"

namespace cuwacunu {
namespace iinuji {

[[nodiscard]] inline int translate_ncurses_key(int ch) {
  switch (ch) {
  case KEY_UP:
    return key_up;
  case KEY_DOWN:
    return key_down;
  case KEY_LEFT:
    return key_left;
  case KEY_RIGHT:
    return key_right;
  case KEY_HOME:
    return key_home;
  case KEY_END:
    return key_end;
  case KEY_PPAGE:
    return key_page_up;
  case KEY_NPAGE:
    return key_page_down;
  case KEY_BACKSPACE:
    return key_backspace;
  case KEY_DC:
    return key_delete;
  case KEY_ENTER:
    return key_enter;
  case KEY_BTAB:
    return key_shift_tab;
  default:
    return ch;
  }
}

} // namespace iinuji
} // namespace cuwacunu
