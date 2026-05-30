#pragma once

#include <ncursesw/ncurses.h>

#include "iinuji/iinuji_cmd/views/runtime/commands.h"
#include "iinuji/iinuji_cmd/views/common/action_menu.h"
#include "iinuji/iinuji_cmd/views/ui/dialogs.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class RuntimePopupActionKind : std::uint8_t {
  PauseSession = 0,
  ResumeSession = 1,
  MessageSession = 2,
  TerminateSession = 3,
  ViewMarshalEvents = 4,
  ViewMarshalCodexStdout = 5,
  ViewMarshalCodexStderr = 6,
  ViewMarshalSummary = 7,
  StopCampaign = 8,
  ForceStopCampaign = 9,
  ViewJobStdout = 10,
  ViewJobStderr = 11,
  StopJob = 12,
  ForceStopJob = 13,
  ViewMarshalBriefing = 14,
  ViewMarshalMemory = 15,
  ViewMarshalInputCheckpoint = 16,
  ViewMarshalIntentCheckpoint = 17,
  ViewMarshalMutationCheckpoint = 18,
  ViewMarshalObjective = 19,
  ViewMarshalGuidance = 20,
  ViewMarshalHumanRequest = 21,
  ViewCampaignStdout = 22,
  ViewCampaignStderr = 23,
  ViewCampaignManifest = 24,
  ViewCampaignDsl = 25,
  ViewJobTrace = 26,
  ViewJobManifest = 27,
  ViewJobCampaignDsl = 28,
  ViewJobContractDsl = 29,
  ViewJobWaveDsl = 30,
};

struct RuntimePopupAction {
  RuntimePopupActionKind kind;
  std::string label;
  bool enabled{true};
  std::string disabled_status{};
};

inline bool runtime_popup_action_file_available(const std::string &path) {
  return runtime_probe_log_viewer_file(path).ok;
}

inline std::string runtime_campaign_manifest_viewer_path(const CmdState &st) {
  if (!st.runtime.campaign.has_value())
    return {};
  return cuwacunu::hero::runtime::runtime_campaign_manifest_path(
             st.runtime.app.defaults.campaigns_root,
             st.runtime.campaign->campaign_cursor)
      .string();
}

inline std::string runtime_job_manifest_viewer_path(const CmdState &st) {
  if (!st.runtime.job.has_value())
    return {};
  return cuwacunu::hero::runtime::runtime_job_manifest_path(
             st.runtime.app.defaults.campaigns_root, st.runtime.job->job_cursor)
      .string();
}

inline RuntimeRowKind runtime_popup_row_kind(const CmdState &st) {
  if (runtime_is_session_lane(st.runtime.lane) &&
      st.runtime.job_filter_active &&
      (!st.runtime.selected_job_cursor.empty() || st.runtime.job.has_value())) {
    return RuntimeRowKind::Job;
  }
  if (runtime_is_session_lane(st.runtime.lane) &&
      st.runtime.campaign_filter_active &&
      (!st.runtime.selected_campaign_cursor.empty() ||
       st.runtime.campaign.has_value())) {
    return RuntimeRowKind::Campaign;
  }
  if (runtime_is_campaign_lane(st.runtime.lane) &&
      st.runtime.job_filter_active &&
      (!st.runtime.selected_job_cursor.empty() || st.runtime.job.has_value())) {
    return RuntimeRowKind::Job;
  }
  return st.runtime.lane;
}

