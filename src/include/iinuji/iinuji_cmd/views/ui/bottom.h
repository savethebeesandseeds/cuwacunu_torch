#pragma once

#include <sstream>
#include <string>
#include <string_view>

#include "iinuji/iinuji_cmd/views/common.h"
#include "iinuji/iinuji_cmd/views/lattice/commands.h"
#include "iinuji/iinuji_cmd/views/logs/view.h"
#include "iinuji/iinuji_cmd/views/runtime/commands.h"
#include "iinuji/iinuji_cmd/views/workbench/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class UiEventfulStatusLogSeverity {
  None = 0,
  Debug = 1,
  Info = 2,
  Warning = 3,
  Error = 4,
};

inline std::string home_default_bottom_line(const CmdState &) {
  return "home | ready | " + ui_screen_hint(ScreenMode::Workbench) + " | " +
         ui_screen_hint(ScreenMode::Runtime) + " | " +
         ui_screen_hint(ScreenMode::Lattice) + " | " +
         ui_screen_hint(ScreenMode::ShellLogs) + " | " +
         ui_screen_hint(ScreenMode::Config) + " | waajacu.com";
}

inline std::string workbench_bottom_row_summary(const CmdState &st) {
  std::size_t selected = 0;
  std::size_t total = 0;
  const auto rows = operator_action_queue_rows(st);
  total = rows.size();
  selected =
      total == 0 ? 0 : std::min(st.workbench.selected_session_row, total - 1);

  std::ostringstream oss;
  oss << "screen=" << ui_screen_name(ScreenMode::Workbench)
      << " | section=Inbox";
  if (total > 0) {
    oss << " | row=" << (selected + 1) << "/" << total;
  } else {
    oss << " | row=0/0";
  }
  return oss.str();
}

inline std::string workbench_default_bottom_line(const CmdState &st) {
  const std::string section = workbench_section_label(st.workbench.section);
  if (workbench_is_navigation_focus(st.workbench.focus)) {
    return "workbench | section=" + section +
           " | Up/Down choose Inbox or Booster. Enter focuses the selected "
           "section.";
  }
  if (workbench_is_booster_section(st.workbench.section)) {
    return "workbench | section=Booster | action=" +
           workbench_booster_action_label(
               selected_workbench_booster_action(st)) +
           " | Up/Down browse actions. Enter runs the guided flow. Esc returns "
           "to navigation.";
  }
  if (operator_action_queue_rows(st).empty()) {
    return "screen=" + std::string(ui_screen_name(ScreenMode::Workbench)) +
           " | section=Inbox | row=0/0 | no operator items are waiting.";
  }
  return workbench_bottom_row_summary(st) +
         " | Enter opens row actions. Esc returns to navigation.";
}

