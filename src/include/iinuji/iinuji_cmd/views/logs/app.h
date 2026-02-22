#pragma once

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool handle_logs_key(CmdState& state, int ch) {
  if (state.screen != ScreenMode::Logs) return false;

  auto wrap_index = [](std::size_t value, int delta, std::size_t count) -> std::size_t {
    if (count == 0) return 0;
    const std::size_t step = (delta >= 0) ? 1u : (count - 1u);
    return (value + step) % count;
  };

  auto cycle_level = [&](bool forward) {
    constexpr std::size_t kLevels = 5;
    std::size_t idx = static_cast<std::size_t>(state.logs.level_filter);
    idx = forward ? ((idx + 1u) % kLevels) : ((idx + kLevels - 1u) % kLevels);
    state.logs.level_filter = static_cast<LogsLevelFilter>(idx);
  };

  switch (ch) {
    case KEY_UP:
      state.logs.selected_setting =
          wrap_index(state.logs.selected_setting, -1, logs_settings_count());
      return true;
    case KEY_DOWN:
      state.logs.selected_setting =
          wrap_index(state.logs.selected_setting, +1, logs_settings_count());
      return true;
    case KEY_LEFT:
    case KEY_RIGHT: {
      const bool forward = (ch == KEY_RIGHT);
      switch (state.logs.selected_setting) {
        case 0:
          cycle_level(forward);
          return true;
        case 1:
          state.logs.show_date = !state.logs.show_date;
          return true;
        case 2:
          state.logs.show_thread = !state.logs.show_thread;
          return true;
        case 3:
          state.logs.show_color = !state.logs.show_color;
          return true;
        case 4:
          state.logs.auto_follow = !state.logs.auto_follow;
          return true;
        case 5:
          state.logs.mouse_capture = !state.logs.mouse_capture;
          return true;
        default:
          return false;
      }
    }
    default:
      return false;
  }

  return false;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
