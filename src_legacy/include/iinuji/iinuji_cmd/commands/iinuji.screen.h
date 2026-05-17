#pragma once

#include "iinuji/iinuji_cmd/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct IinujiScreen {
  CmdState &state;

  void home() const noexcept { state.screen = ScreenMode::Home; }
  void workbench() const noexcept { state.screen = ScreenMode::Workbench; }
  void runtime() const noexcept { state.screen = ScreenMode::Runtime; }
  void lattice() const noexcept { state.screen = ScreenMode::Lattice; }
  void logs() const noexcept {
    const bool entering = (state.screen != ScreenMode::ShellLogs);
    state.screen = ScreenMode::ShellLogs;
    if (entering) {
      state.shell_logs.pending_scroll_y = 0;
      state.shell_logs.pending_scroll_x = 0;
      state.shell_logs.pending_jump_end = true;
      state.shell_logs.pending_jump_home = false;
      state.shell_logs.auto_follow = true;
    }
  }
  void config() const noexcept { state.screen = ScreenMode::Config; }
};

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
