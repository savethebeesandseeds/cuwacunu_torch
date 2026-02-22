#pragma once

#include <random>
#include <string>

#include "iinuji/iinuji_cmd/commands/iinuji.path.tokens.h"
#include "iinuji/iinuji_cmd/views/data/view.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void select_next_data_channel(CmdState& st) {
  if (!data_has_channels(st)) {
    st.data.selected_channel = 0;
    return;
  }
  st.data.selected_channel = (st.data.selected_channel + 1) % st.data.channels.size();
}

inline void select_prev_data_channel(CmdState& st) {
  if (!data_has_channels(st)) {
    st.data.selected_channel = 0;
    return;
  }
  st.data.selected_channel =
      (st.data.selected_channel + st.data.channels.size() - 1) % st.data.channels.size();
}

inline void select_next_data_sample(CmdState& st) {
  if (st.data.plot_sample_count == 0) {
    st.data.plot_sample_index = 0;
    return;
  }
  st.data.plot_sample_index = (st.data.plot_sample_index + 1) % st.data.plot_sample_count;
}

inline void select_prev_data_sample(CmdState& st) {
  if (st.data.plot_sample_count == 0) {
    st.data.plot_sample_index = 0;
    return;
  }
  st.data.plot_sample_index =
      (st.data.plot_sample_index + st.data.plot_sample_count - 1) % st.data.plot_sample_count;
}

inline void select_random_data_sample(CmdState& st) {
  if (st.data.plot_sample_count == 0) {
    st.data.plot_sample_index = 0;
    return;
  }
  static thread_local std::mt19937 rng(std::random_device{}());
  std::uniform_int_distribution<std::size_t> dist(0, st.data.plot_sample_count - 1);
  st.data.plot_sample_index = dist(rng);
}

inline void select_next_data_dim(CmdState& st) {
  if (st.data.plot_D == 0) {
    st.data.plot_feature_dim = 0;
    return;
  }
  st.data.plot_feature_dim = (st.data.plot_feature_dim + 1) % st.data.plot_D;
}

inline void select_prev_data_dim(CmdState& st) {
  if (st.data.plot_D == 0) {
    st.data.plot_feature_dim = 0;
    return;
  }
  st.data.plot_feature_dim =
      (st.data.plot_feature_dim + st.data.plot_D - 1) % st.data.plot_D;
}

inline bool select_data_dim_by_token(CmdState& st, const std::string& token) {
  if (st.data.plot_D == 0) return false;
  std::size_t idx1 = 0;
  if (parse_positive_index(token, &idx1)) {
    if (idx1 == 0 || idx1 > st.data.plot_D) return false;
    st.data.plot_feature_dim = idx1 - 1;
    return true;
  }

  if (!st.data.channels.empty()) {
    const auto& c =
        st.data.channels[std::min(st.data.selected_channel, st.data.channels.size() - 1)];
    const auto& names = data_feature_names_for_record_type(c.record_type);
    const std::string needle = to_lower_copy(token);
    for (std::size_t i = 0; i < names.size(); ++i) {
      if (to_lower_copy(names[i]) == needle ||
          canonical_path_tokens::token_matches(names[i], token)) {
        if (i >= st.data.plot_D) return false;
        st.data.plot_feature_dim = i;
        return true;
      }
    }
  }

  return false;
}

template <class AppendLog>
inline bool handle_data_show(CmdState& st, AppendLog&& append_log) {
  append_log(
      "focus=" + (st.data.focus_instrument.empty() ? std::string("<none>") : st.data.focus_instrument),
      "show",
      "#d8d8ff");
  append_log(
      "channels=" + std::to_string(st.data.channels.size()) +
          " batch=" + std::to_string(st.data.batch_size),
      "show",
      "#d8d8ff");
  append_log(
      "plotview=" + std::string(st.data.plot_view ? "on" : "off") +
          " mode=" + data_plot_mode_token(st.data.plot_mode) +
          " x=" + data_plot_x_axis_token(st.data.plot_x_axis),
      "show",
      "#d8d8ff");
  append_log(
      "sample=" + std::to_string(st.data.plot_sample_index + 1) + "/" +
          std::to_string(st.data.plot_sample_count) +
          " dim=" + std::to_string(st.data.plot_feature_dim + 1) + "/" +
          std::to_string(st.data.plot_D) +
          " mask=" + std::string(st.data.plot_mask_overlay ? "on" : "off") +
          " focus=" + data_nav_focus_name(st.data.nav_focus),
      "show",
      "#d8d8ff");
  append_log(
      "shape=[B,C,T,D+1] => [" + std::to_string(st.data.batch_size) + "," +
          std::to_string(st.data.active_channels) + "," +
          std::to_string(st.data.max_seq_length) + "," +
          std::to_string(st.data.feature_dims > 0 ? (st.data.feature_dims + 1) : 0) + "]",
      "show",
      "#d8d8ff");
  return true;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
