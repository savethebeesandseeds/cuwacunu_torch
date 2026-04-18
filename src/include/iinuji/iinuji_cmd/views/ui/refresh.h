#pragma once

#include <algorithm>
#include <memory>
#include <string>

#include "iinuji/iinuji_cmd/views/common/viewport.h"
#include "iinuji/iinuji_cmd/views/config/view.h"
#include "iinuji/iinuji_cmd/views/workbench/view.h"
#include "iinuji/iinuji_cmd/views/lattice/view.h"
#include "iinuji/iinuji_cmd/views/logs/view.h"
#include "iinuji/iinuji_cmd/views/runtime/view.h"
#include "iinuji/iinuji_cmd/views/ui/bottom.h"
#include "iinuji/iinuji_cmd/views/ui/status.h"
#include "iinuji/render/layout_core.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
ui_title_styled_lines(const CmdState &st) {
  using cuwacunu::iinuji::text_line_emphasis_t;
  return {cuwacunu::iinuji::make_segmented_styled_text_line(
      ui_status_segments(st), text_line_emphasis_t::None)};
}

inline void ui_refresh_home_screen(
    const CmdState &st,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &title,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &status,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left_nav_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
        &left_worklist_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_main,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_aux,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &bottom,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &cmdline) {
  clear_panel_selection_tracking(
      {left, right_main, right_aux, left_nav_panel, left_worklist_panel});
  show_single_left_panel(left, left_nav_panel, left_worklist_panel);
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
  bottom->style.label_color = "#9AB7A1";
  cmdline->style.border_color = "#2D6A44";
  cmdline->style.label_color = "#E8F7EC";
  set_text_box(left, "", true);
  set_text_box(right_main, "", true);
  left->style.title = " waajacamaya ";
  right_main->style.title = " disclosures ";
  (void)st;
}

inline void ui_refresh_workbench_screen(
    const CmdState &st,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left_nav_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
        &left_worklist_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_main,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_aux) {
  clear_panel_selection_tracking(
      {left, right_main, right_aux, left_nav_panel, left_worklist_panel});
  show_split_left_panels(left, left_nav_panel, left_worklist_panel);
  right_main->visible = true;
  right_aux->visible = false;
  apply_text_panel_model(
      left_nav_panel,
      ui_text_panel_model_t{
          .lines = make_workbench_navigation_styled_lines(st),
          .title = "navigation",
          .focused = workbench_is_navigation_focus(st.workbench.focus),
      });
  apply_text_panel_model(
      left_worklist_panel,
      ui_text_panel_model_t{
          .lines = make_workbench_worklist_styled_lines(st),
          .selected_line = selected_workbench_worklist_line(st),
          .reset_horizontal_scroll = true,
          .title = workbench_section_title(st),
          .focused = workbench_is_worklist_focus(st.workbench.focus),
      });
  set_text_box_styled_lines(right_main, make_workbench_right_styled_lines(st),
                            true);
  right_main->style.title = " details ";
}

inline void ui_refresh_runtime_screen(
    const CmdState &st,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left_nav_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
        &left_worklist_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_main,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_aux) {
  clear_selected_text_row_tracking(left);
  if (!runtime_event_viewer_is_open(st)) {
    clear_selected_text_row_tracking(right_main);
  }
  clear_selected_text_row_tracking(right_aux);
  clear_panel_selection_tracking({left_nav_panel, left_worklist_panel});
  show_split_left_panels(left, left_nav_panel, left_worklist_panel);
  right_main->visible = true;
  right_aux->visible = runtime_show_secondary_panel(st);
  const auto runtime_worklist = make_runtime_worklist_panel(st);
  apply_text_panel_model(
      left_nav_panel,
      ui_text_panel_model_t{
          .lines = make_runtime_navigation_styled_lines(st),
          .title = "navigator",
          .focused = runtime_is_menu_focus(st.runtime.focus),
      });
  apply_text_panel_model(
      left_worklist_panel,
      ui_text_panel_model_t{
          .lines = runtime_worklist.lines,
          .selected_line = runtime_worklist.selected_line,
          .reset_horizontal_scroll = true,
          .title = runtime_worklist_panel_title(st),
          .focused = runtime_is_worklist_focus(st.runtime.focus),
      });
  if (runtime_text_log_viewer_is_open(st) && st.runtime.log_viewer) {
    set_editor_box(right_main, st.runtime.log_viewer);
  } else {
    if (runtime_event_viewer_is_open(st)) {
      const auto event_panel = make_runtime_event_viewer_panel(st);
      set_text_box_styled_lines(right_main, event_panel.lines, true);
      if (auto tb = as<cuwacunu::iinuji::textBox_data_t>(
              panel_content_target(right_main));
          tb && event_panel.selected_line.has_value()) {
        tb->scroll_x = 0;
        const Rect r = content_rect(*right_main);
        const auto metrics =
            measure_runtime_styled_box(event_panel.lines, r.w, r.h,
                                       /*wrap=*/true, event_panel.selected_line);
        if (metrics.text_h > 0) {
          reveal_selected_text_row_if_changed(
              right_main, metrics.selected_row, metrics.text_h,
              metrics.max_scroll_y);
        }
      } else {
        clear_selected_text_row_tracking(right_main);
      }
    } else {
      set_text_box_styled_lines(right_main,
                                make_runtime_primary_styled_lines(st), true);
      clear_selected_text_row_tracking(right_main);
    }
    if (runtime_show_device_history_panel(st)) {
      const auto series = make_runtime_device_history_series(st);
      if (series.empty()) {
        set_text_box(right_aux, "collecting device history...", true);
      } else {
        set_plot_box(right_aux, series, make_runtime_device_history_plot_cfg(st),
                     make_runtime_device_history_plot_opts());
      }
    }
  }
  right_main->style.title = runtime_primary_panel_title(st);
  if (runtime_show_secondary_panel(st)) {
    right_main->style.title = runtime_primary_panel_title(st);
    right_aux->style.title = runtime_secondary_panel_title(st);
  }
}

