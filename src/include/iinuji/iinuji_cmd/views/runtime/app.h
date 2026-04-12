#pragma once

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/views/runtime/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class RuntimePopupActionKind : std::uint8_t {
  PauseSession = 0,
  ResumeSession = 1,
  ContinueSession = 2,
  TerminateSession = 3,
  ViewMarshalEvents = 4,
  ViewMarshalCodexStdout = 5,
  ViewMarshalCodexStderr = 6,
  StopCampaign = 7,
  ForceStopCampaign = 8,
  ViewJobStdout = 9,
  ViewJobStderr = 10,
  StopJob = 11,
  ForceStopJob = 12,
};

struct RuntimePopupAction {
  RuntimePopupActionKind kind;
  std::string label;
};

inline RuntimeRowKind runtime_popup_row_kind(const CmdState &st) {
  if (runtime_is_session_lane(st.runtime.lane) &&
      st.runtime.job_filter_active && st.runtime.job.has_value()) {
    return RuntimeRowKind::Job;
  }
  if (runtime_is_session_lane(st.runtime.lane) &&
      st.runtime.campaign_filter_active && st.runtime.campaign.has_value()) {
    return RuntimeRowKind::Campaign;
  }
  if (runtime_is_campaign_lane(st.runtime.lane) &&
      st.runtime.job_filter_active && st.runtime.job.has_value()) {
    return RuntimeRowKind::Job;
  }
  return st.runtime.lane;
}

inline bool drill_runtime_into_campaigns(CmdState &st) {
  const auto visible = runtime_child_campaign_indices(st);
  if (visible.empty()) {
    set_runtime_status(st, "Selected marshal has no campaigns.", false);
    return true;
  }
  st.runtime.campaign_filter_active = true;
  st.runtime.job_filter_active = false;
  const std::string campaign_cursor =
      st.runtime.campaigns[visible.front()].campaign_cursor;
  select_runtime_campaign_and_related(st, campaign_cursor);
  focus_runtime_worklist(st);
  return true;
}

inline bool drill_runtime_into_jobs(CmdState &st) {
  const auto visible = runtime_child_job_indices(st);
  if (visible.empty()) {
    set_runtime_status(st, "Selected campaign has no jobs.", false);
    return true;
  }
  st.runtime.job_filter_active = true;
  const std::string job_cursor = st.runtime.jobs[visible.front()].job_cursor;
  select_runtime_job_and_related(st, job_cursor);
  focus_runtime_worklist(st);
  return true;
}

inline bool exit_runtime_nested_rows(CmdState &st) {
  if (runtime_is_session_lane(st.runtime.lane) &&
      st.runtime.job_filter_active) {
    st.runtime.job_filter_active = false;
    return true;
  }
  if (runtime_is_session_lane(st.runtime.lane) &&
      st.runtime.campaign_filter_active) {
    st.runtime.campaign_filter_active = false;
    return true;
  }
  if (runtime_is_campaign_lane(st.runtime.lane) &&
      st.runtime.job_filter_active) {
    st.runtime.job_filter_active = false;
    return true;
  }
  return false;
}

inline bool runtime_apply_marshal_action(CmdState &st,
                                         const std::string &tool_name,
                                         const std::string &arguments_json,
                                         std::string success_status) {
  cmd_json_value_t structured{};
  std::string error{};
  if (!runtime_invoke_marshal_tool(st, tool_name, arguments_json, &structured,
                                   &error)) {
    set_runtime_status(st, error.empty() ? "marshal action failed" : error,
                       true);
    return true;
  }
  (void)queue_runtime_refresh(st);
  set_runtime_status(st, std::move(success_status), false);
  return true;
}

inline bool runtime_apply_runtime_action(CmdState &st,
                                         const std::string &tool_name,
                                         const std::string &arguments_json,
                                         std::string success_status) {
  cmd_json_value_t structured{};
  std::string error{};
  if (!runtime_invoke_runtime_tool(st, tool_name, arguments_json, &structured,
                                   &error)) {
    set_runtime_status(st, error.empty() ? "runtime action failed" : error,
                       true);
    return true;
  }
  (void)queue_runtime_refresh(st);
  set_runtime_status(st, std::move(success_status), false);
  return true;
}

