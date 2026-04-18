#pragma once

#include <string>
#include <string_view>

#include "iinuji/iinuji_cmd/commands/iinuji.path.tokens.h"
#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void sync_selected_config_file(CmdState &st) {
  st.config.editor_focus = false;
  if (const auto *file = selected_config_file(st); file != nullptr) {
    st.config.selected_family = file->family;
  }
  sync_config_editor_from_selection(st.config);
  config_set_status(st.config, config_default_status_message(st.config),
                    !st.config.ok ||
                        (config_has_files(st) &&
                         !st.config.files[st.config.selected_file].ok));
}

inline void focus_config_families(CmdState &st) {
  st.config.editor_focus = false;
  st.config.browse_focus = ConfigBrowseFocus::Families;
}

inline void focus_config_files(CmdState &st) {
  st.config.editor_focus = false;
  st.config.browse_focus = ConfigBrowseFocus::Files;
}

inline void select_config_family(CmdState &st, ConfigFileFamily family) {
  st.config.selected_family = family;
  if (const auto idx = config_first_file_index_for_family(st, family);
      idx.has_value()) {
    st.config.selected_file = *idx;
    sync_config_editor_from_selection(st.config);
    config_set_status(st.config, config_default_status_message(st.config),
                      !st.config.ok ||
                          (config_has_files(st) &&
                           !st.config.files[st.config.selected_file].ok));
  } else {
    sync_config_editor_from_selection(st.config);
    config_set_status(st.config, "selected family has no files", false);
  }
  st.config.editor_focus = false;
}

inline void select_next_config_family(CmdState &st) {
  const std::size_t next =
      (config_family_index(st.config.selected_family) + 1u) %
      config_family_count();
  select_config_family(st, config_family_from_index(next));
}

inline void select_prev_config_family(CmdState &st) {
  const std::size_t current = config_family_index(st.config.selected_family);
  const std::size_t prev =
      (current + config_family_count() - 1u) % config_family_count();
  select_config_family(st, config_family_from_index(prev));
}

inline void select_first_config_family(CmdState &st) {
  select_config_family(st, config_family_from_index(0u));
}

inline void select_last_config_family(CmdState &st) {
  select_config_family(st,
                       config_family_from_index(config_family_count() - 1u));
}

inline void select_next_config_file_in_family(CmdState &st) {
  const auto indices =
      config_file_indices_for_family(st, st.config.selected_family);
  if (indices.empty()) {
    st.config.editor_focus = false;
    config_set_status(st.config, "selected family has no files", false);
    return;
  }
  std::size_t visible = 0u;
  for (; visible < indices.size(); ++visible) {
    if (indices[visible] == st.config.selected_file)
      break;
  }
  if (visible >= indices.size())
    visible = 0u;
  else
    visible = (visible + 1u) % indices.size();
  st.config.selected_file = indices[visible];
  sync_selected_config_file(st);
}

inline void select_prev_config_file_in_family(CmdState &st) {
  const auto indices =
      config_file_indices_for_family(st, st.config.selected_family);
  if (indices.empty()) {
    st.config.editor_focus = false;
    config_set_status(st.config, "selected family has no files", false);
    return;
  }
  std::size_t visible = 0u;
  for (; visible < indices.size(); ++visible) {
    if (indices[visible] == st.config.selected_file)
      break;
  }
  if (visible >= indices.size())
    visible = 0u;
  else
    visible = (visible + indices.size() - 1u) % indices.size();
  st.config.selected_file = indices[visible];
  sync_selected_config_file(st);
}

inline void select_first_config_file_in_family(CmdState &st) {
  const auto idx =
      config_first_file_index_for_family(st, st.config.selected_family);
  if (!idx.has_value()) {
    st.config.editor_focus = false;
    config_set_status(st.config, "selected family has no files", false);
    return;
  }
  st.config.selected_file = *idx;
  sync_selected_config_file(st);
}

inline void select_last_config_file_in_family(CmdState &st) {
  const auto indices =
      config_file_indices_for_family(st, st.config.selected_family);
  if (indices.empty()) {
    st.config.editor_focus = false;
    config_set_status(st.config, "selected family has no files", false);
    return;
  }
  st.config.selected_file = indices.back();
  sync_selected_config_file(st);
}

inline bool open_selected_config_file(CmdState &st) {
  auto *file = selected_config_file_for_family(st, st.config.selected_family);
  if (file == nullptr) {
    config_set_status(st.config, "no file selected in the active family", true);
    return true;
  }
  std::string error{};
  const bool force_reload = file->payload_loaded && !file->ok;
  if ((!file->payload_loaded || force_reload) &&
      !config_load_file_payload(st.config, file, force_reload, &error)) {
    sync_config_editor_from_selection(st.config);
    st.config.editor_focus = false;
    config_set_status(
        st.config,
        error.empty() ? "failed to load selected config file" : error, true);
    return true;
  }
  sync_config_editor_from_selection(st.config);
  st.config.editor_focus = true;
  config_set_status(st.config,
                    file->editable
                        ? "editor focus | Ctrl+S saves through Config Hero"
                        : "viewer focus | read only",
                    false);
  return true;
}

