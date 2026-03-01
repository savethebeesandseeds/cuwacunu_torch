#pragma once

#include "iinuji/iinuji_cmd/views/data/view.h"
#include "iinuji/iinuji_cmd/views/tsiemene/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct IinujiStateFlow {
  CmdState& state;

  void reload_board() const {
    const auto contract_hash = state.board.contract_hash.empty()
                                   ? resolve_configured_board_contract_hash()
                                   : state.board.contract_hash;
    cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(
        contract_hash);
    state.board = load_board_from_contract_hash(contract_hash);
    clamp_board_navigation_state(state);
    clamp_selected_training_tab(state);
    clamp_selected_training_hash(state);
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
    clamp_selected_training_tab(state);
    clamp_selected_training_hash(state);
    clamp_selected_data_channel(state);
    clamp_data_plot_mode(state);
    clamp_data_plot_x_axis(state);
    clamp_data_nav_focus(state);
    clamp_data_plot_feature_dim(state);
    clamp_data_plot_sample_index(state);
  }

  void reload_config_and_board() const {
    cuwacunu::iitepi::config_space_t::update_config();
    cuwacunu::iitepi::contract_space_t::
        assert_registry_intact_or_fail_fast();
    const auto contract_hash = state.board.contract_hash.empty()
                                   ? resolve_configured_board_contract_hash()
                                   : state.board.contract_hash;
    state.config = load_config_view_from_config(contract_hash);
    clamp_selected_tab(state);
    state.board = load_board_from_contract_hash(contract_hash);
    clamp_board_navigation_state(state);
    clamp_selected_training_tab(state);
    clamp_selected_training_hash(state);
    state.data = load_data_view_from_config(&state.board);
    clamp_selected_data_channel(state);
    clamp_data_plot_mode(state);
    clamp_data_plot_x_axis(state);
    clamp_data_nav_focus(state);
    clamp_data_plot_feature_dim(state);
    clamp_data_plot_sample_index(state);
  }

  void normalize_after_command() const {
    clamp_board_navigation_state(state);
    clamp_selected_training_tab(state);
    clamp_selected_training_hash(state);
    clamp_selected_tsi_tab(state);
    clamp_tsi_navigation_state(state);
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
