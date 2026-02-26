#pragma once

// Included inside struct IinujiPathHandlers.

template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
bool dispatch_data_call(CallHandlerId call_id,
                        PushInfo&& push_info,
                        PushWarn&& push_warn,
                        PushErr&& push_err,
                        AppendLog&& append_log) const {
  (void)push_err;
  switch (call_id) {
    case CallHandlerId::DataChannels:
      if (!data_has_channels(state)) {
        push_warn("no data channels");
        return true;
      }
      for (std::size_t i = 0; i < state.data.channels.size(); ++i) {
        const auto& c = state.data.channels[i];
        append_log(
            "[" + std::to_string(i + 1) + "] " + c.interval + "/" + c.record_type +
                " seq=" + std::to_string(c.seq_length) +
                " fut=" + std::to_string(c.future_seq_length),
            "data.channels",
            "#d0d0d0");
      }
      screen.data();
      return true;
    case CallHandlerId::DataPlotOn:
      screen.data();
      state.data.plot_view = true;
      push_info("data plotview=on");
      return true;
    case CallHandlerId::DataPlotOff:
      screen.data();
      state.data.plot_view = false;
      push_info("data plotview=off");
      return true;
    case CallHandlerId::DataPlotToggle:
      screen.data();
      state.data.plot_view = !state.data.plot_view;
      push_info(std::string("data plotview=") + (state.data.plot_view ? "on" : "off"));
      return true;
    case CallHandlerId::DataPlotModeSeq:
      screen.data();
      state.data.plot_mode = DataPlotMode::SeqLength;
      clamp_data_plot_mode(state);
      push_info("data plot=seq");
      return true;
    case CallHandlerId::DataPlotModeFuture:
      screen.data();
      state.data.plot_mode = DataPlotMode::FutureSeqLength;
      clamp_data_plot_mode(state);
      push_info("data plot=future");
      return true;
    case CallHandlerId::DataPlotModeWeight:
      screen.data();
      state.data.plot_mode = DataPlotMode::ChannelWeight;
      clamp_data_plot_mode(state);
      push_info("data plot=weight");
      return true;
    case CallHandlerId::DataPlotModeNorm:
      screen.data();
      state.data.plot_mode = DataPlotMode::NormWindow;
      clamp_data_plot_mode(state);
      push_info("data plot=norm");
      return true;
    case CallHandlerId::DataPlotModeBytes:
      screen.data();
      state.data.plot_mode = DataPlotMode::FileBytes;
      clamp_data_plot_mode(state);
      push_info("data plot=bytes");
      return true;
    case CallHandlerId::DataAxisToggle:
      screen.data();
      state.data.plot_x_axis = next_data_plot_x_axis(state.data.plot_x_axis);
      clamp_data_plot_x_axis(state);
      push_info("data x=" + data_plot_x_axis_token(state.data.plot_x_axis));
      return true;
    case CallHandlerId::DataAxisIdx:
      screen.data();
      state.data.plot_x_axis = DataPlotXAxis::Index;
      clamp_data_plot_x_axis(state);
      push_info("data x=idx");
      return true;
    case CallHandlerId::DataAxisKey:
      screen.data();
      state.data.plot_x_axis = DataPlotXAxis::KeyValue;
      clamp_data_plot_x_axis(state);
      push_info("data x=key");
      return true;
    case CallHandlerId::DataMaskOn:
      screen.data();
      state.data.plot_mask_overlay = true;
      push_info("data mask=on");
      return true;
    case CallHandlerId::DataMaskOff:
      screen.data();
      state.data.plot_mask_overlay = false;
      push_info("data mask=off");
      return true;
    case CallHandlerId::DataMaskToggle:
      screen.data();
      state.data.plot_mask_overlay = !state.data.plot_mask_overlay;
      push_info(std::string("data mask=") + (state.data.plot_mask_overlay ? "on" : "off"));
      return true;
    case CallHandlerId::DataChNext:
      if (!data_has_channels(state)) {
        push_warn("no data channels");
        return true;
      }
      select_next_data_channel(state);
      screen.data();
      push_info("selected data channel=" + std::to_string(state.data.selected_channel + 1));
      return true;
    case CallHandlerId::DataChPrev:
      if (!data_has_channels(state)) {
        push_warn("no data channels");
        return true;
      }
      select_prev_data_channel(state);
      screen.data();
      push_info("selected data channel=" + std::to_string(state.data.selected_channel + 1));
      return true;
    case CallHandlerId::DataSampleNext:
      if (state.data.plot_sample_count == 0) {
        push_warn("no data samples loaded");
        return true;
      }
      select_next_data_sample(state);
      screen.data();
      push_info("selected data sample=" + std::to_string(state.data.plot_sample_index + 1));
      return true;
    case CallHandlerId::DataSamplePrev:
      if (state.data.plot_sample_count == 0) {
        push_warn("no data samples loaded");
        return true;
      }
      select_prev_data_sample(state);
      screen.data();
      push_info("selected data sample=" + std::to_string(state.data.plot_sample_index + 1));
      return true;
    case CallHandlerId::DataSampleRandom:
    case CallHandlerId::DataSampleRand:
      if (state.data.plot_sample_count == 0) {
        push_warn("no data samples loaded");
        return true;
      }
      select_random_data_sample(state);
      screen.data();
      push_info("selected data sample=" + std::to_string(state.data.plot_sample_index + 1));
      return true;
    case CallHandlerId::DataDimNext:
      if (state.data.plot_D == 0) {
        push_warn("no tensor dims available");
        return true;
      }
      select_next_data_dim(state);
      screen.data();
      push_info("selected data dim=" + std::to_string(state.data.plot_feature_dim + 1));
      return true;
    case CallHandlerId::DataDimPrev:
      if (state.data.plot_D == 0) {
        push_warn("no tensor dims available");
        return true;
      }
      select_prev_data_dim(state);
      screen.data();
      push_info("selected data dim=" + std::to_string(state.data.plot_feature_dim + 1));
      return true;
    case CallHandlerId::DataFocusNext: {
      const std::size_t count = data_nav_focus_count();
      if (count > 0) {
        const auto idx = static_cast<std::size_t>(state.data.nav_focus);
        state.data.nav_focus = static_cast<DataNavFocus>((idx + 1u) % count);
      }
      clamp_data_nav_focus(state);
      screen.data();
      push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
      return true;
    }
    case CallHandlerId::DataFocusPrev: {
      const std::size_t count = data_nav_focus_count();
      if (count > 0) {
        const auto idx = static_cast<std::size_t>(state.data.nav_focus);
        state.data.nav_focus = static_cast<DataNavFocus>((idx + count - 1u) % count);
      }
      clamp_data_nav_focus(state);
      screen.data();
      push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
      return true;
    }
    case CallHandlerId::DataFocusChannel:
      state.data.nav_focus = DataNavFocus::Channel;
      clamp_data_nav_focus(state);
      screen.data();
      push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
      return true;
    case CallHandlerId::DataFocusSample:
      state.data.nav_focus = DataNavFocus::Sample;
      clamp_data_nav_focus(state);
      screen.data();
      push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
      return true;
    case CallHandlerId::DataFocusDim:
      state.data.nav_focus = DataNavFocus::Dim;
      clamp_data_nav_focus(state);
      screen.data();
      push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
      return true;
    case CallHandlerId::DataFocusPlot:
      state.data.nav_focus = DataNavFocus::PlotMode;
      clamp_data_nav_focus(state);
      screen.data();
      push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
      return true;
    case CallHandlerId::DataFocusXAxis:
      state.data.nav_focus = DataNavFocus::XAxis;
      clamp_data_nav_focus(state);
      screen.data();
      push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
      return true;
    case CallHandlerId::DataFocusMask:
      state.data.nav_focus = DataNavFocus::Mask;
      clamp_data_nav_focus(state);
      screen.data();
      push_info("data.focus=" + data_nav_focus_name(state.data.nav_focus));
      return true;
    default:
      return false;
  }
}

