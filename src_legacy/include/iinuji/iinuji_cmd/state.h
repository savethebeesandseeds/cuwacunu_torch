#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>

#include "iinuji/iinuji_cmd/views/config/state.h"
#include "iinuji/iinuji_cmd/views/workbench/state.h"
#include "iinuji/iinuji_cmd/views/lattice/state.h"
#include "iinuji/iinuji_cmd/views/logs/state.h"
#include "iinuji/iinuji_cmd/views/runtime/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class ScreenMode : std::uint8_t {
  Home = 0,
  Workbench = 1,
  Runtime = 2,
  Lattice = 3,
  ShellLogs = 4,
  Config = 5,
};

enum class WorkspaceZoomSlot : std::uint8_t {
  None = 0,
  RuntimeDetail = 1,
  LogsStream = 2,
};

struct WorkspaceState {
  std::uint32_t zoom_flags{0};
};

inline constexpr std::uint32_t workspace_zoom_bit(WorkspaceZoomSlot slot) {
  if (slot == WorkspaceZoomSlot::None)
    return 0u;
  return 1u << (static_cast<std::uint32_t>(slot) - 1u);
}

inline WorkspaceZoomSlot workspace_zoom_slot_for_screen(ScreenMode screen) {
  switch (screen) {
  case ScreenMode::Runtime:
    return WorkspaceZoomSlot::RuntimeDetail;
  case ScreenMode::ShellLogs:
    return WorkspaceZoomSlot::LogsStream;
  case ScreenMode::Home:
  case ScreenMode::Workbench:
  case ScreenMode::Lattice:
  case ScreenMode::Config:
    break;
  }
  return WorkspaceZoomSlot::None;
}

inline bool workspace_zoom_slot_uses_left_panel(WorkspaceZoomSlot slot) {
  return slot == WorkspaceZoomSlot::LogsStream;
}

inline bool workspace_screen_supports_zoom(ScreenMode screen) {
  return workspace_zoom_slot_for_screen(screen) != WorkspaceZoomSlot::None;
}

struct CmdState {
  ScreenMode screen{ScreenMode::Home};
  bool running{true};
  std::string cmdline{};
  bool help_view{false};
  int help_scroll_y{0};
  int help_scroll_x{0};
  WorkspaceState workspace{};

  WorkbenchState workbench{};
  RuntimeState runtime{};
  LatticeState lattice{};
  ShellLogsState shell_logs{};
  ConfigState config{};
  std::function<void()> repaint_now{};
};

inline bool workspace_zoom_slot_enabled(const CmdState &st,
                                        WorkspaceZoomSlot slot) {
  const std::uint32_t bit = workspace_zoom_bit(slot);
  return bit != 0u && (st.workspace.zoom_flags & bit) != 0u;
}

inline bool workspace_is_current_screen_zoomed(const CmdState &st) {
  return workspace_zoom_slot_enabled(st,
                                     workspace_zoom_slot_for_screen(st.screen));
}

inline bool workspace_current_screen_uses_left_panel_zoom(const CmdState &st) {
  return workspace_zoom_slot_uses_left_panel(
      workspace_zoom_slot_for_screen(st.screen));
}

inline bool workspace_toggle_current_screen_zoom(CmdState &st) {
  const WorkspaceZoomSlot slot = workspace_zoom_slot_for_screen(st.screen);
  const std::uint32_t bit = workspace_zoom_bit(slot);
  if (bit == 0u)
    return false;
  st.workspace.zoom_flags ^= bit;
  return true;
}

inline bool workspace_restore_current_screen_split(CmdState &st) {
  const WorkspaceZoomSlot slot = workspace_zoom_slot_for_screen(st.screen);
  const std::uint32_t bit = workspace_zoom_bit(slot);
  if (bit == 0u || (st.workspace.zoom_flags & bit) == 0u)
    return false;
  st.workspace.zoom_flags &= ~bit;
  return true;
}

inline bool config_has_files(const CmdState &st) {
  return st.config.ok && !st.config.files.empty();
}

inline void clamp_selected_config_file(CmdState &st) {
  if (!config_has_files(st)) {
    st.config.selected_file = 0;
    st.config.selected_family = ConfigFileFamily::Defaults;
    return;
  }
  if (st.config.selected_file >= st.config.files.size())
    st.config.selected_file = 0;
  st.config.selected_family = st.config.files[st.config.selected_file].family;
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
