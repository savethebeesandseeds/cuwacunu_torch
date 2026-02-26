#pragma once

#include "iinuji/iinuji_cmd/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct IinujiScreen {
  CmdState& state;

  void home() const noexcept { state.screen = ScreenMode::Home; }
  void board() const noexcept { state.screen = ScreenMode::Board; }
  void training() const noexcept { state.screen = ScreenMode::Training; }
  void logs() const noexcept {
    const bool entering = (state.screen != ScreenMode::Logs);
    state.screen = ScreenMode::Logs;
    if (entering) {
      state.logs.pending_jump_end = true;
      state.logs.pending_jump_home = false;
      state.logs.auto_follow = true;
    }
  }
  void tsi() const noexcept {
    state.screen = ScreenMode::Tsiemene;
    state.tsiemene.panel_focus = TsiPanelFocus::Context;
  }
  void data() const noexcept { state.screen = ScreenMode::Data; }
  void config() const noexcept { state.screen = ScreenMode::Config; }
};

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