template <class PushInfo, class PushErr>
bool dispatch_data_plot_call(const cuwacunu::camahjucunu::canonical_path_t& path,
                             PushInfo&& push_info,
                             PushErr&& push_err) const {
  screen.data();

  if (path.args.empty()) {
    state.data.plot_view = true;
    push_info("data plotview=on");
    return true;
  }

  bool touched_mode = false;
  bool touched_view = false;
  for (const auto& arg : path.args) {
    if (arg.key == "mode") {
      const auto mode = parse_data_plot_mode_token(arg.value);
      if (!mode.has_value()) {
        push_err("invalid plot mode in iinuji call: " + arg.value);
        return true;
      }
      state.data.plot_mode = *mode;
      clamp_data_plot_mode(state);
      touched_mode = true;
      continue;
    }
    if (arg.key == "view") {
      bool view_value = state.data.plot_view;
      bool toggle = false;
      if (!parse_view_bool(arg.value, &view_value, &toggle)) {
        push_err("invalid plot view value in iinuji call: " + arg.value);
        return true;
      }
      if (toggle) state.data.plot_view = !state.data.plot_view;
      else state.data.plot_view = view_value;
      touched_view = true;
      continue;
    }
    push_err("unsupported plot arg in iinuji call: " + arg.key);
    return true;
  }

  if (touched_mode) push_info("data plot=" + data_plot_mode_token(state.data.plot_mode));
  if (touched_view) push_info(std::string("data plotview=") + (state.data.plot_view ? "on" : "off"));
  if (!touched_mode && !touched_view) push_info("data plot call applied");
  return true;
}