inline std::vector<RuntimePopupAction>
runtime_popup_actions(const CmdState &st) {
  std::vector<RuntimePopupAction> actions{};
  const RuntimeRowKind row_kind = runtime_popup_row_kind(st);
  if (row_kind == RuntimeRowKind::Session && st.runtime.session.has_value()) {
    const auto &session = *st.runtime.session;
    if (runtime_probe_log_viewer_file(session.events_path).ok) {
      actions.push_back(
          {RuntimePopupActionKind::ViewMarshalEvents, "View marshal events"});
    }
    if (runtime_probe_log_viewer_file(session.codex_stdout_path).ok) {
      actions.push_back({RuntimePopupActionKind::ViewMarshalCodexStdout,
                         "View codex stdout"});
    }
    if (runtime_probe_log_viewer_file(session.codex_stderr_path).ok) {
      actions.push_back({RuntimePopupActionKind::ViewMarshalCodexStderr,
                         "View codex stderr"});
    }
    if (session.phase == "active" || session.phase == "running_campaign") {
      actions.push_back(
          {RuntimePopupActionKind::PauseSession, "Pause marshal"});
    }
    if (session.phase == "paused" && session.pause_kind == "operator") {
      actions.push_back(
          {RuntimePopupActionKind::ResumeSession, "Resume marshal"});
    }
    if (session.phase == "idle") {
      actions.push_back(
          {RuntimePopupActionKind::ContinueSession, "Continue marshal"});
    }
    if (session.phase != "finished") {
      actions.push_back(
          {RuntimePopupActionKind::TerminateSession, "Terminate marshal"});
    }
    return actions;
  }
  if (row_kind == RuntimeRowKind::Campaign && st.runtime.campaign.has_value()) {
    if (runtime_campaign_is_active(*st.runtime.campaign)) {
      actions.push_back(
          {RuntimePopupActionKind::StopCampaign, "Stop campaign"});
      actions.push_back(
          {RuntimePopupActionKind::ForceStopCampaign, "Force stop campaign"});
    }
    return actions;
  }
  if (row_kind == RuntimeRowKind::Job && st.runtime.job.has_value()) {
    if (!st.runtime.job->stdout_path.empty()) {
      actions.push_back(
          {RuntimePopupActionKind::ViewJobStdout, "View job stdout.log"});
    }
    if (!st.runtime.job->stderr_path.empty()) {
      actions.push_back(
          {RuntimePopupActionKind::ViewJobStderr, "View job stderr.log"});
    }
    if (runtime_job_is_active(*st.runtime.job)) {
      actions.push_back({RuntimePopupActionKind::StopJob, "Stop job"});
      actions.push_back(
          {RuntimePopupActionKind::ForceStopJob, "Force stop job"});
    }
  }
  return actions;
}

