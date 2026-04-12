#pragma once

#include <limits>
#include <string>

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/app/input.h"
#include "iinuji/iinuji_cmd/app/overlays.h"
#include "iinuji/iinuji_cmd/commands.h"
#include "iinuji/iinuji_cmd/commands/iinuji.state.flow.h"
#include "iinuji/iinuji_cmd/views/config/app.h"
#include "iinuji/iinuji_cmd/views/human/app.h"
#include "iinuji/iinuji_cmd/views/lattice/app.h"
#include "iinuji/iinuji_cmd/views/logs/app.h"
#include "iinuji/iinuji_cmd/views/runtime/app.h"
#include "piaabo/dlogs.h"

#ifndef BUTTON_SHIFT
#define BUTTON_SHIFT 0
#endif
#ifndef BUTTON_CTRL
#define BUTTON_CTRL 0
#endif
#ifndef BUTTON_ALT
#define BUTTON_ALT 0
#endif
#ifndef BUTTON4_CLICKED
#define BUTTON4_CLICKED 0
#endif
#ifndef BUTTON1_CLICKED
#define BUTTON1_CLICKED 0
#endif
#ifndef BUTTON1_PRESSED
#define BUTTON1_PRESSED 0
#endif
#ifndef BUTTON1_DOUBLE_CLICKED
#define BUTTON1_DOUBLE_CLICKED 0
#endif
#ifndef BUTTON1_TRIPLE_CLICKED
#define BUTTON1_TRIPLE_CLICKED 0
#endif
#ifndef BUTTON5_CLICKED
#define BUTTON5_CLICKED 0
#endif
#ifndef BUTTON4_DOUBLE_CLICKED
#define BUTTON4_DOUBLE_CLICKED 0
#endif
#ifndef BUTTON5_DOUBLE_CLICKED
#define BUTTON5_DOUBLE_CLICKED 0
#endif
#ifndef BUTTON4_TRIPLE_CLICKED
#define BUTTON4_TRIPLE_CLICKED 0
#endif
#ifndef BUTTON5_TRIPLE_CLICKED
#define BUTTON5_TRIPLE_CLICKED 0
#endif
#ifndef BUTTON6_PRESSED
#define BUTTON6_PRESSED 0
#endif
#ifndef BUTTON7_PRESSED
#define BUTTON7_PRESSED 0
#endif

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct AppKeyDispatchResult {
  bool handled{false};
  bool dirty{false};
};

