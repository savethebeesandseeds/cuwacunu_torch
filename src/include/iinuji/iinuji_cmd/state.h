#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "iinuji/iinuji_cmd/views/board/state.h"
#include "iinuji/iinuji_cmd/views/config/state.h"
#include "iinuji/iinuji_cmd/views/data/state.h"
#include "iinuji/iinuji_cmd/views/home/state.h"
#include "iinuji/iinuji_cmd/views/logs/state.h"
#include "iinuji/iinuji_cmd/views/tsiemene/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class ScreenMode : std::uint8_t {
  Home = 0,
  Board = 1,
  Logs = 2,
  Tsiemene = 3,
  Data = 4,
  Config = 5,
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
  LogsState logs{};
  TsiemeneState tsiemene{};
  DataState data{};
  ConfigState config{};
};

inline bool board_has_circuits(const CmdState& st) {
  return st.board.ok && !st.board.board.circuits.empty();
}

inline void clamp_selected_circuit(CmdState& st) {
  if (!board_has_circuits(st)) {
    st.board.selected_circuit = 0;
    return;
  }
  if (st.board.selected_circuit >= st.board.board.circuits.size()) st.board.selected_circuit = 0;
}

inline constexpr std::size_t tsiemene_implemented_nodes_count() {
  return 5;
}

inline void clamp_selected_tsi_tab(CmdState& st) {
  const std::size_t n = tsiemene_implemented_nodes_count();
  if (n == 0) {
    st.tsiemene.selected_tab = 0;
    return;
  }
  if (st.tsiemene.selected_tab >= n) st.tsiemene.selected_tab = 0;
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