inline void select_next_config_file(CmdState &st) {
  select_next_config_file_in_family(st);
}

inline void select_prev_config_file(CmdState &st) {
  select_prev_config_file_in_family(st);
}

inline void select_first_config_file(CmdState &st) {
  select_first_config_file_in_family(st);
}

inline void select_last_config_file(CmdState &st) {
  select_last_config_file_in_family(st);
}

inline bool select_config_file_by_token(CmdState &st,
                                        const std::string &token) {
  if (!config_has_files(st))
    return false;
  std::size_t idx1 = 0;
  if (parse_positive_index(token, &idx1)) {
    if (idx1 == 0 || idx1 > st.config.files.size())
      return false;
    st.config.selected_file = idx1 - 1;
    sync_selected_config_file(st);
    return true;
  }

  const std::string needle = to_lower_copy(token);
  for (std::size_t i = 0; i < st.config.files.size(); ++i) {
    const auto &file = st.config.files[i];
    if (to_lower_copy(file.id) == needle ||
        to_lower_copy(file.title) == needle ||
        to_lower_copy(file.relative_path) == needle ||
        canonical_path_tokens::token_matches(file.id, token) ||
        canonical_path_tokens::token_matches(file.title, token) ||
        canonical_path_tokens::token_matches(file.relative_path, token)) {
      st.config.selected_file = i;
      sync_selected_config_file(st);
      focus_config_files(st);
      return true;
    }
  }
  return false;
}

inline void discard_config_editor_changes(CmdState &st) {
  auto *file = selected_config_file(st);
  if (file == nullptr)
    return;
  file->content = file->saved_content;
  sync_config_editor_from_selection(st.config);
}

inline bool save_selected_config_editor(CmdState &st) {
  auto *file = selected_config_file(st);
  if (file == nullptr) {
    config_set_status(st.config, "no config file selected", true);
    return true;
  }
  if (!st.config.editor) {
    config_set_status(st.config, "config editor is unavailable", true);
    return true;
  }
  if (!file->ok) {
    config_set_status(st.config, "selected config file is not readable", true);
    return true;
  }
  if (!file->editable) {
    config_set_status(
        st.config, "selected file is not editable through Config Hero", true);
    return true;
  }

  auto &editor = *st.config.editor;
  if (!editor.dirty) {
    config_set_status(st.config, "no config changes to save", false);
    return true;
  }

  const std::string tool_name = config_replace_tool_name(*file);
  if (tool_name.empty()) {
    config_set_status(st.config, "no Config Hero replace tool for this file",
                      true);
    return true;
  }

  cuwacunu::hero::mcp::hero_config_store_t store(
      config_effective_write_policy_path(st.config),
      cuwacunu::iitepi::config_space_t::config_file_path);
  std::string error{};
  if (!store.load(&error)) {
    config_set_status(st.config, "failed to load config policy: " + error,
                      true);
    return true;
  }

  const std::string content = cuwacunu::iinuji::primitives::editor_text(editor);
  cmd_json_value_t structured{};
  if (!config_invoke_tool(tool_name,
                          config_build_replace_request_json(*file, content),
                          &store, &structured, &error)) {
    config_set_status(st.config, "save failed: " + error, true);
    return true;
  }

  (void)config_fill_file_payload(&store, file);
  sync_config_editor_from_selection(st.config);
  st.config.editor_focus = true;
  if (!file->ok) {
    config_set_status(st.config,
                      "save completed but reload failed: " + file->error, true);
  } else {
    config_set_status(st.config, "saved via " + tool_name, false);
  }
  return true;
}

template <class PushWarn, class AppendLog>
inline bool handle_config_show(CmdState &st, PushWarn &&push_warn,
                               AppendLog &&append_log) {
  if (!config_has_files(st)) {
    push_warn("no config files");
    return true;
  }
  const auto &file = st.config.files[st.config.selected_file];
  append_log("family=" + file.family_id, "show", "#d8d8ff");
  append_log("relative=" + file.relative_path, "show", "#d8d8ff");
  append_log("path=" + (file.path.empty() ? "<none>" : file.path), "show",
             "#d8d8ff");
  append_log("mode=" + config_readable_mode(file), "show", "#d8d8ff");
  append_log("policy=" + (st.config.policy_path.empty()
                              ? "<none>"
                              : st.config.policy_path),
             "show", "#d8d8ff");
  append_log("write_policy=" +
                 (config_effective_write_policy_path(st.config).empty()
                      ? "<none>"
                      : config_effective_write_policy_path(st.config)),
             "show", "#d8d8ff");
  return true;
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
