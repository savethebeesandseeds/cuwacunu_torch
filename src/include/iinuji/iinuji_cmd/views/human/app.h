#pragma once

#include <ncursesw/ncurses.h>
#include <string>
#include <utility>
#include <vector>

#include "iinuji/iinuji_cmd/views/human/commands.h"
#include "iinuji/iinuji_cmd/views/lattice/commands.h"
#include "iinuji/iinuji_cmd/views/runtime/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline void refresh_hero_shell_views(CmdState &st) {
  // Human actions should return control to the operator quickly. Refresh the
  // Human cockpit immediately, and let the other screens refresh when they are
  // focused again instead of forcing large runtime/lattice scans inline.
  (void)queue_human_refresh(st);
}

inline bool handle_human_key(CmdState &st, int ch);

inline bool continue_human_session(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string instruction{};
  bool cancelled = false;
  if (!cuwacunu::hero::human_mcp::prompt_text_dialog(
          " Continue session ",
          "Provide the operator instruction that should launch more work for "
          "this idle session. This is the action that continues the session.",
          &instruction, false, false, &cancelled)) {
    set_human_status(st, "failed to collect continuation instruction", true);
    return true;
  }
  if (cancelled) {
    set_human_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  if (!cuwacunu::hero::human_mcp::continue_marshal_session(
          &st.human.app, session, instruction, &structured, &error)) {
    set_human_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_human_status(st, "Continued idle session.", false);
  return true;
}

inline bool answer_human_request(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string answer{};
  bool cancelled = false;
  if (!cuwacunu::hero::human_mcp::prompt_text_dialog(
          " Answer request ",
          "Provide the clarification answer that Marshal Hero should continue "
          "with.",
          &answer, false, false, &cancelled)) {
    set_human_status(st, "failed to collect request answer", true);
    return true;
  }
  if (cancelled) {
    set_human_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  if (!cuwacunu::hero::human_mcp::build_request_answer_and_resume(
          &st.human.app, session, answer, &structured, &error)) {
    set_human_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_human_status(st, "Applied clarification answer.", false);
  return true;
}

inline bool apply_human_request_resolution(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    bool stop_direct) {
  cuwacunu::hero::human_mcp::interactive_resolution_input_t input{};
  bool cancelled = false;
  std::string error{};
  if (!cuwacunu::hero::human_mcp::collect_interactive_resolution_inputs(
          &st.human.app, session, stop_direct, &input, &cancelled, &error)) {
    set_human_status(st, error, true);
    return true;
  }
  if (cancelled) {
    set_human_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  if (!cuwacunu::hero::human_mcp::build_resolution_and_apply(
          &st.human.app, session, input.resolution_kind, input.reason,
          &structured, &error)) {
    set_human_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_human_status(st, "Applied signed governance resolution.", false);
  return true;
}

inline bool acknowledge_human_review(
    CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string note{};
  bool cancelled = false;
  if (!cuwacunu::hero::human_mcp::prompt_text_dialog(
          " Acknowledge review ",
          "This does not continue the session. It records a signed "
          "acknowledgment and clears the review report from the queue after "
          "review. Provide a required acknowledgment message:",
          &note, false, false, &cancelled)) {
    set_human_status(st, "failed to collect review acknowledgment note", true);
    return true;
  }
  if (cancelled) {
    set_human_status(st, "Cancelled.", false);
    return true;
  }
  std::string structured{};
  std::string error{};
  if (!cuwacunu::hero::human_mcp::acknowledge_session_summary(
          &st.human.app, session, note, &structured, &error)) {
    set_human_status(st, error, true);
    return true;
  }
  refresh_hero_shell_views(st);
  set_human_status(st, "Acknowledged review report.", false);
  return true;
}

inline bool
dispatch_human_inbox_popup_action(CmdState &st,
                                  const human_inbox_popup_action_t &action) {
  switch (action.kind) {
  case human_inbox_popup_action_kind_t::PauseSession: {
    const auto selected = selected_visible_human_session_record(st);
    if (!selected.has_value())
      return false;
    std::string structured{};
    std::string error{};
    if (!cuwacunu::hero::human_mcp::pause_marshal_session(
            &st.human.app, *selected, false, &structured, &error)) {
      set_human_status(st, error, true);
      return true;
    }
    refresh_hero_shell_views(st);
    set_human_status(st, "Paused selected session.", false);
    return true;
  }
  case human_inbox_popup_action_kind_t::ResumeOperatorPause: {
    const auto selected = selected_visible_human_session_record(st);
    if (!selected.has_value())
      return false;
    std::string structured{};
    std::string error{};
    if (!cuwacunu::hero::human_mcp::resume_marshal_session(
            &st.human.app, *selected, &structured, &error)) {
      set_human_status(st, error, true);
      return true;
    }
    refresh_hero_shell_views(st);
    set_human_status(st, "Resumed operator-paused session.", false);
    return true;
  }
  case human_inbox_popup_action_kind_t::ContinueIdleSession: {
    const auto selected = selected_visible_human_session_record(st);
    if (!selected.has_value())
      return false;
    return continue_human_session(st, *selected);
  }
  case human_inbox_popup_action_kind_t::TerminateSession: {
    const auto selected = selected_visible_human_session_record(st);
    if (!selected.has_value())
      return false;
    bool confirmed = false;
    bool cancelled = false;
    if (!cuwacunu::hero::human_mcp::prompt_yes_no_dialog(
            " Terminate session ",
            "Terminate the selected session? This action is final.", false,
            &confirmed, &cancelled)) {
      set_human_status(st, "failed to collect session termination confirmation",
                       true);
      return true;
    }
    if (cancelled || !confirmed) {
      set_human_status(st, "Cancelled.", false);
      return true;
    }
    std::string structured{};
    std::string error{};
    if (!cuwacunu::hero::human_mcp::terminate_marshal_session(
            &st.human.app, *selected, false, &structured, &error)) {
      set_human_status(st, error, true);
      return true;
    }
    refresh_hero_shell_views(st);
    set_human_status(st, "Terminated selected session.", false);
    return true;
  }
  case human_inbox_popup_action_kind_t::AnswerRequest: {
    const auto selected = selected_human_request_record(st);
    if (!selected.has_value())
      return false;
    return answer_human_request(st, *selected);
  }
  case human_inbox_popup_action_kind_t::ResolveRequest: {
    const auto selected = selected_human_request_record(st);
    if (!selected.has_value())
      return false;
    return apply_human_request_resolution(st, *selected, false);
  }
  case human_inbox_popup_action_kind_t::TerminateGovernanceRequest: {
    const auto selected = selected_human_request_record(st);
    if (!selected.has_value())
      return false;
    return apply_human_request_resolution(st, *selected, true);
  }
  case human_inbox_popup_action_kind_t::AcknowledgeReview: {
    const auto selected = selected_human_review_record(st);
    if (!selected.has_value())
      return false;
    return acknowledge_human_review(st, *selected);
  }
  }
  return false;
}

inline bool open_human_inbox_action_menu(CmdState &st) {
  const auto actions = human_inbox_popup_actions(st);
  if (actions.empty()) {
    set_human_status(st, "No actions available for the selected row.", false);
    return true;
  }

  std::vector<std::string> options{};
  options.reserve(actions.size());
  bool has_review_action = false;
  for (const auto &action : actions) {
    options.push_back(action.label);
    if (action.kind == human_inbox_popup_action_kind_t::ContinueIdleSession ||
        action.kind == human_inbox_popup_action_kind_t::AcknowledgeReview) {
      has_review_action = true;
    }
  }

  std::size_t selected = 0;
  bool cancelled = false;
  const bool review_context = has_review_action;
  const std::string prompt_body =
      review_context ? "Select review action." : "Select action.";
  if (!cuwacunu::hero::human_mcp::prompt_choice_dialog(
          " Marshal actions ", prompt_body, options, &selected, &cancelled)) {
    set_human_status(st, "failed to open action menu", true);
    return true;
  }
  if (cancelled) {
    set_human_status(st, {}, false);
    return true;
  }
  return dispatch_human_inbox_popup_action(
      st, actions[std::min(selected, actions.size() - 1)]);
}

inline bool handle_human_key(CmdState &st, int ch) {
  if (st.screen != ScreenMode::Human)
    return false;
  if (ch == 'f' || ch == 'F') {
    cycle_human_phase_filter(st);
    set_human_status(st,
                     "Marshal screen is inbox-only now. Runtime owns the "
                     "broader Marshal session inventory.",
                     false);
    return true;
  }
  if (ch == 27 && human_is_inbox_focus(st.human.focus)) {
    focus_human_menu(st);
    return true;
  }
  if (human_is_menu_focus(st.human.focus)) {
    if (ch == KEY_UP || ch == KEY_LEFT || ch == 'k') {
      return select_prev_human_menu_item(st);
    }
    if (ch == KEY_DOWN || ch == KEY_RIGHT || ch == 'j') {
      return select_next_human_menu_item(st);
    }
    if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
      focus_human_inbox(st);
      return true;
    }
    return false;
  }

  if (ch == KEY_UP || ch == 'k')
    return select_prev_human_row(st);
  if (ch == KEY_DOWN || ch == 'j')
    return select_next_human_row(st);
  if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
    return open_human_inbox_action_menu(st);
  }
  return false;
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
