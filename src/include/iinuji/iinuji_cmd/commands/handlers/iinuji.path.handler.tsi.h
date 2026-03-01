#pragma once

// Included inside struct IinujiPathHandlers.

template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
bool dispatch_tsi_call(CallHandlerId call_id,
                       PushInfo&& push_info,
                       PushWarn&& push_warn,
                       PushErr&& push_err,
                       AppendLog&& append_log) const {
  switch (call_id) {
    case CallHandlerId::TsiTabs: {
      const auto& docs = tsi_node_docs();
      if (docs.empty()) {
        push_warn("no tsi tabs");
        return true;
      }
      for (std::size_t i = 0; i < docs.size(); ++i) {
        append_log("[" + std::to_string(i + 1) + "] " + docs[i].id, "tsi.tabs", "#d0d0d0");
      }
      screen.tsi();
      return true;
    }
    case CallHandlerId::TsiTabNext:
      select_next_tsi_tab(state);
      screen.tsi();
      push_info("selected tsi tab=" + std::to_string(state.tsiemene.selected_tab + 1));
      return true;
    case CallHandlerId::TsiTabPrev:
      select_prev_tsi_tab(state);
      screen.tsi();
      push_info("selected tsi tab=" + std::to_string(state.tsiemene.selected_tab + 1));
      return true;
    case CallHandlerId::TsiDataloaderInit:
    case CallHandlerId::TsiDataloaderCreate: {
      screen.tsi();
      const bool legacy_init_call = (call_id == CallHandlerId::TsiDataloaderInit);
      const std::string invoke_action = legacy_init_call
          ? "tsi.source.dataloader.init()"
          : "tsi.source.dataloader.create()";
      if (state.board.contract_hash.empty()) {
        push_err("tsi source.dataloader.create failed: board contract hash is unavailable");
        return true;
      }
      try {
        const auto init = tsiemene::invoke_source_dataloader_init_from_config(
            state.board.contract_hash);
        if (!init.ok) {
          push_err("tsi source.dataloader.create failed: " + init.error);
          return true;
        }
        select_tsi_source_dataloader_by_id(state, init.init_id);
        append_log("invoke=" + invoke_action, "tsi.action", "#d8d8ff");
        append_log("init.id=" + init.init_id, "tsi.action", "#d8d8ff");
        append_log("init.state=contract-backed", "tsi.action", "#d8d8ff");
        append_log("init.dir=" + init.init_directory.string(), "tsi.action", "#d8d8ff");
        append_log("observation.instruments=" + std::to_string(init.instrument_count),
                   "tsi.action",
                   "#d8d8ff");
        append_log("observation.inputs=" + std::to_string(init.input_count) +
                       " active=" + std::to_string(init.active_input_count) +
                       " seq.max=" + std::to_string(init.max_seq_length) +
                       " future.max=" + std::to_string(init.max_future_seq_length),
                   "tsi.action",
                   "#d8d8ff");
        if (!init.default_instrument.empty()) {
          append_log("observation.instrument.default=" + init.default_instrument,
                     "tsi.action",
                     "#d8d8ff");
        }
        push_info("tsi action invoked: source.dataloader.create id=" + init.init_id);
      } catch (const std::exception& e) {
        push_err("tsi source.dataloader.create failed: " + std::string(e.what()));
      }
      return true;
    }
    case CallHandlerId::TsiDataloaderEdit: {
      screen.tsi();
      const std::string init_id = selected_tsi_source_dataloader_id(state);
      if (init_id.empty()) {
        push_warn("no tsi.source.dataloader selected");
        return true;
      }
      if (state.board.contract_hash.empty()) {
        push_err("tsi source.dataloader.edit failed: board contract hash is unavailable");
        return true;
      }
      try {
        const auto updated = tsiemene::update_source_dataloader_init_from_config(
            init_id, state.board.contract_hash);
        if (!updated.ok) {
          push_err("tsi source.dataloader.edit failed: " + updated.error);
          return true;
        }
        select_tsi_source_dataloader_by_id(state, updated.init_id);
        append_log("invoke=tsi.source.dataloader.edit()", "tsi.action", "#d8d8ff");
        append_log("init.id=" + updated.init_id, "tsi.action", "#d8d8ff");
        append_log("init.state=contract-backed", "tsi.action", "#d8d8ff");
        append_log("init.dir=" + updated.init_directory.string(), "tsi.action", "#d8d8ff");
        append_log("observation.instruments=" + std::to_string(updated.instrument_count),
                   "tsi.action",
                   "#d8d8ff");
        append_log("observation.inputs=" + std::to_string(updated.input_count) +
                       " active=" + std::to_string(updated.active_input_count) +
                       " seq.max=" + std::to_string(updated.max_seq_length) +
                       " future.max=" + std::to_string(updated.max_future_seq_length),
                   "tsi.action",
                   "#d8d8ff");
        if (!updated.default_instrument.empty()) {
          append_log("observation.instrument.default=" + updated.default_instrument,
                     "tsi.action",
                     "#d8d8ff");
        }
        push_info("tsi action invoked: source.dataloader.edit id=" + updated.init_id);
      } catch (const std::exception& e) {
        push_err("tsi source.dataloader.edit failed: " + std::string(e.what()));
      }
      return true;
    }
    case CallHandlerId::TsiDataloaderDelete: {
      screen.tsi();
      const std::string init_id = selected_tsi_source_dataloader_id(state);
      if (init_id.empty()) {
        push_warn("no tsi.source.dataloader selected");
        return true;
      }
      std::string error;
      std::uintmax_t removed_count = 0;
      if (!tsiemene::delete_source_dataloader_init(init_id, &removed_count, &error)) {
        push_err("tsi source.dataloader.delete failed: " + error);
        return true;
      }
      clamp_tsi_navigation_state(state);
      append_log("invoke=tsi.source.dataloader.delete()", "tsi.action", "#d8d8ff");
      append_log("init.id=" + init_id, "tsi.action", "#d8d8ff");
      append_log("removed.paths=" + std::to_string(removed_count), "tsi.action", "#d8d8ff");
      if (removed_count == 0) append_log("delete.mode=contract-backed(no-op)", "tsi.action", "#d8d8ff");
      push_info("tsi action invoked: source.dataloader.delete id=" + init_id);
      return true;
    }
    case CallHandlerId::TsiVicregInit:
      screen.tsi();
      try {
        const auto init = tsiemene::invoke_wikimyei_representation_vicreg_init_from_config();
        if (!init.ok) {
          push_err("tsi wikimyei.representation.vicreg.init failed: " + init.error);
          return true;
        }
        append_log("invoke=tsi.wikimyei.representation.vicreg.init()", "tsi.action", "#d8d8ff");
        append_log("hashimyei=" + init.hashimyei, "tsi.action", "#d8d8ff");
        append_log("canonical=" + init.canonical_base, "tsi.action", "#d8d8ff");
        append_log("artifact.dir=" + init.artifact_directory.string(), "tsi.action", "#d8d8ff");
        append_log("weights.file=" + init.weights_file.filename().string(), "tsi.action", "#d8d8ff");
        if (init.metadata_encrypted) {
          append_log("metadata=persisted(encrypted)", "tsi.action", "#d8d8ff");
        } else if (init.metadata_plaintext_fallback) {
          append_log("metadata=persisted(plaintext-fallback)", "tsi.action", "#d8d8ff");
          if (!init.metadata_warning.empty()) {
            push_warn("tsi wikimyei metadata fallback: " + init.metadata_warning);
          }
        }
        push_info("tsi action invoked: wikimyei.representation.vicreg.init hash=" + init.hashimyei);
      } catch (const std::exception& e) {
        push_err("tsi wikimyei.representation.vicreg.init failed: " + std::string(e.what()));
      }
      return true;
    default:
      return false;
  }
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_tsi_tab_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                            PushInfo&& push_info,
                            PushWarn&& push_warn,
                            PushErr&& push_err) const {
  const std::size_t n = tsi_tab_count();
  if (n == 0) {
    push_warn("no tsi tabs");
    return true;
  }

  std::size_t idx1 = 0;
  if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
    push_err("usage: " + canonical_paths::build_tsi_tab_index(1));
    return true;
  }
  if (idx1 == 0 || idx1 > n) {
    push_err("tsi tab out of range");
    return true;
  }
  state.tsiemene.selected_tab = idx1 - 1;
  screen.tsi();
  push_info("selected tsi tab=" + std::to_string(state.tsiemene.selected_tab + 1));
  return true;
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_tsi_tab_id(const cuwacunu::camahjucunu::canonical_path_t& path,
                         PushInfo&& push_info,
                         PushWarn&& push_warn,
                         PushErr&& push_err) const {
  const std::size_t n = tsi_tab_count();
  if (n == 0) {
    push_warn("no tsi tabs");
    return true;
  }

  std::string id;
  if (!parse_string_arg_or_tail(path, 4, &id) || id.empty()) {
    push_err("usage: " + canonical_paths::build_tsi_tab_id("token"));
    return true;
  }
  if (!select_tsi_tab_by_token(state, id)) {
    push_err("tsi tab not found");
    return true;
  }
  screen.tsi();
  push_info("selected tsi tab=" + std::to_string(state.tsiemene.selected_tab + 1));
  return true;
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_tsi_dataloader_edit(const cuwacunu::camahjucunu::canonical_path_t& path,
                                  PushInfo&& push_info,
                                  PushWarn&& push_warn,
                                  PushErr&& push_err) const {
  std::string init_id;
  if (parse_string_arg(path, &init_id)) {
    if (path.segments.size() != 4) {
      push_err("usage: " + canonical_paths::build_tsi_dataloader_edit("0x0000"));
      return true;
    }
  } else if (path.args.empty() && path.segments.size() == 5) {
    init_id = path.segments.back();
  } else if (!(path.args.empty() && path.segments.size() == 4)) {
    push_err("usage: " + canonical_paths::build_tsi_dataloader_edit("0x0000"));
    return true;
  }

  if (init_id.empty()) init_id = selected_tsi_source_dataloader_id(state);
  if (init_id.empty()) {
    push_warn("no tsi.source.dataloader selected");
    return true;
  }
  if (state.board.contract_hash.empty()) {
    push_err("tsi source.dataloader.edit failed: board contract hash is unavailable");
    return true;
  }
  if (!tsiemene::is_valid_source_dataloader_init_id(init_id)) {
    push_err("invalid dataloader id: " + init_id);
    return true;
  }

  screen.tsi();
  try {
    const auto updated = tsiemene::update_source_dataloader_init_from_config(
        init_id, state.board.contract_hash);
    if (!updated.ok) {
      push_err("tsi source.dataloader.edit failed: " + updated.error);
      return true;
    }
    select_tsi_source_dataloader_by_id(state, updated.init_id);
    push_info("tsi action invoked: source.dataloader.edit id=" + updated.init_id);
  } catch (const std::exception& e) {
    push_err("tsi source.dataloader.edit failed: " + std::string(e.what()));
  }
  return true;
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_tsi_dataloader_delete(const cuwacunu::camahjucunu::canonical_path_t& path,
                                    PushInfo&& push_info,
                                    PushWarn&& push_warn,
                                    PushErr&& push_err) const {
  std::string init_id;
  if (parse_string_arg(path, &init_id)) {
    if (path.segments.size() != 4) {
      push_err("usage: " + canonical_paths::build_tsi_dataloader_delete("0x0000"));
      return true;
    }
  } else if (path.args.empty() && path.segments.size() == 5) {
    init_id = path.segments.back();
  } else if (!(path.args.empty() && path.segments.size() == 4)) {
    push_err("usage: " + canonical_paths::build_tsi_dataloader_delete("0x0000"));
    return true;
  }

  if (init_id.empty()) init_id = selected_tsi_source_dataloader_id(state);
  if (init_id.empty()) {
    push_warn("no tsi.source.dataloader selected");
    return true;
  }
  if (!tsiemene::is_valid_source_dataloader_init_id(init_id)) {
    push_err("invalid dataloader id: " + init_id);
    return true;
  }

  screen.tsi();
  std::string error;
  std::uintmax_t removed_count = 0;
  if (!tsiemene::delete_source_dataloader_init(init_id, &removed_count, &error)) {
    push_err("tsi source.dataloader.delete failed: " + error);
    return true;
  }
  clamp_tsi_navigation_state(state);
  push_info("tsi action invoked: source.dataloader.delete id=" + init_id +
            " removed=" + std::to_string(removed_count));
  return true;
}