inline void ui_refresh_lattice_screen(
    const CmdState &st,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left_nav_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
        &left_worklist_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_main,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_aux) {
  clear_panel_selection_tracking({left, left_nav_panel, right_main, right_aux});
  show_split_left_panels(left, left_nav_panel, left_worklist_panel);
  right_main->visible = true;
  right_aux->visible = false;
  const auto navigation_lines = make_lattice_navigation_styled_lines(st);
  const auto worklist_panel = make_lattice_worklist_panel(st);
  apply_text_panel_model(
      left_nav_panel,
      ui_text_panel_model_t{
          .lines = navigation_lines,
          .reset_horizontal_scroll = true,
          .title = "sections",
          .focused = lattice_is_navigator_focus(st),
      });
  apply_text_panel_model(
      left_worklist_panel,
      ui_text_panel_model_t{
          .lines = worklist_panel.lines,
          .selected_line = worklist_panel.selected_line,
          .reset_horizontal_scroll = true,
          .title = lattice_worklist_panel_title(st),
          .focused = lattice_is_worklist_focus(st),
      });
  set_text_box_styled_lines(right_main, make_lattice_right_styled_lines(st),
                            true);
  right_main->style.title = " " + lattice_primary_panel_title(st) + " ";
}

inline void ui_refresh_logs_screen(
    const CmdState &st,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left_nav_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
        &left_worklist_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_main,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_aux) {
  clear_panel_selection_tracking({left, right_main, right_aux});
  show_single_left_panel(left, left_nav_panel, left_worklist_panel);
  right_main->visible = true;
  right_aux->visible = false;
  const auto snap = cuwacunu::piaabo::dlog_snapshot();
  set_text_box_styled_lines(left, make_logs_left_styled_lines(st.shell_logs, snap),
                            false);
  set_text_box(right_main, make_logs_right(st.shell_logs, snap), true);
  left->style.title = workspace_is_current_screen_zoomed(st)
                          ? " shell log stream [full] "
                          : " shell log stream ";
  right_main->style.title = " shell log control deck ";
}