template <class SetMouseCaptureFn>
inline AppKeyDispatchResult dispatch_app_key(
    CmdState &state, int ch,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right,
    int v_scroll_step, int h_scroll_step,
    SetMouseCaptureFn &&set_mouse_capture) {
  const auto consume = [](bool dirty = true) {
    return AppKeyDispatchResult{true, dirty};
  };

  if (ch == KEY_MOUSE) {
    if (!state.shell_logs.mouse_capture)
      return consume(false);
    MEVENT me{};
    if (getmouse(&me) != OK)
      return consume(false);

    const bool left_click =
        (me.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED |
                      BUTTON1_DOUBLE_CLICKED | BUTTON1_TRIPLE_CLICKED)) != 0;
    if (left_click && help_overlay_close_hit(state, left, right, me.x, me.y)) {
      dispatch_canonical_internal_call(state, canonical_paths::kHelpClose);
      return consume(true);
    }

    if (left_click) {
      if (const auto jump = editor_jump_hint_target(left, me.x, me.y);
          jump.has_value() && apply_editor_jump_hint(state, *jump)) {
        return consume(true);
      }
      if (const auto jump = editor_jump_hint_target(right, me.x, me.y);
          jump.has_value() && apply_editor_jump_hint(state, *jump)) {
        return consume(true);
      }
    }

    if (state.help_view) {
      int dy = 0;
      int dx = 0;
      const bool horizontal_mod = ((me.bstate & BUTTON_SHIFT) != 0) ||
                                  ((me.bstate & BUTTON_CTRL) != 0) ||
                                  ((me.bstate & BUTTON_ALT) != 0);
      if (me.bstate & (BUTTON4_PRESSED | BUTTON4_CLICKED |
                       BUTTON4_DOUBLE_CLICKED | BUTTON4_TRIPLE_CLICKED)) {
        if (horizontal_mod)
          dx -= h_scroll_step;
        else
          dy -= v_scroll_step;
      }
      if (me.bstate & (BUTTON5_PRESSED | BUTTON5_CLICKED |
                       BUTTON5_DOUBLE_CLICKED | BUTTON5_TRIPLE_CLICKED)) {
        if (horizontal_mod)
          dx += h_scroll_step;
        else
          dy += v_scroll_step;
      }
      if (dy != 0 || dx != 0) {
        scroll_help_overlay(state, dy, dx);
        return consume(true);
      }
      return consume(false);
    }

    if (left_click) {
      if (workspace_zoom_button_hit(state, left, right, me.x, me.y)) {
        if (workspace_toggle_current_screen_zoom(state) &&
            state.screen == ScreenMode::Runtime) {
          set_runtime_status(state,
                             workspace_is_current_screen_zoomed(state)
                                 ? "Expanded runtime detail panel."
                                 : "Restored split runtime panel layout.",
                             false);
        }
        return consume(true);
      }
      if (logs_jump_top_hit(state, left, me.x, me.y)) {
        dispatch_canonical_internal_call(state,
                                         canonical_paths::kLogsScrollHome);
        return consume(true);
      }
      if (logs_jump_bottom_hit(state, left, me.x, me.y)) {
        dispatch_canonical_internal_call(state,
                                         canonical_paths::kLogsScrollEnd);
        return consume(true);
      }
    }

    const auto l_caps = panel_scroll_caps(left);
    const auto r_caps = panel_scroll_caps(right);
    const bool any_v = l_caps.v || r_caps.v;
    const bool any_h = l_caps.h || r_caps.h;
    const bool horizontal_auto =
        ((me.bstate & BUTTON_SHIFT) != 0) || ((me.bstate & BUTTON_CTRL) != 0) ||
        ((me.bstate & BUTTON_ALT) != 0) || (!any_v && any_h);

    int dy = 0;
    int dx = 0;
    if (me.bstate & (BUTTON4_PRESSED | BUTTON4_CLICKED |
                     BUTTON4_DOUBLE_CLICKED | BUTTON4_TRIPLE_CLICKED)) {
      if (horizontal_auto)
        dx -= h_scroll_step;
      else
        dy -= v_scroll_step;
    }
    if (me.bstate & (BUTTON5_PRESSED | BUTTON5_CLICKED |
                     BUTTON5_DOUBLE_CLICKED | BUTTON5_TRIPLE_CLICKED)) {
      if (horizontal_auto)
        dx += h_scroll_step;
      else
        dy += v_scroll_step;
    }
    if (me.bstate & BUTTON6_PRESSED)
      dx -= h_scroll_step;
    if (me.bstate & BUTTON7_PRESSED)
      dx += h_scroll_step;
    if (dy != 0 || dx != 0) {
      scroll_active_screen(state, left, right, dy, dx);
      return consume(true);
    }
    return consume(false);
  }

  if (ch == 27 && state.help_view) {
    dispatch_canonical_internal_call(state, canonical_paths::kHelpClose);
    return consume(true);
  }
  if (handle_help_overlay_key(state, ch))
    return consume(true);

  const bool is_cmdline_key =
      (ch == '\n' || ch == '\r' || ch == KEY_ENTER) || (ch == 21) ||
      (ch == KEY_BACKSPACE || ch == 127 || ch == 8) || (ch >= 32 && ch <= 126);
  if (state.help_view && !is_cmdline_key)
    return consume(false);

  if (ch == KEY_F(1)) {
    dispatch_canonical_internal_call(state, canonical_paths::kScreenHome);
    return consume(true);
  }
  if (ch == KEY_F(2)) {
    dispatch_canonical_internal_call(state, canonical_paths::kScreenHuman);
    return consume(true);
  }
  if (ch == KEY_F(3)) {
    dispatch_canonical_internal_call(state, canonical_paths::kScreenRuntime);
    return consume(true);
  }
  if (ch == KEY_F(4)) {
    dispatch_canonical_internal_call(state, canonical_paths::kScreenLattice);
    return consume(true);
  }
  if (ch == KEY_F(8)) {
    dispatch_canonical_internal_call(state, canonical_paths::kScreenLogs);
    return consume(true);
  }
  if (ch == KEY_F(9)) {
    dispatch_canonical_internal_call(state, canonical_paths::kScreenConfig);
    return consume(true);
  }

  if (!state.help_view && state.cmdline.empty()) {
    if (handle_human_key(state, ch))
      return consume(true);
    if (handle_runtime_key(state, ch))
      return consume(true);
    if (handle_lattice_key(state, ch))
      return consume(true);
    if (handle_config_key(state, ch))
      return consume(true);
    if (handle_logs_key(state, ch)) {
      set_mouse_capture(state.shell_logs.mouse_capture);
      return consume(true);
    }

    auto right_tb =
        as<cuwacunu::iinuji::textBox_data_t>(find_visible_text_box(right));
    if ((state.screen == ScreenMode::Human ||
         state.screen == ScreenMode::Runtime ||
         state.screen == ScreenMode::Lattice) &&
        right_tb != nullptr) {
      if (ch == KEY_PPAGE) {
        right_tb->scroll_by(-12, 0);
        return consume(true);
      }
      if (ch == KEY_NPAGE) {
        right_tb->scroll_by(+12, 0);
        return consume(true);
      }
      if (ch == KEY_HOME) {
        right_tb->scroll_y = 0;
        return consume(true);
      }
      if (ch == KEY_END) {
        right_tb->scroll_y = std::numeric_limits<int>::max();
        return consume(true);
      }
    }

    if (state.screen == ScreenMode::ShellLogs) {
      if (ch == KEY_HOME) {
        dispatch_canonical_internal_call(state,
                                         canonical_paths::kLogsScrollHome);
        return consume(true);
      }
      if (ch == KEY_END) {
        dispatch_canonical_internal_call(state,
                                         canonical_paths::kLogsScrollEnd);
        return consume(true);
      }
      if (ch == KEY_PPAGE) {
        dispatch_canonical_internal_call(state,
                                         canonical_paths::kLogsScrollPageUp);
        return consume(true);
      }
      if (ch == KEY_NPAGE) {
        dispatch_canonical_internal_call(state,
                                         canonical_paths::kLogsScrollPageDown);
        return consume(true);
      }
    }
  }

  if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
    const std::string cmd = state.cmdline;
    state.cmdline.clear();
    if (state.help_view && !cmd.empty())
      state.help_view = false;
    run_command(state, cmd, nullptr);
    set_mouse_capture(state.shell_logs.mouse_capture);
    IinujiStateFlow{state}.normalize_after_command();
    return consume(true);
  }

  if (ch == 21) {
    state.cmdline.clear();
    return consume(true);
  }

  if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
    if (!state.cmdline.empty()) {
      state.cmdline.pop_back();
      return consume(true);
    }
    return consume(false);
  }

  if (ch >= 32 && ch <= 126) {
    state.cmdline.push_back(static_cast<char>(ch));
    return consume(true);
  }

  return {};
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