inline bool dispatch_runtime_popup_action(CmdState &st,
                                          const RuntimePopupAction &action) {
  switch (action.kind) {
  case RuntimePopupActionKind::PauseSession: {
    if (!st.runtime.session.has_value())
      return false;
    return runtime_apply_marshal_action(
        st, "hero.marshal.pause_session",
        std::string("{\"marshal_session_id\":") +
            config_json_quote(st.runtime.session->marshal_session_id) + "}",
        "Paused selected marshal.");
  }
  case RuntimePopupActionKind::ResumeSession: {
    if (!st.runtime.session.has_value())
      return false;
    return runtime_apply_marshal_action(
        st, "hero.marshal.resume_session",
        std::string("{\"marshal_session_id\":") +
            config_json_quote(st.runtime.session->marshal_session_id) + "}",
        "Resumed selected marshal.");
  }
  case RuntimePopupActionKind::ContinueSession: {
    if (!st.runtime.session.has_value())
      return false;
    std::string instruction{};
    bool cancelled = false;
    if (!cuwacunu::hero::human_mcp::prompt_text_dialog(
            " Continue marshal ",
            "Provide the operator instruction that should launch more work for "
            "this idle marshal.",
            &instruction, false, false, &cancelled)) {
      set_runtime_status(st, "failed to collect continuation instruction",
                         true);
      return true;
    }
    if (cancelled) {
      set_runtime_status(st, "Cancelled.", false);
      return true;
    }
    return runtime_apply_marshal_action(
        st, "hero.marshal.continue_session",
        std::string("{\"marshal_session_id\":") +
            config_json_quote(st.runtime.session->marshal_session_id) +
            ",\"instruction\":" + config_json_quote(instruction) + "}",
        "Continued selected marshal.");
  }
  case RuntimePopupActionKind::TerminateSession: {
    if (!st.runtime.session.has_value())
      return false;
    bool confirmed = false;
    bool cancelled = false;
    if (!cuwacunu::hero::human_mcp::prompt_yes_no_dialog(
            " Terminate marshal ",
            "Terminate the selected marshal? This action is final.", false,
            &confirmed, &cancelled)) {
      set_runtime_status(
          st, "failed to collect marshal termination confirmation", true);
      return true;
    }
    if (cancelled || !confirmed) {
      set_runtime_status(st, "Cancelled.", false);
      return true;
    }
    return runtime_apply_marshal_action(
        st, "hero.marshal.terminate_session",
        std::string("{\"marshal_session_id\":") +
            config_json_quote(st.runtime.session->marshal_session_id) + "}",
        "Terminated selected marshal.");
  }
  case RuntimePopupActionKind::ViewMarshalEvents: {
    return runtime_open_session_log_viewer(st,
                                           RuntimeLogViewerKind::MarshalEvents);
  }
  case RuntimePopupActionKind::ViewMarshalCodexStdout: {
    return runtime_open_session_log_viewer(
        st, RuntimeLogViewerKind::MarshalCodexStdout);
  }
  case RuntimePopupActionKind::ViewMarshalCodexStderr: {
    return runtime_open_session_log_viewer(
        st, RuntimeLogViewerKind::MarshalCodexStderr);
  }
  case RuntimePopupActionKind::StopCampaign: {
    if (!st.runtime.campaign.has_value())
      return false;
    return runtime_apply_runtime_action(
        st, "hero.runtime.stop_campaign",
        std::string("{\"campaign_cursor\":") +
            config_json_quote(st.runtime.campaign->campaign_cursor) + "}",
        "Stopped selected campaign.");
  }
  case RuntimePopupActionKind::ForceStopCampaign: {
    if (!st.runtime.campaign.has_value())
      return false;
    bool confirmed = false;
    bool cancelled = false;
    if (!cuwacunu::hero::human_mcp::prompt_yes_no_dialog(
            " Force stop campaign ",
            "Force-stop the selected campaign and its active child job when "
            "present?",
            false, &confirmed, &cancelled)) {
      set_runtime_status(st, "failed to collect force-stop confirmation", true);
      return true;
    }
    if (cancelled || !confirmed) {
      set_runtime_status(st, "Cancelled.", false);
      return true;
    }
    return runtime_apply_runtime_action(
        st, "hero.runtime.stop_campaign",
        std::string("{\"campaign_cursor\":") +
            config_json_quote(st.runtime.campaign->campaign_cursor) +
            ",\"force\":true}",
        "Force-stopped selected campaign.");
  }
  case RuntimePopupActionKind::ViewJobStdout: {
    return runtime_open_job_log_viewer(st, RuntimeLogViewerKind::JobStdout);
  }
  case RuntimePopupActionKind::ViewJobStderr: {
    return runtime_open_job_log_viewer(st, RuntimeLogViewerKind::JobStderr);
  }
  case RuntimePopupActionKind::StopJob: {
    if (!st.runtime.job.has_value())
      return false;
    return runtime_apply_runtime_action(
        st, "hero.runtime.stop_job",
        std::string("{\"job_cursor\":") +
            config_json_quote(st.runtime.job->job_cursor) + "}",
        "Stopped selected job.");
  }
  case RuntimePopupActionKind::ForceStopJob: {
    if (!st.runtime.job.has_value())
      return false;
    bool confirmed = false;
    bool cancelled = false;
    if (!cuwacunu::hero::human_mcp::prompt_yes_no_dialog(
            " Force stop job ", "Force-stop the selected job?", false,
            &confirmed, &cancelled)) {
      set_runtime_status(st, "failed to collect force-stop confirmation", true);
      return true;
    }
    if (cancelled || !confirmed) {
      set_runtime_status(st, "Cancelled.", false);
      return true;
    }
    return runtime_apply_runtime_action(
        st, "hero.runtime.stop_job",
        std::string("{\"job_cursor\":") +
            config_json_quote(st.runtime.job->job_cursor) + ",\"force\":true}",
        "Force-stopped selected job.");
  }
  }
  return false;
}

inline bool open_runtime_action_menu(CmdState &st) {
  const auto actions = runtime_popup_actions(st);
  if (actions.empty()) {
    set_runtime_status(st, "No actions available for the selected row.", false);
    return true;
  }

  std::vector<std::string> options{};
  options.reserve(actions.size());
  for (const auto &action : actions)
    options.push_back(action.label);

  std::size_t selected = 0;
  bool cancelled = false;
  if (!cuwacunu::hero::human_mcp::prompt_choice_dialog(
          " Runtime actions ", "Select action.", options, &selected,
          &cancelled)) {
    set_runtime_status(st, "failed to open action menu", true);
    return true;
  }
  if (cancelled) {
    set_runtime_status(st, {}, false);
    return true;
  }
  return dispatch_runtime_popup_action(
      st, actions[std::min(selected, actions.size() - 1)]);
}