inline bool drill_runtime_into_campaigns(CmdState &st) {
  const auto visible = runtime_child_campaign_indices(st);
  if (visible.empty() && !runtime_child_has_none_campaign_row(st)) {
    set_runtime_status(st, "Selected marshal has no campaigns.", false);
    return true;
  }
  st.runtime.campaign_filter_active = true;
  st.runtime.job_filter_active = false;
  if (!visible.empty()) {
    const std::string campaign_cursor =
        st.runtime.campaigns[visible.front()].campaign_cursor;
    select_runtime_campaign_and_related(st, campaign_cursor);
  } else {
    select_runtime_none_campaign_and_related(st,
                                             st.runtime.selected_session_id);
  }
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
                                         std::string busy_title,
                                         std::string busy_prompt,
                                         std::string success_status) {
  cmd_json_value_t structured{};
  std::string error{};
  bool ok = false;
  ui_run_blocking_dialog(
      std::move(busy_title), std::move(busy_prompt),
      [&]() {
        ok = runtime_invoke_marshal_tool(st, tool_name, arguments_json,
                                         &structured, &error);
      });
  if (!ok) {
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
                                         std::string busy_title,
                                         std::string busy_prompt,
                                         std::string success_status) {
  cmd_json_value_t structured{};
  std::string error{};
  bool ok = false;
  ui_run_blocking_dialog(
      std::move(busy_title), std::move(busy_prompt),
      [&]() {
        ok = runtime_invoke_runtime_tool(st, tool_name, arguments_json,
                                         &structured, &error);
      });
  if (!ok) {
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
  auto push_action = [&](RuntimePopupActionKind kind, std::string label,
                         bool enabled, std::string disabled_status) {
    actions.push_back(RuntimePopupAction{
        .kind = kind,
        .label = std::move(label),
        .enabled = enabled,
        .disabled_status = std::move(disabled_status),
    });
  };
  const RuntimeRowKind row_kind = runtime_popup_row_kind(st);
  if (row_kind == RuntimeRowKind::Session && st.runtime.session.has_value()) {
    const auto &session = *st.runtime.session;
    push_action(RuntimePopupActionKind::ViewMarshalEvents,
                "View marshal events",
                runtime_popup_action_file_available(session.events_path),
                "Marshal events are not available for this session yet.");
    push_action(RuntimePopupActionKind::ViewMarshalCodexStdout,
                "View codex stdout",
                runtime_popup_action_file_available(session.codex_stdout_path),
                "Codex stdout is not available for this session yet.");
    push_action(RuntimePopupActionKind::ViewMarshalCodexStderr,
                "View codex stderr",
                runtime_popup_action_file_available(session.codex_stderr_path),
                "Codex stderr is not available for this session yet.");
    const std::string summary_path =
        runtime_session_summary_path(st, session);
    const bool summary_state =
        cuwacunu::hero::marshal::is_marshal_session_summary_state(session);
    push_action(RuntimePopupActionKind::ViewMarshalSummary,
                "View report of affairs",
                summary_state &&
                    runtime_popup_action_file_available(summary_path),
                summary_state
                    ? "Report of affairs is not available for this session."
                    : "Report of affairs is available when the marshal is "
                      "review-ready or terminal.");
    push_action(RuntimePopupActionKind::ViewMarshalBriefing,
                "View marshal briefing",
                runtime_popup_action_file_available(session.briefing_path),
                "Marshal briefing is not available for this session.");
    push_action(RuntimePopupActionKind::ViewMarshalMemory,
                "View marshal memory",
                runtime_popup_action_file_available(session.memory_path),
                "Marshal memory is not available for this session.");
    push_action(
        RuntimePopupActionKind::ViewMarshalInputCheckpoint,
        "View latest input checkpoint",
        runtime_popup_action_file_available(session.last_input_checkpoint_path),
        "No input checkpoint has been recorded for this session.");
    push_action(RuntimePopupActionKind::ViewMarshalIntentCheckpoint,
                "View latest intent checkpoint",
                runtime_popup_action_file_available(
                    session.last_intent_checkpoint_path),
                "No intent checkpoint has been recorded for this session.");
    push_action(RuntimePopupActionKind::ViewMarshalMutationCheckpoint,
                "View latest mutation checkpoint",
                runtime_popup_action_file_available(
                    session.last_mutation_checkpoint_path),
                "No mutation checkpoint has been recorded for this session.");
    push_action(
        RuntimePopupActionKind::ViewMarshalObjective,
        "View marshal objective.md",
        runtime_popup_action_file_available(session.marshal_objective_md_path),
        "Marshal objective markdown is not available for this session.");
    push_action(
        RuntimePopupActionKind::ViewMarshalGuidance, "View marshal guidance.md",
        runtime_popup_action_file_available(session.marshal_guidance_md_path),
        "Marshal guidance markdown is not available for this session.");
    push_action(RuntimePopupActionKind::ViewMarshalHumanRequest,
                "View human request",
                runtime_popup_action_file_available(session.human_request_path),
                "There is no human request artifact for this session.");
    push_action(RuntimePopupActionKind::PauseSession, "Pause marshal",
                operator_session_is_working(session),
                "Pause is only available while the marshal is active.");
    push_action(RuntimePopupActionKind::ResumeSession, "Resume marshal",
                operator_session_is_operator_paused(session),
                "Resume is only available for operator-paused marshals.");
    push_action(RuntimePopupActionKind::MessageSession, "Message marshal",
                session.lifecycle == "live",
                "Messaging is only available while the marshal session is live.");
    push_action(RuntimePopupActionKind::TerminateSession, "Terminate marshal",
                operator_session_is_working(session) ||
                    operator_session_is_operator_paused(session),
                "Terminate is only available while the marshal is active or "
                "operator-paused.");
    return actions;
  }
  if (row_kind == RuntimeRowKind::Campaign && st.runtime.campaign.has_value()) {
    const auto &campaign = *st.runtime.campaign;
    push_action(RuntimePopupActionKind::ViewCampaignStdout,
                "View campaign stdout.log",
                runtime_popup_action_file_available(campaign.stdout_path),
                "Campaign stdout is not available yet.");
    push_action(RuntimePopupActionKind::ViewCampaignStderr,
                "View campaign stderr.log",
                runtime_popup_action_file_available(campaign.stderr_path),
                "Campaign stderr is not available yet.");
    push_action(RuntimePopupActionKind::ViewCampaignManifest,
                "View campaign manifest",
                runtime_popup_action_file_available(
                    runtime_campaign_manifest_viewer_path(st)),
                "Campaign manifest is not available for this campaign.");
    push_action(RuntimePopupActionKind::ViewCampaignDsl, "View campaign.dsl",
                runtime_popup_action_file_available(campaign.campaign_dsl_path),
                "Campaign DSL is not available for this campaign.");
    push_action(RuntimePopupActionKind::StopCampaign, "Stop campaign",
                runtime_campaign_is_active(campaign),
                "Stop is only available for active campaigns.");
    push_action(RuntimePopupActionKind::ForceStopCampaign,
                "Force stop campaign", runtime_campaign_is_active(campaign),
                "Force stop is only available for active campaigns.");
    return actions;
  }
  if (row_kind == RuntimeRowKind::Job && st.runtime.job.has_value()) {
    const auto &job = *st.runtime.job;
    push_action(RuntimePopupActionKind::ViewJobStdout, "View job stdout.log",
                runtime_popup_action_file_available(job.stdout_path),
                "Job stdout is not available yet.");
    push_action(RuntimePopupActionKind::ViewJobStderr, "View job stderr.log",
                runtime_popup_action_file_available(job.stderr_path),
                "Job stderr is not available yet.");
    push_action(RuntimePopupActionKind::ViewJobTrace, "View job trace.jsonl",
                runtime_popup_action_file_available(job.trace_path),
                "Job trace is not available yet.");
    push_action(RuntimePopupActionKind::ViewJobManifest, "View job manifest",
                runtime_popup_action_file_available(
                    runtime_job_manifest_viewer_path(st)),
                "Job manifest is not available for this job.");
    push_action(RuntimePopupActionKind::ViewJobCampaignDsl,
                "View bound campaign.dsl",
                runtime_popup_action_file_available(job.campaign_dsl_path),
                "Bound campaign DSL is not available for this job.");
    push_action(RuntimePopupActionKind::ViewJobContractDsl,
                "View bound contract.dsl",
                runtime_popup_action_file_available(job.contract_dsl_path),
                "Bound contract DSL is not available for this job.");
    push_action(RuntimePopupActionKind::ViewJobWaveDsl, "View bound wave.dsl",
                runtime_popup_action_file_available(job.wave_dsl_path),
                "Bound wave DSL is not available for this job.");
    push_action(RuntimePopupActionKind::StopJob, "Stop job",
                runtime_job_is_active(job),
                "Stop is only available for active jobs.");
    push_action(RuntimePopupActionKind::ForceStopJob, "Force stop job",
                runtime_job_is_active(job),
                "Force stop is only available for active jobs.");
  }
  return actions;
}

inline bool dispatch_runtime_popup_action(CmdState &st,
                                          const RuntimePopupAction &action) {
  auto open_session_artifact = [&](const std::string &path, std::string label,
                                   std::string missing_status) -> bool {
    if (!st.runtime.session.has_value())
      return false;
    return runtime_open_artifact_viewer(
        st, path,
        std::move(label) + " | " +
            trim_to_width(st.runtime.session->marshal_session_id, 28),
        std::move(missing_status));
  };
  auto open_campaign_artifact = [&](const std::string &path, std::string label,
                                    std::string missing_status) -> bool {
    if (!st.runtime.campaign.has_value())
      return false;
    return runtime_open_artifact_viewer(
        st, path,
        std::move(label) + " | " +
            trim_to_width(st.runtime.campaign->campaign_cursor, 28),
        std::move(missing_status));
  };
  auto open_job_artifact = [&](const std::string &path, std::string label,
                               std::string missing_status) -> bool {
    if (!st.runtime.job.has_value())
      return false;
    return runtime_open_artifact_viewer(
        st, path,
        std::move(label) + " | " +
            trim_to_width(st.runtime.job->job_cursor, 28),
        std::move(missing_status));
  };

  switch (action.kind) {
  case RuntimePopupActionKind::PauseSession: {
    if (!st.runtime.session.has_value())
      return false;
    return runtime_apply_marshal_action(
        st, "hero.marshal.pause_session",
        std::string("{\"marshal_session_id\":") +
            config_json_quote(st.runtime.session->marshal_session_id) + "}",
        " Pausing marshal ",
        "Sending pause request to Marshal Hero.",
        "Paused selected marshal.");
  }
  case RuntimePopupActionKind::ResumeSession: {
    if (!st.runtime.session.has_value())
      return false;
    return runtime_apply_marshal_action(
        st, "hero.marshal.resume_session",
        std::string("{\"marshal_session_id\":") +
            config_json_quote(st.runtime.session->marshal_session_id) + "}",
        " Resuming marshal ",
        "Sending resume request to Marshal Hero.",
        "Resumed selected marshal.");
  }
  case RuntimePopupActionKind::MessageSession: {
    if (!st.runtime.session.has_value())
      return false;
    std::string message{};
    bool cancelled = false;
    if (!ui_prompt_text_dialog(
            " Message marshal ",
            "Provide the operator message Marshal Hero should receive. Plain "
            "messages can ask questions, update future strategy, or request "
            "that the active campaign be interrupted and replanned.",
            &message, false, false, &cancelled)) {
      set_runtime_status(st, "failed to collect operator message", true);
      return true;
    }
    if (cancelled) {
      set_runtime_status(st, "Cancelled.", false);
      return true;
    }
    cmd_json_value_t structured{};
    std::string error{};
    bool ok = false;
    ui_run_blocking_dialog(
        " Sending message ",
        "Dispatching operator message to Marshal Hero.",
        [&]() {
          ok = runtime_invoke_marshal_tool(
              st, "hero.marshal.message_session",
              std::string("{\"marshal_session_id\":") +
                  config_json_quote(st.runtime.session->marshal_session_id) +
                  ",\"message\":" + config_json_quote(message) + "}",
              &structured, &error);
        });
    if (!ok) {
      set_runtime_status(
          st, error.empty() ? "marshal action failed" : error, true);
      return true;
    }
    (void)queue_runtime_refresh(st);
    const std::string delivery =
        cmd_json_string(cmd_json_field(&structured, "delivery"));
    const std::string reply_text =
        cmd_json_string(cmd_json_field(&structured, "reply_text"));
    const std::string warning =
        cmd_json_string(cmd_json_field(&structured, "warning"));
    const auto compact = [](std::string text) {
      text = trim_copy(text);
      for (char &ch : text) {
        if (ch == '\n' || ch == '\r' || ch == '\t')
          ch = ' ';
      }
      constexpr std::size_t kMax = 180;
      if (text.size() > kMax) {
        text = text.substr(0, kMax - 3);
        text += "...";
      }
      return text;
    };
    set_runtime_status(
        st,
        delivery == "failed"
            ? compact(warning.empty() ? "Marshal message delivery failed."
                                      : warning)
        : (delivery == "delivered" && !trim_copy(reply_text).empty())
            ? "Marshal: " + compact(reply_text)
        : delivery == "queued"
            ? "Marshal is processing the message."
            : "Delivered message to selected marshal.",
        delivery == "failed");
    return true;
  }
  case RuntimePopupActionKind::TerminateSession: {
    if (!st.runtime.session.has_value())
      return false;
    bool confirmed = false;
    bool cancelled = false;
    if (!ui_prompt_yes_no_dialog(
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
        " Terminating marshal ",
        "Sending terminate request to Marshal Hero.",
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
  case RuntimePopupActionKind::ViewMarshalSummary: {
    if (!st.runtime.session.has_value())
      return false;
    return open_session_artifact(
        runtime_session_summary_path(st, *st.runtime.session),
        "report of affairs",
        "Report of affairs is available when the marshal is review-ready or "
        "terminal.");
  }
  case RuntimePopupActionKind::ViewMarshalBriefing: {
    if (!st.runtime.session.has_value())
      return false;
    return open_session_artifact(st.runtime.session->briefing_path,
                                 "marshal briefing",
                                 "Marshal briefing is not available.");
  }
  case RuntimePopupActionKind::ViewMarshalMemory: {
    if (!st.runtime.session.has_value())
      return false;
    return open_session_artifact(st.runtime.session->memory_path,
                                 "marshal memory",
                                 "Marshal memory is not available.");
  }
  case RuntimePopupActionKind::ViewMarshalInputCheckpoint: {
    if (!st.runtime.session.has_value())
      return false;
    return open_session_artifact(
        st.runtime.session->last_input_checkpoint_path, "input checkpoint",
        "The latest input checkpoint is not available.");
  }
  case RuntimePopupActionKind::ViewMarshalIntentCheckpoint: {
    if (!st.runtime.session.has_value())
      return false;
    return open_session_artifact(
        st.runtime.session->last_intent_checkpoint_path, "intent checkpoint",
        "The latest intent checkpoint is not available.");
  }
  case RuntimePopupActionKind::ViewMarshalMutationCheckpoint: {
    if (!st.runtime.session.has_value())
      return false;
    return open_session_artifact(
        st.runtime.session->last_mutation_checkpoint_path,
        "mutation checkpoint",
        "The latest mutation checkpoint is not available.");
  }
  case RuntimePopupActionKind::ViewMarshalObjective: {
    if (!st.runtime.session.has_value())
      return false;
    return open_session_artifact(
        st.runtime.session->marshal_objective_md_path, "marshal objective.md",
        "Marshal objective markdown is not available.");
  }
  case RuntimePopupActionKind::ViewMarshalGuidance: {
    if (!st.runtime.session.has_value())
      return false;
    return open_session_artifact(st.runtime.session->marshal_guidance_md_path,
                                 "marshal guidance.md",
                                 "Marshal guidance markdown is not available.");
  }
  case RuntimePopupActionKind::ViewMarshalHumanRequest: {
    if (!st.runtime.session.has_value())
      return false;
    return open_session_artifact(st.runtime.session->human_request_path,
                                 "human request",
                                 "Human request artifact is not available.");
  }
  case RuntimePopupActionKind::StopCampaign: {
    if (!st.runtime.campaign.has_value())
      return false;
    return runtime_apply_runtime_action(
        st, "hero.runtime.stop_campaign",
        std::string("{\"campaign_cursor\":") +
            config_json_quote(st.runtime.campaign->campaign_cursor) + "}",
        " Stopping campaign ",
        "Sending stop signal to Runtime Hero.",
        "Stopped selected campaign.");
  }
  case RuntimePopupActionKind::ForceStopCampaign: {
    if (!st.runtime.campaign.has_value())
      return false;
    bool confirmed = false;
    bool cancelled = false;
    if (!ui_prompt_yes_no_dialog(
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
        " Force-stopping campaign ",
        "Sending force-stop signal to Runtime Hero.",
        "Force-stopped selected campaign.");
  }
  case RuntimePopupActionKind::ViewCampaignStdout: {
    return runtime_open_campaign_log_viewer(
        st, RuntimeLogViewerKind::CampaignStdout);
  }
  case RuntimePopupActionKind::ViewCampaignStderr: {
    return runtime_open_campaign_log_viewer(
        st, RuntimeLogViewerKind::CampaignStderr);
  }
  case RuntimePopupActionKind::ViewCampaignManifest: {
    return open_campaign_artifact(runtime_campaign_manifest_viewer_path(st),
                                  "campaign manifest",
                                  "Campaign manifest is not available.");
  }
  case RuntimePopupActionKind::ViewCampaignDsl: {
    if (!st.runtime.campaign.has_value())
      return false;
    return open_campaign_artifact(st.runtime.campaign->campaign_dsl_path,
                                  "campaign.dsl",
                                  "Campaign DSL is not available.");
  }
  case RuntimePopupActionKind::ViewJobStdout: {
    return runtime_open_job_log_viewer(st, RuntimeLogViewerKind::JobStdout);
  }
  case RuntimePopupActionKind::ViewJobStderr: {
    return runtime_open_job_log_viewer(st, RuntimeLogViewerKind::JobStderr);
  }
  case RuntimePopupActionKind::ViewJobTrace: {
    return runtime_open_job_log_viewer(st, RuntimeLogViewerKind::JobTrace);
  }
  case RuntimePopupActionKind::ViewJobManifest: {
    return open_job_artifact(runtime_job_manifest_viewer_path(st),
                             "job manifest", "Job manifest is not available.");
  }
  case RuntimePopupActionKind::ViewJobCampaignDsl: {
    if (!st.runtime.job.has_value())
      return false;
    return open_job_artifact(st.runtime.job->campaign_dsl_path,
                             "bound campaign.dsl",
                             "Bound campaign DSL is not available.");
  }
  case RuntimePopupActionKind::ViewJobContractDsl: {
    if (!st.runtime.job.has_value())
      return false;
    return open_job_artifact(st.runtime.job->contract_dsl_path,
                             "bound contract.dsl",
                             "Bound contract DSL is not available.");
  }
  case RuntimePopupActionKind::ViewJobWaveDsl: {
    if (!st.runtime.job.has_value())
      return false;
    return open_job_artifact(st.runtime.job->wave_dsl_path, "bound wave.dsl",
                             "Bound wave DSL is not available.");
  }
  case RuntimePopupActionKind::StopJob: {
    if (!st.runtime.job.has_value())
      return false;
    return runtime_apply_runtime_action(
        st, "hero.runtime.stop_job",
        std::string("{\"job_cursor\":") +
            config_json_quote(st.runtime.job->job_cursor) + "}",
        " Stopping job ",
        "Sending stop signal to Runtime Hero.",
        "Stopped selected job.");
  }
  case RuntimePopupActionKind::ForceStopJob: {
    if (!st.runtime.job.has_value())
      return false;
    bool confirmed = false;
    bool cancelled = false;
    if (!ui_prompt_yes_no_dialog(" Force stop job ",
                                 "Force-stop the selected job?", false,
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
        " Force-stopping job ",
        "Sending force-stop signal to Runtime Hero.",
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

  std::size_t selected = 0u;
  bool cancelled = false;
  if (!prompt_action_choice_panel("Runtime Actions", "Select action.", actions,
                                  &selected, &cancelled)) {
    set_runtime_status(st, "failed to open action menu", true);
    return true;
  }
  if (cancelled) {
    set_runtime_status(st, {}, false);
    return true;
  }
  const auto &action = selected_action_or_last(actions, selected);
  if (!action.enabled) {
    set_runtime_status(st,
                       action.disabled_status.empty()
                           ? "Selected action is currently unavailable."
                           : action.disabled_status,
                       false);
    return true;
  }
  return dispatch_runtime_popup_action(st, action);
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

  if (runtime_event_viewer_is_open(st)) {
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
    if (ch == 27) {
      clear_runtime_log_viewer(st);
      set_runtime_status(st, "Closed runtime log viewer.", false);
      return true;
    }
    if (ch == KEY_UP || ch == 'k') {
      (void)runtime_select_prev_event_viewer_entry(st);
      return true;
    }
    if (ch == KEY_DOWN || ch == 'j') {
      (void)runtime_select_next_event_viewer_entry(st);
      return true;
    }
    if (ch == KEY_PPAGE) {
      (void)runtime_page_event_viewer_entries(st, -5);
      return true;
    }
    if (ch == KEY_NPAGE) {
      (void)runtime_page_event_viewer_entries(st, +5);
      return true;
    }
    if (ch == KEY_HOME) {
      (void)runtime_select_first_event_viewer_entry(st);
      return true;
    }
    if (ch == KEY_END) {
      (void)runtime_select_last_event_viewer_entry(st, true);
      return true;
    }
    return true;
  }

  if (runtime_text_log_viewer_is_open(st) && st.runtime.log_viewer) {
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
