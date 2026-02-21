#pragma once

#include <random>
#include <sstream>
#include <string>

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

inline bool select_data_channel_by_token(CmdState& st, const std::string& token) {
  if (!data_has_channels(st)) return false;
  std::size_t idx1 = 0;
  if (parse_positive_index(token, &idx1)) {
    if (idx1 == 0 || idx1 > st.data.channels.size()) return false;
    st.data.selected_channel = idx1 - 1;
    return true;
  }
  return false;
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

inline bool select_data_sample_by_token(CmdState& st, const std::string& token) {
  if (st.data.plot_sample_count == 0) return false;
  std::size_t idx1 = 0;
  if (!parse_positive_index(token, &idx1)) return false;
  if (idx1 == 0 || idx1 > st.data.plot_sample_count) return false;
  st.data.plot_sample_index = idx1 - 1;
  return true;
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
      if (to_lower_copy(names[i]) == needle) {
        if (i >= st.data.plot_D) return false;
        st.data.plot_feature_dim = i;
        return true;
      }
    }
  }

  return false;
}

template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
inline bool handle_data_command(CmdState& st,
                                const std::string& command,
                                std::istringstream& iss,
                                PushInfo&& push_info,
                                PushWarn&& push_warn,
                                PushErr&& push_err,
                                AppendLog&& append_log) {
  if (!(command == "data" || command == "f5")) return false;

  std::string arg0;
  iss >> arg0;
  arg0 = to_lower_copy(arg0);
  if (arg0.empty()) {
    st.screen = ScreenMode::Data;
    push_info("screen=data");
    return true;
  }
  if (arg0 == "reload") {
    st.data = load_data_view_from_config(&st.board);
    clamp_selected_data_channel(st);
    clamp_data_plot_mode(st);
    clamp_data_plot_x_axis(st);
    clamp_data_nav_focus(st);
    clamp_data_plot_feature_dim(st);
    clamp_data_plot_sample_index(st);
    st.screen = ScreenMode::Data;
    if (st.data.ok) push_info("data reloaded");
    else push_err("reload data failed: " + st.data.error);
    return true;
  }
  if (arg0 == "plotview") {
    std::string arg1;
    iss >> arg1;
    arg1 = to_lower_copy(arg1);
    if (arg1.empty() || arg1 == "toggle") {
      st.data.plot_view = !st.data.plot_view;
    } else if (arg1 == "on") {
      st.data.plot_view = true;
    } else if (arg1 == "off") {
      st.data.plot_view = false;
    } else {
      push_err("usage: data plotview on|off|toggle");
      return true;
    }
    st.screen = ScreenMode::Data;
    push_info(std::string("data plotview=") + (st.data.plot_view ? "on" : "off"));
    return true;
  }
  if (arg0 == "plot") {
    std::string arg1;
    iss >> arg1;
    arg1 = to_lower_copy(arg1);
    if (arg1.empty() || arg1 == "on" || arg1 == "open" || arg1 == "view") {
      st.data.plot_view = true;
      st.screen = ScreenMode::Data;
      push_info("data plotview=on");
      return true;
    }
    if (arg1 == "off") {
      st.data.plot_view = false;
      st.screen = ScreenMode::Data;
      push_info("data plotview=off");
      return true;
    }
    if (arg1 == "toggle") {
      st.data.plot_view = !st.data.plot_view;
      st.screen = ScreenMode::Data;
      push_info(std::string("data plotview=") + (st.data.plot_view ? "on" : "off"));
      return true;
    }
    const auto mode = parse_data_plot_mode_token(arg1);
    if (!mode.has_value()) {
      push_err("usage: data plot [on|off|toggle|seq|future|weight|norm|bytes]");
      return true;
    }
    st.data.plot_mode = *mode;
    clamp_data_plot_mode(st);
    st.screen = ScreenMode::Data;
    push_info("data plot=" + data_plot_mode_token(st.data.plot_mode));
    return true;
  }
  if (arg0 == "x" || arg0 == "xaxis" || arg0 == "axis") {
    std::string arg1;
    iss >> arg1;
    arg1 = to_lower_copy(arg1);
    if (arg1.empty() || arg1 == "toggle") {
      st.data.plot_x_axis = next_data_plot_x_axis(st.data.plot_x_axis);
    } else {
      const auto axis = parse_data_plot_x_axis_token(arg1);
      if (!axis.has_value()) {
        push_err("usage: data x idx|key|toggle");
        return true;
      }
      st.data.plot_x_axis = *axis;
    }
    clamp_data_plot_x_axis(st);
    st.screen = ScreenMode::Data;
    push_info("data x=" + data_plot_x_axis_token(st.data.plot_x_axis));
    return true;
  }
  if (arg0 == "ch" || arg0 == "channel") {
    std::string arg1;
    iss >> arg1;
    arg1 = to_lower_copy(arg1);
    if (arg1.empty()) {
      push_err("usage: data ch next|prev|N");
      return true;
    }
    if (!data_has_channels(st)) {
      push_warn("no data channels");
      return true;
    }
    if (arg1 == "next") {
      select_next_data_channel(st);
    } else if (arg1 == "prev") {
      select_prev_data_channel(st);
    } else if (!select_data_channel_by_token(st, arg1)) {
      push_err("data channel not found");
      return true;
    }
    st.screen = ScreenMode::Data;
    push_info("selected data channel=" + std::to_string(st.data.selected_channel + 1));
    return true;
  }
  if (arg0 == "sample" || arg0 == "seq") {
    std::string arg1;
    iss >> arg1;
    arg1 = to_lower_copy(arg1);
    if (arg1.empty()) {
      push_err("usage: data sample next|prev|random|N");
      return true;
    }
    if (st.data.plot_sample_count == 0) {
      push_warn("no data samples loaded");
      return true;
    }
    if (arg1 == "next") {
      select_next_data_sample(st);
    } else if (arg1 == "prev") {
      select_prev_data_sample(st);
    } else if (arg1 == "random" || arg1 == "rand") {
      select_random_data_sample(st);
    } else if (!select_data_sample_by_token(st, arg1)) {
      push_err("sample out of range");
      return true;
    }
    st.screen = ScreenMode::Data;
    push_info("selected data sample=" + std::to_string(st.data.plot_sample_index + 1));
    return true;
  }
  if (arg0 == "dim" || arg0 == "feature") {
    std::string arg1;
    iss >> arg1;
    arg1 = to_lower_copy(arg1);
    if (arg1.empty()) {
      push_err("usage: data dim next|prev|N|<name>");
      return true;
    }
    if (st.data.plot_D == 0) {
      push_warn("no tensor dims available");
      return true;
    }
    if (arg1 == "next") {
      select_next_data_dim(st);
    } else if (arg1 == "prev") {
      select_prev_data_dim(st);
    } else if (!select_data_dim_by_token(st, arg1)) {
      push_err("dim out of range");
      return true;
    }
    st.screen = ScreenMode::Data;
    push_info("selected data dim=" + std::to_string(st.data.plot_feature_dim + 1));
    return true;
  }
  if (arg0 == "mask") {
    std::string arg1;
    iss >> arg1;
    arg1 = to_lower_copy(arg1);
    if (arg1.empty() || arg1 == "toggle") {
      st.data.plot_mask_overlay = !st.data.plot_mask_overlay;
    } else if (arg1 == "on") {
      st.data.plot_mask_overlay = true;
    } else if (arg1 == "off") {
      st.data.plot_mask_overlay = false;
    } else {
      push_err("usage: data mask on|off|toggle");
      return true;
    }
    st.screen = ScreenMode::Data;
    push_info(std::string("data mask=") + (st.data.plot_mask_overlay ? "on" : "off"));
    return true;
  }
  if (arg0 == "channels") {
    if (!data_has_channels(st)) {
      push_warn("no data channels");
      return true;
    }
    for (std::size_t i = 0; i < st.data.channels.size(); ++i) {
      const auto& c = st.data.channels[i];
      append_log(
          "[" + std::to_string(i + 1) + "] " + c.interval + "/" + c.record_type +
              " seq=" + std::to_string(c.seq_length) +
              " fut=" + std::to_string(c.future_seq_length),
          "data.channels",
          "#d0d0d0");
    }
    st.screen = ScreenMode::Data;
    return true;
  }

  push_err("usage: data | data reload | data plotview on|off|toggle | data plot [on|off|toggle|seq|future|weight|norm|bytes] | data x idx|key|toggle | data ch next|prev|N | data sample next|prev|random|N | data dim next|prev|N|<name> | data mask on|off|toggle | data channels");
  return true;
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
