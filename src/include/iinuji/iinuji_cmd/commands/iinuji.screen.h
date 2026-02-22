#pragma once

#include "iinuji/iinuji_cmd/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct IinujiScreen {
  CmdState& state;

  void home() const noexcept { state.screen = ScreenMode::Home; }
  void board() const noexcept { state.screen = ScreenMode::Board; }
  void logs() const noexcept { state.screen = ScreenMode::Logs; }
  void tsi() const noexcept { state.screen = ScreenMode::Tsiemene; }
  void data() const noexcept { state.screen = ScreenMode::Data; }
  void config() const noexcept { state.screen = ScreenMode::Config; }
};

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu

