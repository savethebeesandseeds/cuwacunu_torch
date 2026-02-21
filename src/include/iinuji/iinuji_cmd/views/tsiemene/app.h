#pragma once

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/views/tsiemene/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool handle_tsi_key(CmdState& st, int ch) {
  if (st.screen != ScreenMode::Tsiemene) return false;
  if (ch == KEY_DOWN) {
    select_next_tsi_tab(st);
    return true;
  }
  if (ch == KEY_UP) {
    select_prev_tsi_tab(st);
    return true;
  }
  return false;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
