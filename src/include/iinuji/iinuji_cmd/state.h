#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "hashimyei/hashimyei_identity.h"
#include "iinuji/iinuji_cmd/catalog.h"
#include "iinuji/iinuji_cmd/views/board/state.h"
#include "iinuji/iinuji_cmd/views/config/state.h"
#include "iinuji/iinuji_cmd/views/data/state.h"
#include "iinuji/iinuji_cmd/views/home/state.h"
#include "iinuji/iinuji_cmd/views/logs/state.h"
#include "iinuji/iinuji_cmd/views/training/state.h"
#include "iinuji/iinuji_cmd/views/tsiemene/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class ScreenMode : std::uint8_t {
  Home = 0,
  Board = 1,
  Training = 2,
  Logs = 3,
  Tsiemene = 4,
  Data = 5,
  Config = 6,
};

struct CmdState {
  ScreenMode screen{ScreenMode::Home};
  bool running{true};
  std::string cmdline{};
  bool help_view{false};
  int help_scroll_y{0};
  int help_scroll_x{0};

  HomeState home{};
  BoardState board{};
  TrainingState training{};
  LogsState logs{};
  TsiemeneState tsiemene{};
  DataState data{};
  ConfigState config{};
};

inline bool board_has_circuits(const CmdState& st) {
  return st.board.ok && !st.board.board.contracts.empty();
}

inline void clamp_selected_circuit(CmdState& st) {
  if (!board_has_circuits(st)) {
    st.board.selected_circuit = 0;
    st.board.editing_contract_index = 0;
    return;
  }
  if (st.board.selected_circuit >= st.board.board.contracts.size()) st.board.selected_circuit = 0;
  if (st.board.editing_contract_index >= st.board.board.contracts.size()) {
    st.board.editing_contract_index = st.board.selected_circuit;
  }
}

inline constexpr std::size_t board_view_option_count() {
  return 2;
}

inline constexpr std::size_t board_contract_section_count() {
  return 4;
}

inline void clamp_board_navigation_state(CmdState& st) {
  clamp_selected_circuit(st);
  if (st.board.panel_focus != BoardPanelFocus::Context &&
      st.board.panel_focus != BoardPanelFocus::ViewOptions &&
      st.board.panel_focus != BoardPanelFocus::ContractSections) {
    st.board.panel_focus = BoardPanelFocus::Context;
  }
  if (st.board.display_mode != BoardDisplayMode::Diagram &&
      st.board.display_mode != BoardDisplayMode::ContractTextEdit) {
    st.board.display_mode = BoardDisplayMode::Diagram;
  }
  const std::size_t n = board_view_option_count();
  if (n == 0) {
    st.board.selected_view_option = 0;
  } else if (st.board.selected_view_option >= n) {
    st.board.selected_view_option = 0;
  }
  const std::size_t sn = board_contract_section_count();
  if (sn == 0) {
    st.board.selected_contract_section = 0;
  } else if (st.board.selected_contract_section >= sn) {
    st.board.selected_contract_section = 0;
  }
  if (st.board.display_mode != BoardDisplayMode::ContractTextEdit &&
      st.board.panel_focus == BoardPanelFocus::ContractSections) {
    st.board.panel_focus = BoardPanelFocus::ViewOptions;
  }
}

inline void clamp_selected_tsi_tab(CmdState& st) {
  const std::size_t n = tsi_tab_count();
  if (n == 0) {
    st.tsiemene.selected_tab = 0;
    st.tsiemene.panel_focus = TsiPanelFocus::Context;
    st.tsiemene.view_cursor = 0;
    st.tsiemene.selected_source_dataloader = 0;
    return;
  }
  if (st.tsiemene.selected_tab >= n) st.tsiemene.selected_tab = 0;
}

inline std::size_t training_known_hashimyei_count() {
  return cuwacunu::hashimyei::known_hashimyeis().size();
}

inline void clamp_selected_training_tab(CmdState& st) {
  const std::size_t n = training_wikimyei_count();
  if (n == 0) {
    st.training.selected_tab = 0;
    return;
  }
  if (st.training.selected_tab >= n) st.training.selected_tab = 0;
}

inline void clamp_selected_training_hash(CmdState& st) {
  const std::size_t n = training_known_hashimyei_count();
  if (n == 0) {
    st.training.selected_hash = 0;
    return;
  }
  if (st.training.selected_hash >= n) st.training.selected_hash = 0;
}

inline bool config_has_tabs(const CmdState& st) {
  return st.config.ok && !st.config.tabs.empty();
}

inline void clamp_selected_tab(CmdState& st) {
  if (!config_has_tabs(st)) {
    st.config.selected_tab = 0;
    return;
  }
  if (st.config.selected_tab >= st.config.tabs.size()) st.config.selected_tab = 0;
}

inline bool data_has_channels(const CmdState& st) {
  return st.data.ok && !st.data.channels.empty();
}

inline constexpr std::size_t data_plot_mode_count() {
  return 5;
}

inline constexpr std::size_t data_plot_x_axis_count() {
  return 2;
}

inline constexpr std::size_t data_nav_focus_count() {
  return 6;
}

inline void clamp_selected_data_channel(CmdState& st) {
  if (!data_has_channels(st)) {
    st.data.selected_channel = 0;
    return;
  }
  if (st.data.selected_channel >= st.data.channels.size()) st.data.selected_channel = 0;
}

inline void clamp_data_plot_mode(CmdState& st) {
  const std::size_t n = data_plot_mode_count();
  if (n == 0) {
    st.data.plot_mode = DataPlotMode::SeqLength;
    return;
  }
  const auto idx = static_cast<std::size_t>(st.data.plot_mode);
  if (idx >= n) st.data.plot_mode = DataPlotMode::SeqLength;
}

inline void clamp_data_plot_x_axis(CmdState& st) {
  const std::size_t n = data_plot_x_axis_count();
  if (n == 0) {
    st.data.plot_x_axis = DataPlotXAxis::Index;
    return;
  }
  const auto idx = static_cast<std::size_t>(st.data.plot_x_axis);
  if (idx >= n) st.data.plot_x_axis = DataPlotXAxis::Index;
}

inline void clamp_data_nav_focus(CmdState& st) {
  const std::size_t n = data_nav_focus_count();
  if (n == 0) {
    st.data.nav_focus = DataNavFocus::Channel;
    return;
  }
  const auto idx = static_cast<std::size_t>(st.data.nav_focus);
  if (idx >= n) st.data.nav_focus = DataNavFocus::Channel;
}

inline void clamp_data_plot_feature_dim(CmdState& st) {
  if (st.data.plot_D == 0) {
    st.data.plot_feature_dim = 0;
    return;
  }
  if (st.data.plot_feature_dim >= st.data.plot_D) st.data.plot_feature_dim = 0;
}

inline void clamp_data_plot_sample_index(CmdState& st) {
  if (st.data.plot_sample_count == 0) {
    st.data.plot_sample_index = 0;
    return;
  }
  if (st.data.plot_sample_index >= st.data.plot_sample_count) st.data.plot_sample_index = 0;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
