#pragma once

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/views/board/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool handle_board_key(CmdState& st, int ch) {
  if (st.screen != ScreenMode::Board) return false;
  if (ch == KEY_DOWN) {
    if (select_next_board_circuit(st)) return true;
    return false;
  }
  if (ch == KEY_UP) {
    if (select_prev_board_circuit(st)) return true;
    return false;
  }
  return false;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
