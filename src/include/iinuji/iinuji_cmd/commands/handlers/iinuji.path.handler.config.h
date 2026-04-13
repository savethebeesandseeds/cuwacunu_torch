#pragma once

// Included inside struct IinujiPathHandlers.

template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
bool dispatch_config_call(CallHandlerId call_id, PushInfo &&push_info,
                          PushWarn &&push_warn, PushErr &&push_err,
                          AppendLog &&append_log) const {
  (void)push_err;
  switch (call_id) {
  case CallHandlerId::ConfigFiles:
    if (!config_has_files(state)) {
      push_warn("no config files");
      return true;
    }
    for (std::size_t i = 0; i < state.config.files.size(); ++i) {
      const auto &file = state.config.files[i];
      append_log("[" + std::to_string(i + 1) + "] " + file.family_id + " " +
                     file.relative_path + " [" + config_access_indicator(file) +
                     "]",
                 "files", "#d0d0d0");
    }
    screen.config();
    return true;
  case CallHandlerId::ConfigFileNext:
    if (!config_has_files(state)) {
      push_warn("no config files");
      return true;
    }
    select_next_config_file(state);
    screen.config();
    push_info("selected file=" +
              std::to_string(state.config.selected_file + 1));
    return true;
  case CallHandlerId::ConfigFilePrev:
    if (!config_has_files(state)) {
      push_warn("no config files");
      return true;
    }
    select_prev_config_file(state);
    screen.config();
    push_info("selected file=" +
              std::to_string(state.config.selected_file + 1));
    return true;
  default:
    return false;
  }
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_config_file_index(
    const cuwacunu::camahjucunu::canonical_path_t &path, PushInfo &&push_info,
    PushWarn &&push_warn, PushErr &&push_err) const {
  if (!config_has_files(state)) {
    push_warn("no config files");
    return true;
  }

  std::size_t idx1 = 0;
  if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
    push_err("usage: " + canonical_paths::build_config_file_index(1));
    return true;
  }
  if (idx1 == 0 || idx1 > state.config.files.size()) {
    push_err("config file out of range");
    return true;
  }
  state.config.selected_file = idx1 - 1;
  sync_selected_config_file(state);
  screen.config();
  push_info("selected file=" + std::to_string(state.config.selected_file + 1));
  return true;
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_config_file_id(
    const cuwacunu::camahjucunu::canonical_path_t &path, PushInfo &&push_info,
    PushWarn &&push_warn, PushErr &&push_err) const {
  if (!config_has_files(state)) {
    push_warn("no config files");
    return true;
  }

  std::string id;
  if (!parse_string_arg_or_tail(path, 4, &id) || id.empty()) {
    push_err("usage: " + canonical_paths::build_config_file_id("token"));
    return true;
  }
  if (!select_config_file_by_token(state, id)) {
    push_err("config file not found");
    return true;
  }
  screen.config();
  push_info("selected file=" + std::to_string(state.config.selected_file + 1));
  return true;
}