template <class PushInfo, class PushErr>
bool dispatch_data_x(const cuwacunu::camahjucunu::canonical_path_t& path,
                     PushInfo&& push_info,
                     PushErr&& push_err) const {
  std::string axis_raw;
  if (!get_arg_value(path, "axis", &axis_raw) &&
      !get_arg_value(path, "x", &axis_raw) &&
      !get_arg_value(path, "value", &axis_raw)) {
    axis_raw = "toggle";
  }
  const std::string axis = to_lower_copy(axis_raw);
  if (axis.empty() || axis == "toggle") {
    state.data.plot_x_axis = next_data_plot_x_axis(state.data.plot_x_axis);
  } else {
    const auto parsed_axis = parse_data_plot_x_axis_token(axis);
    if (!parsed_axis.has_value()) {
      push_err(
          "usage: " + canonical_paths::build_data_x("idx") +
          " | " + canonical_paths::build_data_x("key") +
          " | " + canonical_paths::build_data_x("toggle"));
      return true;
    }
    state.data.plot_x_axis = *parsed_axis;
  }
  clamp_data_plot_x_axis(state);
  screen.data();
  push_info("data x=" + data_plot_x_axis_token(state.data.plot_x_axis));
  return true;
}

template <class PushInfo, class PushErr>
bool dispatch_data_mask(const cuwacunu::camahjucunu::canonical_path_t& path,
                        PushInfo&& push_info,
                        PushErr&& push_err) const {
  std::string view_raw;
  if (!get_arg_value(path, "view", &view_raw) &&
      !get_arg_value(path, "value", &view_raw)) {
    view_raw = "toggle";
  }
  bool view_value = state.data.plot_mask_overlay;
  bool toggle = false;
  if (!parse_view_bool(view_raw, &view_value, &toggle)) {
    push_err(
        "usage: " + canonical_paths::build_data_mask("on") +
        " | " + canonical_paths::build_data_mask("off") +
        " | " + canonical_paths::build_data_mask("toggle"));
    return true;
  }
  if (toggle) state.data.plot_mask_overlay = !state.data.plot_mask_overlay;
  else state.data.plot_mask_overlay = view_value;
  screen.data();
  push_info(std::string("data mask=") + (state.data.plot_mask_overlay ? "on" : "off"));
  return true;
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_data_ch_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                            PushInfo&& push_info,
                            PushWarn&& push_warn,
                            PushErr&& push_err) const {
  if (!data_has_channels(state)) {
    push_warn("no data channels");
    return true;
  }
  std::size_t idx1 = 0;
  if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
    push_err("usage: " + canonical_paths::build_data_ch_index(1));
    return true;
  }
  if (idx1 == 0 || idx1 > state.data.channels.size()) {
    push_err("data channel not found");
    return true;
  }
  state.data.selected_channel = idx1 - 1;
  screen.data();
  push_info("selected data channel=" + std::to_string(state.data.selected_channel + 1));
  return true;
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_data_sample_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                PushInfo&& push_info,
                                PushWarn&& push_warn,
                                PushErr&& push_err) const {
  if (state.data.plot_sample_count == 0) {
    push_warn("no data samples loaded");
    return true;
  }
  std::size_t idx1 = 0;
  if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
    push_err("usage: " + canonical_paths::build_data_sample_index(1));
    return true;
  }
  if (idx1 == 0 || idx1 > state.data.plot_sample_count) {
    push_err("sample out of range");
    return true;
  }
  state.data.plot_sample_index = idx1 - 1;
  screen.data();
  push_info("selected data sample=" + std::to_string(state.data.plot_sample_index + 1));
  return true;
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_data_dim_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                             PushInfo&& push_info,
                             PushWarn&& push_warn,
                             PushErr&& push_err) const {
  if (state.data.plot_D == 0) {
    push_warn("no tensor dims available");
    return true;
  }
  std::size_t idx1 = 0;
  if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
    push_err("usage: " + canonical_paths::build_data_dim_index(1));
    return true;
  }
  if (idx1 == 0 || idx1 > state.data.plot_D) {
    push_err("dim out of range");
    return true;
  }
  state.data.plot_feature_dim = idx1 - 1;
  screen.data();
  push_info("selected data dim=" + std::to_string(state.data.plot_feature_dim + 1));
  return true;
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_data_dim_id(const cuwacunu::camahjucunu::canonical_path_t& path,
                          PushInfo&& push_info,
                          PushWarn&& push_warn,
                          PushErr&& push_err) const {
  if (state.data.plot_D == 0) {
    push_warn("no tensor dims available");
    return true;
  }
  std::string token;
  if (!parse_string_arg_or_tail(path, 4, &token) || token.empty()) {
    push_err("usage: " + canonical_paths::build_data_dim_id("token"));
    return true;
  }
  if (!select_data_dim_by_token(state, token)) {
    push_err("dim out of range");
    return true;
  }
  screen.data();
  push_info("selected data dim=" + std::to_string(state.data.plot_feature_dim + 1));
  return true;
}