inline void ui_refresh_config_screen(
    const CmdState &st,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left_nav_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
        &left_worklist_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_main,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_aux) {
  if (st.workbench.editor_return.active && st.config.editor_focus &&
      st.config.editor) {
    clear_panel_selection_tracking(
        {left, right_main, right_aux, left_nav_panel, left_worklist_panel});
    show_single_left_panel(left, left_nav_panel, left_worklist_panel);
    left->focusable = true;
    left->focused = true;
    right_main->visible = false;
    right_aux->visible = false;
    set_editor_box(left, st.config.editor);
    if (auto target = panel_content_target(left); target) {
      target->focusable = true;
      target->focused = true;
    }
    const auto *file =
        selected_config_file_for_family(st, st.config.selected_family);
    const bool editable = file != nullptr && file->editable;
    left->style.title = editable ? " file [edit] " : " file [read only] ";
    return;
  }
  clear_panel_selection_tracking(
      {left, right_main, right_aux, left_nav_panel, left_worklist_panel});
  show_split_left_panels(left, left_nav_panel, left_worklist_panel);
  right_main->visible = true;
  right_aux->visible = false;
  const auto families_panel = make_config_families_panel(st);
  const auto files_panel = make_config_files_panel(st);
  const auto detail_panel = make_config_right_panel(st);
  apply_text_panel_model(
      left_nav_panel,
      ui_text_panel_model_t{
          .lines = families_panel.lines,
          .selected_line = families_panel.selected_line,
          .title = "families",
          .focused = config_is_family_focus(st.config),
      });
  apply_text_panel_model(
      left_worklist_panel,
      ui_text_panel_model_t{
          .lines = files_panel.lines,
          .selected_line = files_panel.selected_line,
          .title = "files [" + config_family_title(st.config.selected_family) +
                   "]",
          .focused = config_is_file_focus(st.config),
      });
  if (st.config.editor_focus && st.config.editor) {
    right_main->focusable = true;
    right_main->focused = true;
    set_editor_box(right_main, st.config.editor);
    if (auto target = panel_content_target(right_main); target) {
      target->focusable = true;
      target->focused = true;
    }
  } else {
    set_text_box_styled_lines(right_main, detail_panel.lines, false);
  }
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

inline void ui_refresh_panels(
    const CmdState &st,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &title,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &status,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left_nav_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
        &left_worklist_panel,
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
  status->visible = false;
  left->focusable = false;
  left->focused = false;
  left_nav_panel->focusable = false;
  left_nav_panel->focused = false;
  left_worklist_panel->focusable = false;
  left_worklist_panel->focused = false;

  title->style.label_color = "#EDEDED";
  title->style.background_color = "#202028";
  title->style.border_color = "#6C6C75";
  status->style.label_color = "#B8B8BF";
  status->style.background_color = "#101014";
  status->style.border_color = "#101014";
  left->style.label_color = "#D0D0D0";
  left->style.background_color = "#101014";
  left->style.border_color = "#5E5E68";
  left_nav_panel->style.label_color = "#D0D0D0";
  left_nav_panel->style.background_color = "#101014";
  left_nav_panel->style.border_color = "#5E5E68";
  left_worklist_panel->style.label_color = "#D0D0D0";
  left_worklist_panel->style.background_color = "#101014";
  left_worklist_panel->style.border_color = "#5E5E68";
  right_main->style.label_color = "#C8C8CE";
  right_main->style.background_color = "#101014";
  right_main->style.border_color = "#5E5E68";
  right_aux->style.label_color = "#C8C8CE";
  right_aux->style.background_color = "#101014";
  right_aux->style.border_color = "#5E5E68";
  bottom->style.background_color = "#1E1B18";
  bottom->style.border_color = "#4E473E";
  cmdline->style.label_color = "#E8E8E8";
  cmdline->style.background_color = "#101014";
  cmdline->style.border_color = "#5E5E68";

  const bool lattice_is_loading =
      st.screen == ScreenMode::Lattice && lattice_is_visibly_loading(st);
  const bool bottom_is_error =
      (st.screen == ScreenMode::Workbench && st.workbench.status_is_error) ||
      (st.screen == ScreenMode::Runtime && st.runtime.status_is_error) ||
      (st.screen == ScreenMode::Lattice && st.lattice.status_is_error) ||
      (st.screen == ScreenMode::Config &&
       (!st.config.ok || st.config.status_is_error));
  if (bottom_is_error) {
    bottom->style.label_color = "#C98C83";
  } else if (lattice_is_loading &&
             st.lattice.refresh_mode == LatticeRefreshMode::SyncStore) {
    bottom->style.label_color = "#C6B27A";
  } else if (lattice_is_loading) {
    bottom->style.label_color = "#8DAEC4";
  } else {
    bottom->style.label_color = "#B3A99B";
  }
  status->style.label_color = "#B8B8BF";
  set_text_box(bottom, ui_bottom_line(st), false);

  switch (st.screen) {
  case ScreenMode::Home:
    ui_refresh_home_screen(st, title, status, left, left_nav_panel,
                           left_worklist_panel, right_main, right_aux, bottom,
                           cmdline);
    break;
  case ScreenMode::Workbench:
    ui_refresh_workbench_screen(st, left, left_nav_panel, left_worklist_panel,
                                right_main, right_aux);
    break;
  case ScreenMode::Runtime:
    ui_refresh_runtime_screen(st, left, left_nav_panel, left_worklist_panel,
                              right_main, right_aux);
    break;
  case ScreenMode::Lattice:
    ui_refresh_lattice_screen(st, left, left_nav_panel, left_worklist_panel,
                              right_main, right_aux);
    break;
  case ScreenMode::ShellLogs:
    ui_refresh_logs_screen(st, left, left_nav_panel, left_worklist_panel,
                           right_main, right_aux);
    break;
  case ScreenMode::Config:
    ui_refresh_config_screen(st, left, left_nav_panel, left_worklist_panel,
                             right_main, right_aux);
    break;
  }

  set_text_box_styled_lines(title, ui_title_styled_lines(st), false);
  set_text_box_styled_lines(status, ui_status_styled_lines(st), false);
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
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left_nav_panel,
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
          &left_worklist_panel,
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_main,
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_aux,
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &bottom,
      const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &cmdline) const {
    ui_refresh_panels(st, title, status, left, left_nav_panel,
                      left_worklist_panel, right_main, right_aux, bottom,
                      cmdline);
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
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left_nav_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left_worklist_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_main,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &right_aux,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &bottom,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &cmdline) {
  IinujiUi{st}.refresh(title, status, left, left_nav_panel, left_worklist_panel,
                       right_main, right_aux, bottom, cmdline);
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
