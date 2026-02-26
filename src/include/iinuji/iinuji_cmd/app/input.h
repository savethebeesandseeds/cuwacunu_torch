#pragma once

#include <algorithm>
#include <array>
#include <string>
#include <string_view>

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/commands/iinuji.path.handlers.h"
#include "iinuji/iinuji_cmd/state.h"
#include "piaabo/dlogs.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool dispatch_canonical_internal_call(CmdState& state, std::string_view canonical_call) {
  IinujiPathHandlers handlers{state};
  auto push_ignore = [](const std::string&) {};
  auto append_ignore = [](const std::string&, const std::string&, const std::string&) {};
  auto push_err = [&](const std::string& msg) {
    log_err("[iinuji_cmd.internal] %s\n", msg.c_str());
  };
  return handlers.dispatch_canonical_text(
      std::string(canonical_call),
      push_ignore,
      push_ignore,
      push_err,
      append_ignore);
}

inline void scroll_help_overlay(CmdState& state, int dy, int dx) {
  if (!state.help_view) return;
  if (dy != 0) {
    const int y = state.help_scroll_y + dy;
    state.help_scroll_y = std::max(0, y);
  }
  if (dx != 0) {
    const int x = state.help_scroll_x + dx;
    state.help_scroll_x = std::max(0, x);
  }
}

inline bool handle_help_overlay_key(CmdState& state, int ch) {
  if (!state.help_view) return false;
  switch (ch) {
    case KEY_UP:
      dispatch_canonical_internal_call(state, canonical_paths::kHelpScrollUp);
      return true;
    case KEY_DOWN:
      dispatch_canonical_internal_call(state, canonical_paths::kHelpScrollDown);
      return true;
    case KEY_LEFT:
      dispatch_canonical_internal_call(state, canonical_paths::kHelpScrollLeft);
      return true;
    case KEY_RIGHT:
      dispatch_canonical_internal_call(state, canonical_paths::kHelpScrollRight);
      return true;
    case KEY_PPAGE:
      dispatch_canonical_internal_call(state, canonical_paths::kHelpScrollPageUp);
      return true;
    case KEY_NPAGE:
      dispatch_canonical_internal_call(state, canonical_paths::kHelpScrollPageDown);
      return true;
    case KEY_HOME:
      dispatch_canonical_internal_call(state, canonical_paths::kHelpScrollHome);
      return true;
    case KEY_END:
      dispatch_canonical_internal_call(state, canonical_paths::kHelpScrollEnd);
      return true;
    default:
      return false;
  }
}

inline bool dispatch_logs_setting_adjust(CmdState& state, bool forward) {
  if (state.screen != ScreenMode::Logs) return false;
  switch (state.logs.selected_setting) {
    case 0: {
      constexpr std::array<std::string_view, 5> kLevelCalls = {
          canonical_paths::kLogsSettingsLevelDebug,
          canonical_paths::kLogsSettingsLevelInfo,
          canonical_paths::kLogsSettingsLevelWarning,
          canonical_paths::kLogsSettingsLevelError,
          canonical_paths::kLogsSettingsLevelFatal,
      };
      std::size_t idx = static_cast<std::size_t>(state.logs.level_filter);
      idx = forward ? ((idx + 1u) % kLevelCalls.size()) : ((idx + kLevelCalls.size() - 1u) % kLevelCalls.size());
      return dispatch_canonical_internal_call(state, kLevelCalls[idx]);
    }
    case 1:
      return dispatch_canonical_internal_call(state, canonical_paths::kLogsSettingsDateToggle);
    case 2:
      return dispatch_canonical_internal_call(state, canonical_paths::kLogsSettingsThreadToggle);
    case 3:
      return dispatch_canonical_internal_call(state, canonical_paths::kLogsSettingsColorToggle);
    case 4:
      return dispatch_canonical_internal_call(state, canonical_paths::kLogsSettingsFollowToggle);
    case 5:
      return dispatch_canonical_internal_call(state, canonical_paths::kLogsSettingsMouseCaptureToggle);
    default:
      return false;
  }
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