inline std::string runtime_default_bottom_line(const CmdState &st) {
  if (runtime_log_viewer_is_open(st)) {
    if (runtime_event_viewer_is_open(st)) {
      return "viewer=" +
             runtime_log_viewer_kind_label(st.runtime.log_viewer_kind) +
             (st.runtime.log_viewer_live_follow ? " | live=follow"
                                                : " | live=hold") +
             " | Up/Down selects event cards. PgUp/PgDn jump. Home jumps to "
             "the first event. End snaps back to live. t toggles follow. f "
             "widens panel. Esc closes. r reloads file.";
    }
    if (!runtime_log_viewer_supports_follow(st)) {
      return "viewer=" + runtime_log_viewer_title(st) +
             " | Up/Down, Left/Right, Home/End, PgUp/PgDn browse. f widens "
             "panel. Esc closes. r reloads file.";
    }
    return "viewer=" +
           runtime_log_viewer_kind_label(st.runtime.log_viewer_kind) +
           (st.runtime.log_viewer_live_follow ? " | live=follow"
                                              : " | live=hold") +
           " | Up/Down, Left/Right, Home/End, PgUp/PgDn browse. t toggles live "
           "tail. f widens panel. Esc closes. r reloads file.";
  }
  const std::string lane = runtime_lane_label(st.runtime.lane);
  if (runtime_is_menu_focus(st.runtime.focus)) {
    return "lane=" + lane +
           " | Up/Down or Left/Right selects lane. Enter focuses worklist. Tab "
           "cycles manifest/log/trace. f widens detail panel. r refreshes "
           "runtime inventory.";
  }
  if (runtime_is_device_lane(st.runtime.lane)) {
    return "lane=" + lane +
           " | Up/Down browses host and gpu rows. Tab changes the "
           "selected-device pane mode. lower panel shows live history. f "
           "widens detail panel. Esc returns to navigator.";
  }
  if (runtime_is_session_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active) {
      return "lane=" + lane +
             " | Up/Down browses jobs for the selected campaign. Left returns "
             "to campaigns. Enter or a opens actions. f widens detail panel. "
             "Esc returns to navigator.";
    }
    if (st.runtime.campaign_filter_active) {
      return "lane=" + lane +
             " | Up/Down browses campaigns for the selected marshal. Right "
             "enters jobs for the selected campaign. Left returns to marshals. "
             "Enter or a opens actions. f widens detail panel. Esc returns to "
             "navigator.";
    }
    return "lane=" + lane +
           " | Up/Down browses marshals. Right enters campaigns for the "
           "selected marshal. Enter or a opens actions. Tab cycles "
           "manifest/log/trace. f widens detail panel. Esc returns to "
           "navigator.";
  }
  if (runtime_is_campaign_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active) {
      return "lane=" + lane +
             " | Up/Down browses jobs for the selected campaign. Left returns "
             "to campaigns. Enter or a opens actions. f widens detail panel. "
             "Esc returns to navigator.";
    }
    return "lane=" + lane +
           " | Up/Down browses campaigns. Right enters jobs for the selected "
           "campaign. Enter or a opens actions. Tab cycles manifest/log/trace. "
           "f widens detail panel. Esc returns to navigator.";
  }
  return "lane=" + lane +
         " | Up/Down browses jobs. Enter or a opens actions. Tab cycles "
         "manifest/log/trace. f widens detail panel. Esc returns to navigator.";
}

inline std::string lattice_default_bottom_line(const CmdState &st) {
  const std::string section =
      lattice_section_label(st.lattice.selected_section);
  if (lattice_is_navigator_focus(st)) {
    return "section=" + section +
           " | Up/Down picks overview, components, or lattice. Enter opens the "
           "section. r refreshes Hero catalogs.";
  }
  if (st.lattice.selected_section == LatticeSection::Overview) {
    return "section=" + section +
           " | Esc or Left returns to sections. r refreshes Hero catalogs.";
  }
  if (st.lattice.selected_section == LatticeSection::Components) {
    if (st.lattice.component_rows.empty()) {
      return "section=" + section +
             " | no hashimyei components discovered yet. Enter or r refreshes "
             "catalogs. Esc or Left returns to sections.";
    }
    return "section=" + section + " | row=" +
           std::to_string(std::min(st.lattice.selected_component_row,
                                   st.lattice.component_rows.size() - 1) +
                          1) +
           "/" + std::to_string(st.lattice.component_rows.size()) +
           " | badges=cf/c/f | Up/Down browse the visible layer. Right drills "
           "in. Left climbs out. Esc returns to sections. r refreshes.";
  }
  if (st.lattice.mode == LatticeMode::Roots) {
    return "section=" + section +
           " | Up/Down picks facts or views. Right or Enter focuses the "
           "selected branch while the full worklist stays expanded. Left or "
           "Esc returns to sections. r refreshes catalogs.";
  }
  if (st.lattice.mode == LatticeMode::Views) {
    if (st.lattice.views.empty()) {
      return "section=" + section +
             " | no views reported yet. Full worklist stays expanded. Left or "
             "Esc returns to facts/views. r refreshes catalogs.";
    }
    return "section=" + section + " | row=" +
           std::to_string(std::min(st.lattice.selected_view_index,
                                   st.lattice.views.size() - 1) +
                          1) +
           "/" + std::to_string(st.lattice.views.size()) +
           " | Up/Down browse views in the expanded worklist. Left or Esc "
           "returns to facts/views. r refreshes.";
  }
  if (st.lattice.rows.empty()) {
    return "section=" + section +
           " | no fact rows discovered yet. Components may still exist. Full "
           "worklist stays expanded. Left or Esc returns to facts/views. r "
           "refreshes catalogs.";
  }
  const auto *row = selected_lattice_row_ptr(st);
  const auto *family = selected_lattice_family_ptr(st);
  const auto *project = selected_lattice_project_ptr(st);
  const auto *member = selected_lattice_member_ptr(st);

  std::ostringstream oss;
  oss << "section=" << section;
  oss << " | kind=" << (row ? lattice_row_kind_label(row->kind) : "family");
  oss << " | row="
      << (std::min(st.lattice.selected_row, st.lattice.rows.size() - 1) + 1)
      << "/" << st.lattice.rows.size();
  if (family)
    oss << " | family=" << family->family;
  if (project)
    oss << " | project=" << trim_to_width(project->project_path, 24);
  if (member)
    oss << " | member=" << member->display_name;
  oss << " | mode=" << lattice_source_mode_text(st);
  oss << " | badges=cf/c/f | Up/Down browse facts in the expanded worklist. "
         "Right drills in. Left climbs out or returns to facts/views. Esc "
         "returns to sections. r refreshes.";
  return oss.str();
}

