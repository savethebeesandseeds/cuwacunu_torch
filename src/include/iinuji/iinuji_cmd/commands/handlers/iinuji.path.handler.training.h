#pragma once

// Included inside struct IinujiPathHandlers.

template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
bool dispatch_training_call(CallHandlerId call_id,
                            PushInfo&& push_info,
                            PushWarn&& push_warn,
                            PushErr&& push_err,
                            AppendLog&& append_log) const {
  (void)push_err;
  switch (call_id) {
    case CallHandlerId::TrainingTabs: {
      const auto& docs = training_wikimyei_docs();
      if (docs.empty()) {
        push_warn("no training wikimyei tabs");
        return true;
      }
      for (std::size_t i = 0; i < docs.size(); ++i) {
        append_log("[" + std::to_string(i + 1) + "] " + docs[i].id, "training.tabs", "#d0d0d0");
      }
      screen.training();
      return true;
    }
    case CallHandlerId::TrainingTabNext:
      select_next_training_tab(state);
      screen.training();
      push_info("selected training tab=" + std::to_string(state.training.selected_tab + 1));
      return true;
    case CallHandlerId::TrainingTabPrev:
      select_prev_training_tab(state);
      screen.training();
      push_info("selected training tab=" + std::to_string(state.training.selected_tab + 1));
      return true;
    case CallHandlerId::TrainingHashNext:
      select_next_training_hash(state);
      screen.training();
      push_info("selected training hash=" + std::to_string(state.training.selected_hash + 1));
      return true;
    case CallHandlerId::TrainingHashPrev:
      select_prev_training_hash(state);
      screen.training();
      push_info("selected training hash=" + std::to_string(state.training.selected_hash + 1));
      return true;
    default:
      return false;
  }
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_training_tab_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                 PushInfo&& push_info,
                                 PushWarn&& push_warn,
                                 PushErr&& push_err) const {
  const std::size_t n = training_wikimyei_count();
  if (n == 0) {
    push_warn("no training wikimyei tabs");
    return true;
  }

  std::size_t idx1 = 0;
  if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
    push_err("usage: " + canonical_paths::build_training_tab_index(1));
    return true;
  }
  if (idx1 == 0 || idx1 > n) {
    push_err("training tab out of range");
    return true;
  }
  state.training.selected_tab = idx1 - 1;
  screen.training();
  push_info("selected training tab=" + std::to_string(state.training.selected_tab + 1));
  return true;
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_training_tab_id(const cuwacunu::camahjucunu::canonical_path_t& path,
                              PushInfo&& push_info,
                              PushWarn&& push_warn,
                              PushErr&& push_err) const {
  const std::size_t n = training_wikimyei_count();
  if (n == 0) {
    push_warn("no training wikimyei tabs");
    return true;
  }

  std::string id;
  if (!parse_string_arg_or_tail(path, 4, &id) || id.empty()) {
    push_err("usage: " + canonical_paths::build_training_tab_id("token"));
    return true;
  }
  if (!select_training_tab_by_token(state, id)) {
    push_err("training tab not found");
    return true;
  }
  screen.training();
  push_info("selected training tab=" + std::to_string(state.training.selected_tab + 1));
  return true;
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_training_hash_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                  PushInfo&& push_info,
                                  PushWarn&& push_warn,
                                  PushErr&& push_err) const {
  const auto hashes = training_hashes_for_selected_tab(state);
  if (hashes.empty()) {
    push_warn("no created hashimyei artifacts for selected wikimyei");
    return true;
  }

  std::size_t idx1 = 0;
  if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
    push_err("usage: " + canonical_paths::build_training_hash_index(1));
    return true;
  }
  if (idx1 == 0 || idx1 > hashes.size()) {
    push_err("training hash out of range");
    return true;
  }
  state.training.selected_hash = idx1 - 1;
  screen.training();
  push_info("selected training hash=" + std::to_string(state.training.selected_hash + 1));
  return true;
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_training_hash_id(const cuwacunu::camahjucunu::canonical_path_t& path,
                               PushInfo&& push_info,
                               PushWarn&& push_warn,
                               PushErr&& push_err) const {
  const auto hashes = training_hashes_for_selected_tab(state);
  if (hashes.empty()) {
    push_warn("no created hashimyei artifacts for selected wikimyei");
    return true;
  }

  std::string id;
  if (!parse_string_arg_or_tail(path, 4, &id) || id.empty()) {
    push_err("usage: " + canonical_paths::build_training_hash_id("token"));
    return true;
  }
  if (!select_training_hash_by_token(state, id)) {
    push_err("training hash not found");
    return true;
  }
  screen.training();
  push_info("selected training hash=" + std::to_string(state.training.selected_hash + 1));
  return true;
}
