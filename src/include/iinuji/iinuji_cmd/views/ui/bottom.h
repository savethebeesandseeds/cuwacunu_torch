#pragma once

#include <string>

#include "iinuji/iinuji_cmd/views/common.h"
#include "iinuji/iinuji_cmd/views/human/commands.h"
#include "iinuji/iinuji_cmd/views/lattice/commands.h"
#include "iinuji/iinuji_cmd/views/logs/view.h"
#include "iinuji/iinuji_cmd/views/runtime/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string home_default_bottom_line(const CmdState &) {
  return "home | ready | F2 marshal | F3 runtime | F4 lattice | F8 shell logs "
         "| F9 config | waajacu.com";
}

inline std::string human_bottom_row_summary(const CmdState &st) {
  std::size_t selected = 0;
  std::size_t total = 0;
  const auto rows = human_inbox_session_rows(st);
  total = rows.size();
  selected =
      total == 0 ? 0 : std::min(st.human.selected_inbox_session, total - 1);

  std::ostringstream oss;
  oss << "lane=Inbox";
  if (total > 0) {
    oss << " | row=" << (selected + 1) << "/" << total;
  } else {
    oss << " | row=0/0";
  }
  return oss.str();
}

inline std::string human_default_bottom_line(const CmdState &st) {
  if (human_inbox_session_rows(st).empty()) {
    return "lane=Inbox | row=0/0";
  }
  return human_bottom_row_summary(st);
}

inline std::string runtime_default_bottom_line(const CmdState &st) {
  if (runtime_log_viewer_is_open(st)) {
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
           " | Up/Down browse the visible layer. Right drills in. Left climbs "
           "out. Esc returns to sections. r refreshes.";
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
  oss << " | Up/Down browse facts in the expanded worklist. Right drills in. "
         "Left climbs out or returns to facts/views. Esc returns to sections. "
         "r refreshes.";
  return oss.str();
}

inline std::string logs_default_bottom_line(const CmdState &st) {
  return std::string("logs | filter=") +
         logs_filter_label(st.shell_logs.level_filter) + " | metadata=" +
         logs_metadata_filter_label(st.shell_logs.metadata_filter) +
         (st.shell_logs.auto_follow ? " | follow=on" : " | follow=off") +
         " | f toggles full stream. Esc restores split. Home/End jump "
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

inline std::string ui_bottom_line(const CmdState &st) {
  if (st.screen == ScreenMode::Home) {
    return home_default_bottom_line(st);
  }
  if (st.screen == ScreenMode::Human) {
    const std::string status = visible_human_status(st);
    if (!status.empty())
      return status;
    return human_default_bottom_line(st);
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