inline std::string logs_default_bottom_line(const CmdState &st) {
  return std::string("logs | filter=") +
         logs_filter_label(st.shell_logs.level_filter) + " | metadata=" +
         logs_metadata_filter_label(st.shell_logs.metadata_filter) +
         (st.shell_logs.auto_follow ? " | follow=on" : " | follow=off") +
         " | f toggles full screen. Esc restores split. Home/End jump "
         "oldest/newest.";
}

inline std::string config_default_bottom_line(const CmdState &st) {
  if (st.config.editor_focus) {
    return "config | file open | Ctrl+S saves when editable. Esc closes file. "
           "Left column keeps families and files visible.";
  }
  if (config_is_family_focus(st.config)) {
    return "config | families focus | Up/Down browse families. Enter moves to "
           "files. r reloads active policy.";
  }
  return "config | files focus | Up/Down browse files in family. Esc returns "
         "to families. Enter or e opens file. r reloads active policy.";
}

inline bool ui_config_status_is_eventful(const ConfigState &st) {
  if (st.status.empty()) {
    return false;
  }
  if (st.status_is_error) {
    return true;
  }
  return st.status != "files focus" && st.status != "families focus";
}

inline bool ui_eventful_status_is_error(const CmdState &st) {
  if (st.screen == ScreenMode::Workbench) {
    return workbench_has_visible_status(st.workbench) &&
           st.workbench.status_is_error;
  }
  if (st.screen == ScreenMode::Runtime) {
    return !st.runtime.status.empty() && st.runtime.status_is_error;
  }
  if (st.screen == ScreenMode::Lattice) {
    return !st.lattice.status.empty() && st.lattice.status_is_error;
  }
  if (st.screen == ScreenMode::Config) {
    if (ui_config_status_is_eventful(st.config)) {
      return st.config.status_is_error;
    }
    return !st.config.ok;
  }
  return false;
}

inline std::string ui_eventful_status_line(const CmdState &st) {
  if (st.screen == ScreenMode::Workbench) {
    return visible_workbench_status(st);
  }
  if (st.screen == ScreenMode::Runtime) {
    return st.runtime.status;
  }
  if (st.screen == ScreenMode::Lattice) {
    return st.lattice.status;
  }
  if (st.screen == ScreenMode::Config) {
    if (ui_config_status_is_eventful(st.config)) {
      return st.config.status;
    }
    if (!st.config.ok) {
      return st.config.error;
    }
  }
  return {};
}

