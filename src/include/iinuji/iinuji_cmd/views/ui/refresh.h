#pragma once

#include <algorithm>
#include <memory>
#include <string>

#include "iinuji/iinuji_cmd/views/config/view.h"
#include "iinuji/iinuji_cmd/views/human/view.h"
#include "iinuji/iinuji_cmd/views/lattice/view.h"
#include "iinuji/iinuji_cmd/views/logs/view.h"
#include "iinuji/iinuji_cmd/views/runtime/view.h"
#include "iinuji/iinuji_cmd/views/ui/bottom.h"
#include "iinuji/iinuji_cmd/views/ui/status.h"
#include "iinuji/render/layout_core.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void reveal_selected_text_row_if_changed(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &box,
    int selected_row, int text_h, int max_scroll_y) {
  auto tb = as<cuwacunu::iinuji::textBox_data_t>(panel_content_target(box));
  if (!tb)
    return;

  const bool selection_changed = (tb->tracked_selected_row != selected_row);
  tb->tracked_selected_row = selected_row;

  if (selection_changed && text_h > 0) {
    const int top = std::max(0, tb->scroll_y);
    const int bottom = top + text_h - 1;
    if (selected_row < top) {
      tb->scroll_y = std::max(0, selected_row - 1);
    } else if (selected_row > bottom) {
      tb->scroll_y = std::min(max_scroll_y, selected_row - text_h + 2);
    }
  }

  tb->scroll_y = std::clamp(tb->scroll_y, 0, max_scroll_y);
}

inline void clear_selected_text_row_tracking(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &box) {
  auto tb = as<cuwacunu::iinuji::textBox_data_t>(panel_content_target(box));
  if (!tb)
    return;
  tb->tracked_selected_row = -1;
}

