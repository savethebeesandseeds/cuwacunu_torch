#pragma once

#include <string>

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/commands.h"
#include "iinuji/iinuji_cmd/commands/iinuji.state.flow.h"
#include "iinuji/iinuji_cmd/app/input.h"
#include "iinuji/iinuji_cmd/app/overlays.h"
#include "iinuji/iinuji_cmd/views/board/app.h"
#include "iinuji/iinuji_cmd/views/data/app.h"
#include "iinuji/iinuji_cmd/views/tsiemene/app.h"
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
    CmdState& state,
    int ch,
    DataAppRuntime& data_rt,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>& right,
    int v_scroll_step,
    int h_scroll_step,
    SetMouseCaptureFn&& set_mouse_capture) {
  const auto consume = [](bool dirty = true) {
    return AppKeyDispatchResult{true, dirty};
  };

  if (ch == KEY_MOUSE) {
    if (!state.logs.mouse_capture) return consume(false);
    MEVENT me{};
    if (getmouse(&me) != OK) return consume(false);

    const bool left_click = (me.bstate & (BUTTON1_PRESSED | BUTTON1_CLICKED |
                                          BUTTON1_DOUBLE_CLICKED | BUTTON1_TRIPLE_CLICKED)) != 0;
    if (left_click) {
      if (help_overlay_close_hit(state, left, right, me.x, me.y)) {
        dispatch_canonical_internal_call(state, canonical_paths::kHelpClose);
        log_info("[iinuji_cmd] help overlay closed (mouse)\n");
        return consume(true);
      }
      if (data_plot_overlay_close_hit(state, left, right, me.x, me.y)) {
        dispatch_canonical_internal_call(state, canonical_paths::kDataPlotOff);
        log_info("[iinuji_cmd] data plot overlay closed (mouse)\n");
        return consume(true);
      }
    }
    const bool mod_shift = (me.bstate & BUTTON_SHIFT) != 0;
    const bool mod_ctrl = (me.bstate & BUTTON_CTRL) != 0;
    const bool mod_alt = (me.bstate & BUTTON_ALT) != 0;
    const bool horizontal_mod = (mod_shift || mod_ctrl || mod_alt);

    int dy = 0;
    int dx = 0;
    bool wheel = false;

    if (state.help_view) {
      if (me.bstate & (BUTTON4_PRESSED | BUTTON4_CLICKED | BUTTON4_DOUBLE_CLICKED | BUTTON4_TRIPLE_CLICKED)) {
        wheel = true;
        if (horizontal_mod) dx -= h_scroll_step;
        else dy -= v_scroll_step;
      }
      if (me.bstate & (BUTTON5_PRESSED | BUTTON5_CLICKED | BUTTON5_DOUBLE_CLICKED | BUTTON5_TRIPLE_CLICKED)) {
        wheel = true;
        if (horizontal_mod) dx += h_scroll_step;
        else dy += v_scroll_step;
      }
      if (me.bstate & BUTTON6_PRESSED) {
        wheel = true;
        dx -= h_scroll_step;
      }
      if (me.bstate & BUTTON7_PRESSED) {
        wheel = true;
        dx += h_scroll_step;
      }
      if (wheel) {
        scroll_help_overlay(state, dy, dx);
        return consume(true);
      }
      return consume(false);
    }

    if (left_click) {
      if (logs_jump_top_hit(state, left, me.x, me.y)) {
        dispatch_canonical_internal_call(state, canonical_paths::kLogsScrollHome);
        return consume(true);
      }
      if (logs_jump_bottom_hit(state, left, me.x, me.y)) {
        dispatch_canonical_internal_call(state, canonical_paths::kLogsScrollEnd);
        return consume(true);
      }
    }

    const auto l_caps = panel_scroll_caps(left);
    const auto r_caps = panel_scroll_caps(right);
    const bool any_v = l_caps.v || r_caps.v;
    const bool any_h = l_caps.h || r_caps.h;

    const bool horizontal_auto = horizontal_mod || (!any_v && any_h);
    if (me.bstate & (BUTTON4_PRESSED | BUTTON4_CLICKED | BUTTON4_DOUBLE_CLICKED | BUTTON4_TRIPLE_CLICKED)) {
      wheel = true;
      if (horizontal_auto) dx -= h_scroll_step;
      else dy -= v_scroll_step;
    }
    if (me.bstate & (BUTTON5_PRESSED | BUTTON5_CLICKED | BUTTON5_DOUBLE_CLICKED | BUTTON5_TRIPLE_CLICKED)) {
      wheel = true;
      if (horizontal_auto) dx += h_scroll_step;
      else dy += v_scroll_step;
    }
    if (me.bstate & BUTTON6_PRESSED) {
      wheel = true;
      dx -= h_scroll_step;
    }
    if (me.bstate & BUTTON7_PRESSED) {
      wheel = true;
      dx += h_scroll_step;
    }
    if (wheel) {
      scroll_active_screen(state, left, right, dy, dx);
      return consume(true);
    }
    return consume(false);
  }

  if (ch == 27 && state.help_view) {
    dispatch_canonical_internal_call(state, canonical_paths::kHelpClose);
    log_info("[iinuji_cmd] help overlay closed (esc)\n");
    return consume(true);
  }
  if (handle_help_overlay_key(state, ch)) return consume(true);

  const bool is_cmdline_key =
      (ch == '\n' || ch == '\r' || ch == KEY_ENTER) ||
      (ch == 21) ||
      (ch == KEY_BACKSPACE || ch == 127 || ch == 8) ||
      (ch >= 32 && ch <= 126);
  if (state.help_view && !is_cmdline_key) return consume(false);

  if (ch == KEY_F(1)) {
    dispatch_canonical_internal_call(state, canonical_paths::kScreenHome);
    log_info("[iinuji_cmd] screen=home\n");
    return consume(true);
  }
  if (ch == KEY_F(2)) {
    dispatch_canonical_internal_call(state, canonical_paths::kScreenBoard);
    log_info("[iinuji_cmd] screen=board\n");
    return consume(true);
  }
  if (ch == KEY_F(3)) {
    dispatch_canonical_internal_call(state, canonical_paths::kScreenTraining);
    log_info("[iinuji_cmd] screen=training\n");
    return consume(true);
  }
  if (ch == KEY_F(4)) {
    dispatch_canonical_internal_call(state, canonical_paths::kScreenTsi);
    log_info("[iinuji_cmd] screen=tsi\n");
    return consume(true);
  }
  if (ch == KEY_F(5)) {
    dispatch_canonical_internal_call(state, canonical_paths::kScreenData);
    log_info("[iinuji_cmd] screen=data\n");
    return consume(true);
  }
  if (ch == KEY_F(8)) {
    dispatch_canonical_internal_call(state, canonical_paths::kScreenLogs);
    log_info("[iinuji_cmd] screen=logs\n");
    return consume(true);
  }
  if (ch == KEY_F(9)) {
    dispatch_canonical_internal_call(state, canonical_paths::kScreenConfig);
    log_info("[iinuji_cmd] screen=config\n");
    return consume(true);
  }

  if (!state.help_view && handle_board_navigation_key(state, ch, state.cmdline.empty())) {
    return consume(true);
  }

  if (!state.help_view && handle_board_editor_key(state, ch)) return consume(true);

  if (!state.help_view &&
      state.screen == ScreenMode::Tsiemene &&
      state.cmdline.empty() &&
      (ch == KEY_ENTER || ch == '\n' || ch == '\r') &&
      state.tsiemene.panel_focus == TsiPanelFocus::View) {
    const auto action = handle_tsi_view_enter_action(state);
    if (action.handled && !action.canonical_call.empty()) {
      dispatch_canonical_internal_call(state, action.canonical_call);
    }
    if (action.handled) return consume(true);
  }

  if (!state.help_view && handle_tsi_key(state, ch, state.cmdline.empty())) return consume(true);

  if (state.screen == ScreenMode::Training && ch == KEY_LEFT) {
    dispatch_canonical_internal_call(state, canonical_paths::kTrainingTabPrev);
    return consume(true);
  }
  if (state.screen == ScreenMode::Training && ch == KEY_RIGHT) {
    dispatch_canonical_internal_call(state, canonical_paths::kTrainingTabNext);
    return consume(true);
  }
  if (state.screen == ScreenMode::Training && ch == KEY_UP) {
    dispatch_canonical_internal_call(state, canonical_paths::kTrainingHashPrev);
    return consume(true);
  }
  if (state.screen == ScreenMode::Training && ch == KEY_DOWN) {
    dispatch_canonical_internal_call(state, canonical_paths::kTrainingHashNext);
    return consume(true);
  }
  if (state.screen == ScreenMode::Config && ch == KEY_DOWN) {
    dispatch_canonical_internal_call(state, canonical_paths::kConfigTabNext);
    return consume(true);
  }
  if (state.screen == ScreenMode::Config && ch == KEY_UP) {
    dispatch_canonical_internal_call(state, canonical_paths::kConfigTabPrev);
    return consume(true);
  }
  if (state.screen == ScreenMode::Logs && ch == KEY_UP) {
    dispatch_canonical_internal_call(state, canonical_paths::kLogsSettingsSelectPrev);
    return consume(true);
  }
  if (state.screen == ScreenMode::Logs && ch == KEY_DOWN) {
    dispatch_canonical_internal_call(state, canonical_paths::kLogsSettingsSelectNext);
    return consume(true);
  }
  if (state.screen == ScreenMode::Logs && (ch == KEY_LEFT || ch == KEY_RIGHT)) {
    const bool forward = (ch == KEY_RIGHT);
    if (dispatch_logs_setting_adjust(state, forward)) {
      set_mouse_capture(state.logs.mouse_capture);
      return consume(true);
    }
  }

  if (state.screen == ScreenMode::Logs && ch == KEY_HOME) {
    dispatch_canonical_internal_call(state, canonical_paths::kLogsScrollHome);
    return consume(true);
  }
  if (state.screen == ScreenMode::Logs && ch == KEY_END) {
    dispatch_canonical_internal_call(state, canonical_paths::kLogsScrollEnd);
    return consume(true);
  }
  if (state.screen == ScreenMode::Logs && ch == KEY_PPAGE) {
    dispatch_canonical_internal_call(state, canonical_paths::kLogsScrollPageUp);
    return consume(true);
  }
  if (state.screen == ScreenMode::Logs && ch == KEY_NPAGE) {
    dispatch_canonical_internal_call(state, canonical_paths::kLogsScrollPageDown);
    return consume(true);
  }
  if (state.screen == ScreenMode::Data && state.cmdline.empty() && ch == 27 && state.data.plot_view) {
    dispatch_canonical_internal_call(state, canonical_paths::kDataPlotOff);
    return consume(true);
  }
  if (state.screen == ScreenMode::Data && state.cmdline.empty() && ch == KEY_UP) {
    dispatch_canonical_internal_call(state, canonical_paths::kDataFocusPrev);
    return consume(true);
  }
  if (state.screen == ScreenMode::Data && state.cmdline.empty() && ch == KEY_DOWN) {
    dispatch_canonical_internal_call(state, canonical_paths::kDataFocusNext);
    return consume(true);
  }
  if (state.screen == ScreenMode::Data && state.cmdline.empty() && (ch == KEY_LEFT || ch == KEY_RIGHT)) {
    const bool forward = (ch == KEY_RIGHT);
    switch (state.data.nav_focus) {
      case DataNavFocus::Channel:
        dispatch_canonical_internal_call(state, forward ? canonical_paths::kDataChNext : canonical_paths::kDataChPrev);
        break;
      case DataNavFocus::Sample:
        dispatch_canonical_internal_call(state, forward ? canonical_paths::kDataSampleNext : canonical_paths::kDataSamplePrev);
        break;
      case DataNavFocus::Dim:
        dispatch_canonical_internal_call(state, forward ? canonical_paths::kDataDimNext : canonical_paths::kDataDimPrev);
        break;
      case DataNavFocus::PlotMode: {
        const auto next_mode = forward
            ? next_data_plot_mode(state.data.plot_mode)
            : prev_data_plot_mode(state.data.plot_mode);
        switch (next_mode) {
          case DataPlotMode::SeqLength:
            dispatch_canonical_internal_call(state, canonical_paths::kDataPlotModeSeq);
            break;
          case DataPlotMode::FutureSeqLength:
            dispatch_canonical_internal_call(state, canonical_paths::kDataPlotModeFuture);
            break;
          case DataPlotMode::ChannelWeight:
            dispatch_canonical_internal_call(state, canonical_paths::kDataPlotModeWeight);
            break;
          case DataPlotMode::NormWindow:
            dispatch_canonical_internal_call(state, canonical_paths::kDataPlotModeNorm);
            break;
          case DataPlotMode::FileBytes:
            dispatch_canonical_internal_call(state, canonical_paths::kDataPlotModeBytes);
            break;
        }
      } break;
      case DataNavFocus::XAxis:
        dispatch_canonical_internal_call(state, canonical_paths::kDataAxisToggle);
        break;
      case DataNavFocus::Mask:
        dispatch_canonical_internal_call(state, forward ? canonical_paths::kDataMaskOn : canonical_paths::kDataMaskOff);
        break;
    }
    return consume(true);
  }

  if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
    const std::string cmd = state.cmdline;
    state.cmdline.clear();
    if (state.help_view && !cmd.empty()) state.help_view = false;
    run_command(state, cmd, nullptr);
    set_mouse_capture(state.logs.mouse_capture);
    IinujiStateFlow{state}.normalize_after_command();
    init_data_runtime(state, data_rt, false);
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

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