inline const char *ui_eventful_status_log_scope(const CmdState &st) {
  switch (st.screen) {
  case ScreenMode::Workbench:
    return "workbench";
  case ScreenMode::Runtime:
    return "runtime";
  case ScreenMode::Lattice:
    return "lattice";
  case ScreenMode::Config:
    return "config";
  case ScreenMode::Home:
    return "home";
  case ScreenMode::ShellLogs:
    return "logs";
  }
  return "status";
}

inline bool ui_status_is_debug_only(std::string_view status) {
  if (status.empty()) {
    return false;
  }
  if (status == "Cancelled." || status == "Cancelled Marshal launch." ||
      status == "Loading Workbench..." ||
      status == "Loading runtime inventory..." ||
      status == "Loading config view..." ||
      status == "Refreshing runtime inventory..." || status == "files focus" ||
      status == "families focus" || status == "viewer focus | read only" ||
      status == "editor focus | Ctrl+S saves through Config Hero" ||
      status == "editor modified | Ctrl+S saves through Config Hero" ||
      status == "discarded unsaved config changes" ||
      status == "Selected marshal has no campaigns." ||
      status == "Selected campaign has no jobs." ||
      status == "No actions available for the selected row." ||
      status == "Selected action is currently unavailable." ||
      status == "This viewer does not support live-follow." ||
      status == "Runtime log live-follow enabled." ||
      status == "Runtime log live-follow paused." ||
      status == "Closed runtime log viewer." ||
      status == "Expanded runtime detail panel." ||
      status == "Restored split runtime panel layout.") {
    return true;
  }
  if (status.starts_with(
          "Refreshing hero.lattice and hero.hashimyei catalogs;") ||
      status.starts_with("Syncing hero.lattice and hero.hashimyei catalogs") ||
      status.starts_with("Loading lattice components, facts, and views...")) {
    return true;
  }
  if (status.starts_with("Opened ") &&
      status.ends_with(" in the right panel.")) {
    return true;
  }
  if (status.starts_with("Reloaded ")) {
    return true;
  }
  return false;
}

inline bool ui_status_is_warning(std::string_view status) {
  if (status.empty()) {
    return false;
  }
  return status.starts_with("Runtime inventory partially loaded.") ||
         status.starts_with("Lattice refresh skipped;") ||
         status.starts_with("Lattice member detail unavailable;") ||
         status.starts_with("selected marshal write scope unavailable:");
}

inline UiEventfulStatusLogSeverity
ui_eventful_status_log_severity(const CmdState &st) {
  const std::string status = ui_eventful_status_line(st);
  if (status.empty()) {
    return UiEventfulStatusLogSeverity::None;
  }
  if (ui_status_is_debug_only(status)) {
    return UiEventfulStatusLogSeverity::Debug;
  }
  if (ui_status_is_warning(status)) {
    return UiEventfulStatusLogSeverity::Warning;
  }
  if (ui_eventful_status_is_error(st)) {
    return UiEventfulStatusLogSeverity::Error;
  }
  return UiEventfulStatusLogSeverity::Info;
}

inline std::string ui_bottom_line(const CmdState &st) {
  if (st.screen == ScreenMode::Home) {
    return home_default_bottom_line(st);
  }
  if (st.screen == ScreenMode::Workbench) {
    const std::string status = visible_workbench_status(st);
    if (!status.empty())
      return status;
    return workbench_default_bottom_line(st);
  }
  if (st.screen == ScreenMode::Runtime) {
    if (!st.runtime.status.empty())
      return st.runtime.status;
    return runtime_default_bottom_line(st);
  }
  if (st.screen == ScreenMode::Lattice) {
    if (!st.lattice.status.empty())
      return st.lattice.status;
    return lattice_default_bottom_line(st);
  }
  if (st.screen == ScreenMode::ShellLogs) {
    return logs_default_bottom_line(st);
  }
  if (st.screen == ScreenMode::Config) {
    if (!st.config.status.empty())
      return st.config.status;
    if (!st.config.ok)
      return st.config.error;
    return config_default_bottom_line(st);
  }
  return {};
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
