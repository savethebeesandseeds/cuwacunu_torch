#pragma once

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/views/config/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool handle_config_key(CmdState& st, int ch) {
  if (st.screen != ScreenMode::Config) return false;

  if (!st.config.editor_focus && (ch == 'r' || ch == 'R')) {
    config_set_status(st.config, "Loading config view...", false);
    (void)queue_config_refresh(st);
    return true;
  }

  if (st.config.editor_focus && st.config.editor) {
    if (ch == 19) return save_selected_config_editor(st);

    const auto result = cuwacunu::iinuji::primitives::editor_handle_key(
        *st.config.editor, ch,
        cuwacunu::iinuji::primitives::editor_keymap_options_t{
            .allow_save = false,
            .allow_reload = false,
            .allow_history = true,
            .allow_close = true,
            .allow_newline = true,
            .allow_tab_indent = true,
            .allow_printable_insert = true,
            .escape_requests_close = true,
        });
    if (!result.handled) return false;

    if (auto* file = selected_config_file(st); file != nullptr) {
      file->content =
          cuwacunu::iinuji::primitives::editor_text(*st.config.editor);
    }

    if (result.close_requested) {
      const bool dirty = st.config.editor->dirty;
      st.config.editor_focus = false;
      if (dirty) {
        discard_config_editor_changes(st);
        config_set_status(st.config, "discarded unsaved config changes", false);
      } else {
        config_set_status(st.config, config_default_status_message(st.config),
                          !st.config.ok ||
                              (config_has_files(st) &&
                               !st.config.files[st.config.selected_file].ok));
      }
      return true;
    }

    if (result.content_changed && st.config.editor->dirty) {
      config_set_status(st.config,
                        "editor modified | Ctrl+S saves through Config Hero",
                        false);
    } else if (!result.status.empty()) {
      config_set_status(st.config, result.status, false);
    }
    return true;
  }

  if (!config_has_files(st)) return false;

  if (config_is_family_focus(st.config)) {
    if (ch == KEY_DOWN) {
      select_next_config_family(st);
      return true;
    }
    if (ch == KEY_UP) {
      select_prev_config_family(st);
      return true;
    }
    if (ch == KEY_HOME) {
      select_first_config_family(st);
      return true;
    }
    if (ch == KEY_END) {
      select_last_config_family(st);
      return true;
    }
    if (ch == '\t' || ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      focus_config_files(st);
      if (!config_first_file_index_for_family(st, st.config.selected_family).has_value()) {
        config_set_status(st.config, "selected family has no files", false);
      } else {
        config_set_status(st.config, "files focus", false);
      }
      return true;
    }
    return false;
  }

  if (ch == 27) {
    focus_config_families(st);
    config_set_status(st.config, "families focus", false);
    return true;
  }
  if (ch == KEY_DOWN) {
    select_next_config_file(st);
    return true;
  }
  if (ch == KEY_UP) {
    select_prev_config_file(st);
    return true;
  }
  if (ch == KEY_HOME) {
    select_first_config_file(st);
    return true;
  }
  if (ch == KEY_END) {
    select_last_config_file(st);
    return true;
  }
  if (ch == '\n' || ch == '\r' || ch == KEY_ENTER || ch == 'e' || ch == 'E') {
    return open_selected_config_file(st);
  }
  return false;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
