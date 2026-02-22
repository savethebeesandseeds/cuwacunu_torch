#pragma once

#include "iinuji/iinuji_cmd/views/data/view.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct IinujiStateFlow {
  CmdState& state;

  void reload_board() const {
    state.board = load_board_from_config();
    clamp_selected_circuit(state);
    state.data = load_data_view_from_config(&state.board);
    clamp_selected_data_channel(state);
    clamp_data_plot_mode(state);
    clamp_data_plot_x_axis(state);
    clamp_data_nav_focus(state);
    clamp_data_plot_feature_dim(state);
    clamp_data_plot_sample_index(state);
  }

  void reload_data() const {
    state.data = load_data_view_from_config(&state.board);
    clamp_selected_data_channel(state);
    clamp_data_plot_mode(state);
    clamp_data_plot_x_axis(state);
    clamp_data_nav_focus(state);
    clamp_data_plot_feature_dim(state);
    clamp_data_plot_sample_index(state);
  }

  void reload_config_and_board() const {
    cuwacunu::piaabo::dconfig::config_space_t::update_config();
    state.config = load_config_view_from_config();
    clamp_selected_tab(state);
    state.board = load_board_from_config();
    clamp_selected_circuit(state);
    state.data = load_data_view_from_config(&state.board);
    clamp_selected_data_channel(state);
    clamp_data_plot_mode(state);
    clamp_data_plot_x_axis(state);
    clamp_data_nav_focus(state);
    clamp_data_plot_feature_dim(state);
    clamp_data_plot_sample_index(state);
  }

  void normalize_after_command() const {
    clamp_selected_circuit(state);
    clamp_selected_tsi_tab(state);
    clamp_selected_data_channel(state);
    clamp_data_plot_mode(state);
    clamp_data_plot_x_axis(state);
    clamp_data_nav_focus(state);
    clamp_data_plot_feature_dim(state);
    clamp_data_plot_sample_index(state);
    clamp_selected_tab(state);
  }
};

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
