#pragma once

#include <ncursesw/ncurses.h>
#include <string>
#include <utility>
#include <vector>

#include "iinuji/iinuji_cmd/views/inbox/commands.h"
#include "iinuji/iinuji_cmd/views/lattice/commands.h"
#include "iinuji/iinuji_cmd/views/runtime/commands.h"
#include "iinuji/iinuji_cmd/views/ui/dialogs.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void refresh_hero_shell_views(CmdState &st) {
  // Inbox actions should return control to the operator quickly. Refresh the
  // Inbox screen immediately, and let the other screens refresh when they are
  // focused again instead of forcing large runtime/lattice scans inline.
  (void)queue_inbox_refresh(st);
}

inline bool handle_inbox_key(CmdState &st, int ch);

inline bool continue_inbox_session(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string instruction{};
  bool cancelled = false;
  if (!ui_prompt_text_dialog(
          " Continue session ",
          "Provide the operator instruction that should launch more work for "
          "this idle session. This is the action that continues the session.",
          &instruction, false, false, &cancelled)) {
    set_inbox_status(st, "failed to collect continuation instruction", true);
    return true;
  }
  if (cancelled) {
    set_inbox_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  if (!cuwacunu::hero::human_mcp::continue_marshal_session(
          &st.inbox.app, session, instruction, &structured, &error)) {
    set_inbox_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_inbox_status(st, "Continued idle session.", false);
  return true;
}

inline bool answer_inbox_request(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string answer{};
  bool cancelled = false;
  if (!ui_prompt_text_dialog(
          " Answer request ",
          "Provide the clarification answer that Marshal Hero should continue "
          "with.",
          &answer, false, false, &cancelled)) {
    set_inbox_status(st, "failed to collect request answer", true);
    return true;
  }
  if (cancelled) {
    set_inbox_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  if (!cuwacunu::hero::human_mcp::build_request_answer_and_resume(
          &st.inbox.app, session, answer, &structured, &error)) {
    set_inbox_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_inbox_status(st, "Applied clarification answer.", false);
  return true;
}

inline bool collect_inbox_resolution_input(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool stop_direct,
    cuwacunu::hero::human_mcp::interactive_resolution_input_t *out,
    bool *cancelled) {
  if (cancelled)
    *cancelled = false;
  if (out == nullptr)
    return false;
  *out = cuwacunu::hero::human_mcp::interactive_resolution_input_t{};

  if (stop_direct) {
    out->resolution_kind = "terminate";
  } else {
    std::vector<ui_choice_panel_option_t> options{
        ui_choice_panel_option_t{.label = "Grant requested governance",
                                 .enabled = true},
        ui_choice_panel_option_t{
            .label = "Clarify objective or policy without widening authority",
            .enabled = true},
        ui_choice_panel_option_t{.label = "Deny requested governance",
                                 .enabled = true},
        ui_choice_panel_option_t{.label = "Terminate the session",
                                 .enabled = true},
    };
    std::size_t resolution_idx = 0u;
    if (!ui_prompt_choice_panel(
            " Resolution ",
            "Choose how to answer this pending governance request.", options,
            &resolution_idx, cancelled)) {
      return false;
    }
    if (cancelled && *cancelled)
      return true;
    if (resolution_idx == 0u) {
      out->resolution_kind = "grant";
    } else if (resolution_idx == 1u) {
      out->resolution_kind = "clarify";
    } else if (resolution_idx == 2u) {
      out->resolution_kind = "deny";
    } else {
      out->resolution_kind = "terminate";
    }
  }

  const std::string prompt_title =
      (out->resolution_kind == "grant")     ? " Grant rationale "
      : (out->resolution_kind == "clarify") ? " Clarification "
      : (out->resolution_kind == "deny")    ? " Denial rationale "
                                            : " Termination rationale ";
  const std::string prompt_body =
      (out->resolution_kind == "grant")
          ? "Why grant this governance request? This reason is shown to "
            "Marshal Hero."
      : (out->resolution_kind == "clarify")
          ? "What clarification should Marshal Hero incorporate on the next "
            "planning checkpoint?"
      : (out->resolution_kind == "deny")
          ? "Why deny this governance request? This reason is shown to Marshal "
            "Hero."
          : "Why terminate this session? This reason is shown to Marshal Hero.";
  if (!ui_prompt_text_dialog(prompt_title, prompt_body, &out->reason, false,
                             false, cancelled)) {
    return false;
  }
  if (cancelled && *cancelled)
    return true;

  (void)session;
  return true;
}

inline bool apply_inbox_request_resolution(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool stop_direct) {
  cuwacunu::hero::human_mcp::interactive_resolution_input_t input{};
  bool cancelled = false;
  if (!collect_inbox_resolution_input(session, stop_direct, &input,
                                      &cancelled)) {
    set_inbox_status(st, "failed to collect governance resolution input", true);
    return true;
  }
  if (cancelled) {
    set_inbox_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  if (!cuwacunu::hero::human_mcp::build_resolution_and_apply(
          &st.inbox.app, session, input.resolution_kind, input.reason,
          &structured, &error)) {
    set_inbox_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_inbox_status(st, "Applied signed governance resolution.", false);
  return true;
}

inline bool acknowledge_inbox_review(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string note{};
  bool cancelled = false;
  if (!ui_prompt_text_dialog(
          " Acknowledge review ",
          "This does not continue the session. It records a signed "
          "acknowledgment and clears the review report from the queue after "
          "review. Provide a required acknowledgment message:",
          &note, false, false, &cancelled)) {
    set_inbox_status(st, "failed to collect review acknowledgment note", true);
    return true;
  }
  if (cancelled) {
    set_inbox_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  if (!cuwacunu::hero::human_mcp::acknowledge_session_summary(
          &st.inbox.app, session, note, &structured, &error)) {
    set_inbox_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_inbox_status(st, "Acknowledged review report.", false);
  return true;
}

inline bool
dispatch_inbox_popup_action(CmdState &st,
                                  const inbox_popup_action_t &action) {
  switch (action.kind) {
  case inbox_popup_action_kind_t::PauseSession: {
    const auto selected = selected_visible_session_record(st);
    if (!selected.has_value())
      return false;
    std::string structured{};
    std::string error{};
    if (!cuwacunu::hero::human_mcp::pause_marshal_session(
            &st.inbox.app, *selected, false, &structured, &error)) {
      set_inbox_status(st, error, true);
      return true;
    }
    refresh_hero_shell_views(st);
    set_inbox_status(st, "Paused selected session.", false);
    return true;
  }
  case inbox_popup_action_kind_t::ResumeOperatorPause: {
    const auto selected = selected_visible_session_record(st);
    if (!selected.has_value())
      return false;
    std::string structured{};
    std::string error{};
    if (!cuwacunu::hero::human_mcp::resume_marshal_session(
            &st.inbox.app, *selected, &structured, &error)) {
      set_inbox_status(st, error, true);
      return true;
    }
    refresh_hero_shell_views(st);
    set_inbox_status(st, "Resumed operator-paused session.", false);
    return true;
  }
  case inbox_popup_action_kind_t::ContinueIdleSession: {
    const auto selected = selected_visible_session_record(st);
    if (!selected.has_value())
      return false;
    return continue_inbox_session(st, *selected);
  }
  case inbox_popup_action_kind_t::TerminateSession: {
    const auto selected = selected_visible_session_record(st);
    if (!selected.has_value())
      return false;
    bool confirmed = false;
    bool cancelled = false;
    if (!ui_prompt_yes_no_dialog(
            " Terminate session ",
            "Terminate the selected session? This action is final.", false,
            &confirmed, &cancelled)) {
      set_inbox_status(st, "failed to collect session termination confirmation",
                       true);
      return true;
    }
    if (cancelled || !confirmed) {
      set_inbox_status(st, "Cancelled.", false);
      return true;
    }
    std::string structured{};
    std::string error{};
    if (!cuwacunu::hero::human_mcp::terminate_marshal_session(
            &st.inbox.app, *selected, false, &structured, &error)) {
      set_inbox_status(st, error, true);
      return true;
    }
    refresh_hero_shell_views(st);
    set_inbox_status(st, "Terminated selected session.", false);
    return true;
  }
  case inbox_popup_action_kind_t::AnswerRequest: {
    const auto selected = selected_inbox_request_record(st);
    if (!selected.has_value())
      return false;
    return answer_inbox_request(st, *selected);
  }
  case inbox_popup_action_kind_t::ResolveRequest: {
    const auto selected = selected_inbox_request_record(st);
    if (!selected.has_value())
      return false;
    return apply_inbox_request_resolution(st, *selected, false);
  }
  case inbox_popup_action_kind_t::TerminateGovernanceRequest: {
    const auto selected = selected_inbox_request_record(st);
    if (!selected.has_value())
      return false;
    return apply_inbox_request_resolution(st, *selected, true);
  }
  case inbox_popup_action_kind_t::AcknowledgeReview: {
    const auto selected = selected_inbox_review_record(st);
    if (!selected.has_value())
      return false;
    return acknowledge_inbox_review(st, *selected);
  }
  }
  return false;
}

inline bool open_inbox_action_menu(CmdState &st) {
  const auto actions = inbox_popup_actions(st);
  if (actions.empty()) {
    set_inbox_status(st, "No actions available for the selected row.", false);
    return true;
  }

  std::vector<ui_choice_panel_option_t> options{};
  options.reserve(actions.size());
  for (const auto &action : actions) {
    options.push_back(ui_choice_panel_option_t{
        .label = action.label,
        .enabled = action.enabled,
        .disabled_status = action.disabled_status,
    });
  }

  std::size_t selected = 0u;
  for (std::size_t i = 0; i < actions.size(); ++i) {
    if (actions[i].enabled) {
      selected = i;
      break;
    }
  }
  bool cancelled = false;
  if (!ui_prompt_choice_panel("Inbox Actions", "Select inbox action.",
                              options, &selected, &cancelled)) {
    set_inbox_status(st, "failed to open action menu", true);
    return true;
  }
  if (cancelled) {
    set_inbox_status(st, {}, false);
    return true;
  }
  const auto &action = actions[std::min(selected, actions.size() - 1)];
  if (!action.enabled) {
    set_inbox_status(st,
                     action.disabled_status.empty()
                         ? "Selected action is currently unavailable."
                         : action.disabled_status,
                     false);
    return true;
  }
  return dispatch_inbox_popup_action(st, action);
}

inline bool handle_inbox_key(CmdState &st, int ch) {
  if (st.screen != ScreenMode::Inbox)
    return false;
  if (ch == 'f' || ch == 'F') {
    cycle_inbox_phase_filter(st);
    set_inbox_status(
        st,
        "Inbox is the only F2 lane now. Runtime owns the broader Marshal "
        "session inventory.",
        false);
    return true;
  }
  if (ch == 27 && inbox_is_inbox_focus(st.inbox.focus)) {
    focus_inbox_menu(st);
    return true;
  }
  if (inbox_is_menu_focus(st.inbox.focus)) {
    if (ch == KEY_UP || ch == KEY_LEFT || ch == 'k') {
      return select_prev_inbox_menu_item(st);
    }
    if (ch == KEY_DOWN || ch == KEY_RIGHT || ch == 'j') {
      return select_next_inbox_menu_item(st);
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      focus_inbox_worklist(st);
      return true;
    }
    return false;
  }

  if (ch == KEY_UP || ch == 'k')
    return select_prev_inbox_row(st);
  if (ch == KEY_DOWN || ch == 'j')
    return select_next_inbox_row(st);
  if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
    return open_inbox_action_menu(st);
  }
  return false;
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