inline bool handle_runtime_key(CmdState &st, int ch) {
  if (st.screen != ScreenMode::Runtime)
    return false;

  auto toggle_zoom = [&]() {
    if (!workspace_toggle_current_screen_zoom(st))
      return false;
    set_runtime_status(st,
                       workspace_is_current_screen_zoomed(st)
                           ? "Expanded runtime detail panel."
                           : "Restored split runtime panel layout.",
                       false);
    return true;
  };
  auto restore_zoom = [&]() {
    if (!workspace_restore_current_screen_split(st))
      return false;
    set_runtime_status(st, "Restored split runtime panel layout.", false);
    return true;
  };

  if (runtime_log_viewer_is_open(st) && st.runtime.log_viewer) {
    if (ch == 27 && workspace_is_current_screen_zoomed(st)) {
      return restore_zoom();
    }
    if (ch == 'f' || ch == 'F') {
      return toggle_zoom();
    }
    if (ch == 't' || ch == 'T') {
      return runtime_toggle_log_viewer_follow(st);
    }
    if (ch == 'r' || ch == 'R') {
      return runtime_reload_log_viewer(st);
    }

    const auto result = cuwacunu::iinuji::primitives::editor_handle_key(
        *st.runtime.log_viewer, ch,
        cuwacunu::iinuji::primitives::editor_keymap_options_t{
            .allow_save = false,
            .allow_reload = false,
            .allow_history = false,
            .allow_close = true,
            .allow_newline = false,
            .allow_tab_indent = false,
            .allow_printable_insert = false,
            .escape_requests_close = true,
        });
    if (result.close_requested) {
      clear_runtime_log_viewer(st);
      set_runtime_status(st, "Closed runtime log viewer.", false);
      return true;
    }
    if (!result.status.empty()) {
      set_runtime_status(st, result.status, false);
    }
    runtime_refresh_log_viewer_follow_state(st);
    return true;
  }
  if (ch == 27 && workspace_is_current_screen_zoomed(st)) {
    return restore_zoom();
  }
  if (ch == 'f' || ch == 'F') {
    return toggle_zoom();
  }
  if (ch == 'r' || ch == 'R') {
    set_runtime_status(st, "Refreshing runtime inventory...", false);
    (void)queue_runtime_refresh(st);
    return true;
  }
  if (ch == '\t' || ch == KEY_BTAB) {
    cycle_runtime_detail_mode(st);
    return true;
  }
  if ((ch == '\n' || ch == '\r' || ch == KEY_ENTER) &&
      runtime_has_worklist_items(st)) {
    if (runtime_is_menu_focus(st.runtime.focus)) {
      clear_runtime_drill_filters(st);
      focus_runtime_worklist(st);
      return true;
    }
    return open_runtime_action_menu(st);
  }
  if ((ch == 'a' || ch == 'A') && runtime_is_worklist_focus(st.runtime.focus)) {
    return open_runtime_action_menu(st);
  }
  if (ch == 27 && runtime_is_worklist_focus(st.runtime.focus)) {
    focus_runtime_menu(st);
    return true;
  }
  if (runtime_is_worklist_focus(st.runtime.focus) && ch == KEY_LEFT) {
    if (exit_runtime_nested_rows(st))
      return true;
    return true;
  }
  if (runtime_is_worklist_focus(st.runtime.focus) && ch == KEY_RIGHT) {
    if (runtime_is_session_lane(st.runtime.lane)) {
      if (st.runtime.job_filter_active)
        return true;
      if (st.runtime.campaign_filter_active) {
        return drill_runtime_into_jobs(st);
      }
      return drill_runtime_into_campaigns(st);
    }
    if (runtime_is_campaign_lane(st.runtime.lane)) {
      if (st.runtime.job_filter_active)
        return true;
      return drill_runtime_into_jobs(st);
    }
    return false;
  }
  if (ch == KEY_LEFT) {
    return select_prev_runtime_lane(st);
  }
  if (ch == KEY_RIGHT) {
    return select_next_runtime_lane(st);
  }
  if (ch == 'h' || ch == '[') {
    return select_prev_runtime_item(st);
  }
  if (ch == 'l' || ch == ']') {
    return select_next_runtime_item(st);
  }
  if (runtime_is_menu_focus(st.runtime.focus)) {
    if (ch == KEY_UP || ch == 'k')
      return select_prev_runtime_lane(st);
    if (ch == KEY_DOWN || ch == 'j')
      return select_next_runtime_lane(st);
    return false;
  }
  if (ch == KEY_UP || ch == 'k')
    return select_prev_runtime_item(st);
  if (ch == KEY_DOWN || ch == 'j')
    return select_next_runtime_item(st);
  return false;
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
