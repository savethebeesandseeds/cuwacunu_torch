#pragma once

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/views/config/commands.h"
#include "iinuji/iinuji_cmd/views/workbench/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool handle_config_key(CmdState& st, int ch) {
  if (st.screen != ScreenMode::Config) return false;

  const auto mirror_config_error_to_workbench = [&](std::string prefix = {}) {
    if (!st.config.status_is_error || st.config.status.empty()) {
      return;
    }
    set_workbench_status(
        st, prefix.empty() ? st.config.status : prefix + st.config.status, true);
  };

  if (!st.config.editor_focus && (ch == 'r' || ch == 'R')) {
    config_set_status(st.config, "Loading config view...", false);
    (void)queue_config_refresh(st);
    return true;
  }

  if (st.config.editor_focus && st.config.editor) {
    if (ch == 19) {
      const bool handled = save_selected_config_editor(st);
      mirror_config_error_to_workbench("Config save failed: ");
      return handled;
    }

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
      if (dirty) {
        std::vector<ui_choice_panel_option_t> options{
            ui_choice_panel_option_t{.label = "Save changes", .enabled = true},
            ui_choice_panel_option_t{.label = "Discard changes", .enabled = true},
            ui_choice_panel_option_t{.label = "Keep editing", .enabled = true},
        };
        std::size_t selected = 0u;
        bool cancelled = false;
        if (!ui_prompt_choice_panel(
                " Unsaved changes ",
                "This editor has unsaved changes. Save, discard, or keep "
                "editing before leaving the editor.",
                options, &selected, &cancelled)) {
          config_set_status(st.config,
                            "failed to collect save/discard decision", true);
          return true;
        }
        if (cancelled || selected == 2u) {
          config_set_status(st.config,
                            "editor still open | Ctrl+S saves through Config Hero",
                            false);
          return true;
        }
        if (selected == 0u) {
          (void)save_selected_config_editor(st);
          if (st.config.editor && st.config.editor->dirty) {
            mirror_config_error_to_workbench("Config save failed: ");
            return true;
          }
        } else {
          discard_config_editor_changes(st);
          config_set_status(st.config, "discarded unsaved config changes", false);
        }
      }
      st.config.editor_focus = false;
      if (!dirty) {
        config_set_status(st.config, config_default_status_message(st.config),
                          !st.config.ok ||
                              (config_has_files(st) &&
                               !st.config.files[st.config.selected_file].ok));
      }
      if (st.workbench.editor_return.active) {
        if (st.config.transient_single_file_view && st.config.catalog_snapshot) {
          const std::shared_ptr<ConfigAsyncState> preserved_async = st.config.async;
          st.config = *st.config.catalog_snapshot;
          st.config.async = preserved_async ? preserved_async : st.config.async;
          st.config.transient_single_file_view = false;
          st.config.catalog_snapshot.reset();
          st.config.editor_focus = false;
        }
        const auto action = st.workbench.editor_return.booster_action;
        const std::string status =
            st.workbench.editor_return.status.empty()
                ? "Returned to Booster. Review edits and continue."
                : st.workbench.editor_return.status;
        st.workbench.editor_return = WorkbenchEditorReturnState{};
        st.screen = ScreenMode::Workbench;
        st.workbench.section = kWorkbenchBoosterSection;
        focus_workbench_worklist(st);
        st.workbench.selected_booster_action = static_cast<std::size_t>(action);
        set_workbench_status(st, status, false);
        (void)queue_config_refresh(st);
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
