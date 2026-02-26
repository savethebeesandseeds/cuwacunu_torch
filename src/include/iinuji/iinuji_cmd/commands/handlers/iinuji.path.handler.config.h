#pragma once

// Included inside struct IinujiPathHandlers.

template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
bool dispatch_config_call(CallHandlerId call_id,
                          PushInfo&& push_info,
                          PushWarn&& push_warn,
                          PushErr&& push_err,
                          AppendLog&& append_log) const {
  (void)push_err;
  switch (call_id) {
    case CallHandlerId::ConfigTabs:
      if (!config_has_tabs(state)) {
        push_warn("no config tabs");
        return true;
      }
      for (std::size_t i = 0; i < state.config.tabs.size(); ++i) {
        const auto& tab = state.config.tabs[i];
        append_log(
            "[" + std::to_string(i + 1) + "] " + tab.id + (tab.ok ? "" : " (err)"),
            "tabs",
            "#d0d0d0");
      }
      screen.config();
      return true;
    case CallHandlerId::ConfigTabNext:
      if (!config_has_tabs(state)) {
        push_warn("no config tabs");
        return true;
      }
      select_next_tab(state);
      screen.config();
      push_info("selected tab=" + std::to_string(state.config.selected_tab + 1));
      return true;
    case CallHandlerId::ConfigTabPrev:
      if (!config_has_tabs(state)) {
        push_warn("no config tabs");
        return true;
      }
      select_prev_tab(state);
      screen.config();
      push_info("selected tab=" + std::to_string(state.config.selected_tab + 1));
      return true;
    default:
      return false;
  }
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_config_tab_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                               PushInfo&& push_info,
                               PushWarn&& push_warn,
                               PushErr&& push_err) const {
  if (!config_has_tabs(state)) {
    push_warn("no config tabs");
    return true;
  }

  std::size_t idx1 = 0;
  if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
    push_err("usage: " + canonical_paths::build_config_tab_index(1));
    return true;
  }
  if (idx1 == 0 || idx1 > state.config.tabs.size()) {
    push_err("config tab out of range");
    return true;
  }
  state.config.selected_tab = idx1 - 1;
  screen.config();
  push_info("selected tab=" + std::to_string(state.config.selected_tab + 1));
  return true;
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_config_tab_id(const cuwacunu::camahjucunu::canonical_path_t& path,
                            PushInfo&& push_info,
                            PushWarn&& push_warn,
                            PushErr&& push_err) const {
  if (!config_has_tabs(state)) {
    push_warn("no config tabs");
    return true;
  }

  std::string id;
  if (!parse_string_arg_or_tail(path, 4, &id) || id.empty()) {
    push_err("usage: " + canonical_paths::build_config_tab_id("token"));
    return true;
  }
  if (!select_tab_by_token(state, id)) {
    push_err("tab not found");
    return true;
  }
  screen.config();
  push_info("selected tab=" + std::to_string(state.config.selected_tab + 1));
  return true;
}
