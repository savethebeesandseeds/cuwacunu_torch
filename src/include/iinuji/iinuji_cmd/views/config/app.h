#pragma once

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/views/config/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool handle_config_key(CmdState& st, int ch) {
  if (st.screen != ScreenMode::Config || !config_has_tabs(st)) return false;
  if (ch == KEY_DOWN) {
    select_next_tab(st);
    return true;
  }
  if (ch == KEY_UP) {
    select_prev_tab(st);
    return true;
  }
  return false;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