inline void ui_refresh_panels(
    const CmdState &st,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &title,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &status,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &human_nav,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &human_worklist,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_main,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_aux,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &bottom,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &cmdline) {
  right_main->focusable = false;
  right_main->focused = false;
  right_aux->focusable = false;
  right_aux->focused = false;
  bottom->focusable = false;
  bottom->focused = false;
  left->focusable = false;
  left->focused = false;
  human_nav->focusable = false;
  human_nav->focused = false;
  human_worklist->focusable = false;
  human_worklist->focused = false;

  title->style.label_color = "#EDEDED";
  title->style.background_color = "#202028";
  title->style.border_color = "#6C6C75";
  status->style.label_color = "#B8B8BF";
  status->style.background_color = "#101014";
  status->style.border_color = "#101014";
  left->style.label_color = "#D0D0D0";
  left->style.background_color = "#101014";
  left->style.border_color = "#5E5E68";
  human_nav->style.label_color = "#D0D0D0";
  human_nav->style.background_color = "#101014";
  human_nav->style.border_color = "#5E5E68";
  human_worklist->style.label_color = "#D0D0D0";
  human_worklist->style.background_color = "#101014";
  human_worklist->style.border_color = "#5E5E68";
  right_main->style.label_color = "#C8C8CE";
  right_main->style.background_color = "#101014";
  right_main->style.border_color = "#5E5E68";
  right_aux->style.label_color = "#C8C8CE";
  right_aux->style.background_color = "#101014";
  right_aux->style.border_color = "#5E5E68";
  bottom->style.background_color = "#101014";
  bottom->style.border_color = "#5E5E68";
  cmdline->style.label_color = "#E8E8E8";
  cmdline->style.background_color = "#101014";
  cmdline->style.border_color = "#5E5E68";

  const bool lattice_is_loading =
      st.screen == ScreenMode::Lattice && lattice_is_visibly_loading(st);
  const bool bottom_is_error =
      (st.screen == ScreenMode::Human && st.human.status_is_error) ||
      (st.screen == ScreenMode::Runtime && st.runtime.status_is_error) ||
      (st.screen == ScreenMode::Lattice && st.lattice.status_is_error) ||
      (st.screen == ScreenMode::Config &&
       (!st.config.ok || st.config.status_is_error));
  if (bottom_is_error) {
    bottom->style.label_color = "#c38e8e";
  } else if (lattice_is_loading &&
             st.lattice.refresh_mode == LatticeRefreshMode::SyncStore) {
    bottom->style.label_color = "#e0c36c";
  } else if (lattice_is_loading) {
    bottom->style.label_color = "#7fc4ff";
  } else {
    bottom->style.label_color = "#A8A8AF";
  }
  status->style.label_color = "#B8B8BF";
  set_text_box(bottom, ui_bottom_line(st), false);

  if (st.screen == ScreenMode::Home) {
    clear_selected_text_row_tracking(left);
    clear_selected_text_row_tracking(right_main);
    clear_selected_text_row_tracking(right_aux);
    clear_selected_text_row_tracking(human_nav);
    clear_selected_text_row_tracking(human_worklist);
    left->visible = true;
    human_nav->visible = false;
    human_worklist->visible = false;
    right_main->visible = true;
    right_aux->visible = false;
    title->style.label_color = "#F1FFF4";
    title->style.background_color = "#173122";
    title->style.border_color = "#2D6A44";
    status->style.label_color = "#9ED8AE";
    left->style.border_color = "#2D6A44";
    left->style.label_color = "#DAF5E0";
    right_main->style.border_color = "#2D6A44";
    right_main->style.label_color = "#D6EBD9";
    bottom->style.border_color = "#2D6A44";
    bottom->style.label_color = "#95C9A2";
    cmdline->style.border_color = "#2D6A44";
    cmdline->style.label_color = "#E8F7EC";
    set_text_box(title, "cuwacunu.cmd - home", true);
    set_text_box(left, "", true);
    set_text_box(right_main, "", true);
    left->style.title = " waajacamaya ";
    right_main->style.title = " disclosures ";
  } else if (st.screen == ScreenMode::Human) {
    clear_selected_text_row_tracking(left);
    clear_selected_text_row_tracking(right_main);
    clear_selected_text_row_tracking(right_aux);
    clear_selected_text_row_tracking(human_nav);
    clear_selected_text_row_tracking(human_worklist);
    left->visible = false;
    human_nav->visible = true;
    human_worklist->visible = true;
    right_main->visible = true;
    right_aux->visible = false;
    set_text_box(title, "cuwacunu.cmd - human", true);
    set_text_box_styled_lines(human_nav, make_human_navigation_styled_lines(st),
                              false);
    set_text_box_styled_lines(human_worklist,
                              make_human_worklist_styled_lines(st), false);
    set_text_box_styled_lines(right_main, make_human_right_styled_lines(st),
                              true);
    human_nav->style.title = human_is_menu_focus(st.human.focus)
                                 ? " navigation [focus] "
                                 : " navigation ";
    human_worklist->style.title =
        human_is_inbox_focus(st.human.focus)
            ? " " + human_inbox_section_title(st) + " [focus] "
            : " " + human_inbox_section_title(st) + " ";
    right_main->style.title = " details ";
  } else if (st.screen == ScreenMode::Runtime) {
    clear_selected_text_row_tracking(left);
    clear_selected_text_row_tracking(right_main);
    clear_selected_text_row_tracking(right_aux);
    clear_selected_text_row_tracking(human_nav);
    clear_selected_text_row_tracking(human_worklist);
    left->visible = false;
    human_nav->visible = true;
    human_worklist->visible = true;
    right_main->visible = true;
    right_aux->visible = runtime_show_secondary_panel(st);
    set_text_box(title, "cuwacunu.cmd - runtime", true);
    set_text_box_styled_lines(human_nav,
                              make_runtime_navigation_styled_lines(st), false);
    const auto runtime_worklist = make_runtime_worklist_panel(st);
    set_text_box_styled_lines(human_worklist, runtime_worklist.lines, false);
    if (runtime_log_viewer_is_open(st) && st.runtime.log_viewer) {
      set_editor_box(right_main, st.runtime.log_viewer);
    } else {
      set_text_box_styled_lines(right_main,
                                make_runtime_primary_styled_lines(st), true);
      if (runtime_show_device_history_panel(st)) {
        const auto series = make_runtime_device_history_series(st);
        if (series.empty()) {
          set_text_box(right_aux, "collecting device history...", true);
        } else {
          set_plot_box(right_aux, series,
                       make_runtime_device_history_plot_cfg(st),
                       make_runtime_device_history_plot_opts());
        }
      }
    }
    human_nav->style.title = runtime_is_menu_focus(st.runtime.focus)
                                 ? " navigator [focus] "
                                 : " navigator ";
    human_worklist->style.title =
        runtime_is_worklist_focus(st.runtime.focus)
            ? " " + runtime_worklist_panel_title(st) + " [focus] "
            : " " + runtime_worklist_panel_title(st) + " ";
    right_main->style.title = runtime_primary_panel_title(st);
    if (runtime_show_secondary_panel(st)) {
      right_main->style.title = runtime_primary_panel_title(st);
      right_aux->style.title = runtime_secondary_panel_title(st);
    }
    if (auto tb = as<cuwacunu::iinuji::textBox_data_t>(
            panel_content_target(human_worklist));
        tb && runtime_worklist.selected_line.has_value()) {
      tb->scroll_x = 0;
      const Rect r = content_rect(*human_worklist);
      const int text_h = std::max(1, r.h);
      const int selected_row =
          static_cast<int>(*runtime_worklist.selected_line);
      const int max_scroll =
          std::max(0, static_cast<int>(runtime_worklist.lines.size()) - text_h);
      reveal_selected_text_row_if_changed(human_worklist, selected_row, text_h,
                                          max_scroll);
    } else {
      clear_selected_text_row_tracking(human_worklist);
    }
  } else if (st.screen == ScreenMode::Lattice) {
    clear_selected_text_row_tracking(left);
    clear_selected_text_row_tracking(human_nav);
    clear_selected_text_row_tracking(right_main);
    clear_selected_text_row_tracking(right_aux);
    left->visible = false;
    human_nav->visible = true;
    human_worklist->visible = true;
    right_main->visible = true;
    right_aux->visible = false;
    set_text_box(title,
                 lattice_is_loading
                     ? (st.lattice.refresh_mode == LatticeRefreshMode::SyncStore
                            ? std::string("cuwacunu.cmd - lattice [syncing]")
                            : std::string("cuwacunu.cmd - lattice [loading]"))
                     : std::string("cuwacunu.cmd - lattice"),
                 true);
    const auto navigation_lines = make_lattice_navigation_styled_lines(st);
    const auto worklist_panel = make_lattice_worklist_panel(st);
    set_text_box_styled_lines(human_nav, navigation_lines, false);
    set_text_box_styled_lines(human_worklist, worklist_panel.lines, false);
    set_text_box_styled_lines(right_main, make_lattice_right_styled_lines(st),
                              true);
    if (auto tb = as<cuwacunu::iinuji::textBox_data_t>(
            panel_content_target(human_nav));
        tb) {
      tb->scroll_x = 0;
    }
    if (auto tb = as<cuwacunu::iinuji::textBox_data_t>(
            panel_content_target(human_worklist));
        tb) {
      tb->scroll_x = 0;
    }
    if (auto tb = as<cuwacunu::iinuji::textBox_data_t>(
            panel_content_target(human_worklist));
        tb && worklist_panel.selected_line.has_value()) {
      const Rect r = content_rect(*human_worklist);
      const int text_h = std::max(1, r.h);
      const int selected_row = static_cast<int>(*worklist_panel.selected_line);
      const int max_scroll =
          std::max(0, static_cast<int>(worklist_panel.lines.size()) - text_h);
      reveal_selected_text_row_if_changed(human_worklist, selected_row, text_h,
                                          max_scroll);
    } else {
      clear_selected_text_row_tracking(human_worklist);
    }
    human_nav->style.title =
        lattice_is_navigator_focus(st) ? " sections [focus] " : " sections ";
    human_worklist->style.title =
        lattice_is_worklist_focus(st)
            ? " " + lattice_worklist_panel_title(st) + " [focus] "
            : " " + lattice_worklist_panel_title(st) + " ";
    right_main->style.title = " " + lattice_primary_panel_title(st) + " ";
  } else if (st.screen == ScreenMode::ShellLogs) {
    clear_selected_text_row_tracking(left);
    clear_selected_text_row_tracking(right_main);
    clear_selected_text_row_tracking(right_aux);
    left->visible = true;
    human_nav->visible = false;
    human_worklist->visible = false;
    right_main->visible = true;
    right_aux->visible = false;
    set_text_box(title, "cuwacunu.cmd - shell logs", true);
    const auto snap = cuwacunu::piaabo::dlog_snapshot();
    set_text_box_styled_lines(
        left, make_logs_left_styled_lines(st.shell_logs, snap), false);
    set_text_box(right_main, make_logs_right(st.shell_logs, snap), true);
    left->style.title = workspace_is_current_screen_zoomed(st)
                            ? " shell log stream [full] "
                            : " shell log stream ";
    right_main->style.title = " shell log settings ";
  } else {
    clear_selected_text_row_tracking(left);
    clear_selected_text_row_tracking(right_main);
    clear_selected_text_row_tracking(right_aux);
    left->visible = false;
    human_nav->visible = true;
    human_worklist->visible = true;
    right_main->visible = true;
    right_aux->visible = false;
    set_text_box(title, "cuwacunu.cmd - config", true);
    const auto families_panel = make_config_families_panel(st);
    const auto files_panel = make_config_files_panel(st);
    const auto detail_panel = make_config_right_panel(st);
    set_text_box_styled_lines(human_nav, families_panel.lines, false);
    set_text_box_styled_lines(human_worklist, files_panel.lines, false);
    if (st.config.editor_focus && st.config.editor) {
      set_editor_box(right_main, st.config.editor);
    } else {
      set_text_box_styled_lines(right_main, detail_panel.lines, false);
    }
    if (auto tb = as<cuwacunu::iinuji::textBox_data_t>(
            panel_content_target(human_nav));
        tb && families_panel.selected_line.has_value()) {
      const Rect r = content_rect(*human_nav);
      const auto metrics = measure_config_styled_box(
          families_panel.lines, r.w, r.h, /*wrap=*/false,
          families_panel.selected_line);
      if (metrics.text_h > 0) {
        reveal_selected_text_row_if_changed(human_nav, metrics.selected_row,
                                            metrics.text_h,
                                            metrics.max_scroll_y);
      }
    } else {
      clear_selected_text_row_tracking(human_nav);
    }
    if (auto tb = as<cuwacunu::iinuji::textBox_data_t>(
            panel_content_target(human_worklist));
        tb && files_panel.selected_line.has_value()) {
      const Rect r = content_rect(*human_worklist);
      const auto metrics =
          measure_config_styled_box(files_panel.lines, r.w, r.h, /*wrap=*/false,
                                    files_panel.selected_line);
      if (metrics.text_h > 0) {
        reveal_selected_text_row_if_changed(
            human_worklist, metrics.selected_row, metrics.text_h,
            metrics.max_scroll_y);
      }
    } else {
      clear_selected_text_row_tracking(human_worklist);
    }
    human_nav->style.title =
        config_is_family_focus(st.config) ? " families [focus] " : " families ";
    human_worklist->style.title =
        config_is_file_focus(st.config)
            ? " files [" + config_family_title(st.config.selected_family) +
                  "] [focus] "
            : " files [" + config_family_title(st.config.selected_family) +
                  "] ";
    if (st.config.editor_focus && st.config.editor) {
      const auto *file =
          selected_config_file_for_family(st, st.config.selected_family);
      const bool editable = file != nullptr && file->editable;
      right_main->style.title =
          editable ? " file [edit] " : " file [read only] ";
    } else {
      right_main->style.title = " details ";
    }
  }

  set_text_box(status, ui_status_line(st), true);
  set_text_box(cmdline, "cmd> " + st.cmdline, false);
  cmdline->focused = true;
}

struct IinujiUi {
  const CmdState &st;

  std::string status_line() const { return ui_status_line(st); }

  std::string bottom_line() const { return ui_bottom_line(st); }

  void refresh(
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &title,
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &status,
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &human_nav,
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &human_worklist,
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_main,
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_aux,
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &bottom,
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &cmdline) const {
    ui_refresh_panels(st, title, status, left, human_nav, human_worklist,
                      right_main, right_aux, bottom, cmdline);
  }
};

inline std::string make_status_line(const CmdState &st) {
  return IinujiUi{st}.status_line();
}

inline void refresh_ui(
    const CmdState &st,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &title,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &status,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &human_nav,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &human_worklist,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_main,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_aux,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &bottom,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &cmdline) {
  IinujiUi{st}.refresh(title, status, left, human_nav, human_worklist,
                       right_main, right_aux, bottom, cmdline);
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
