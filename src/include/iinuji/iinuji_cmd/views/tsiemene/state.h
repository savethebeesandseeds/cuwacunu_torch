#pragma once

#include <cstddef>
#include <cstdint>

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class TsiPanelFocus : std::uint8_t {
  Context = 0,
  View = 1,
};

struct TsiemeneState {
  std::size_t selected_tab{0};
  TsiPanelFocus panel_focus{TsiPanelFocus::Context};
  std::size_t view_cursor{0};
  std::size_t selected_source_dataloader{0};
};

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
