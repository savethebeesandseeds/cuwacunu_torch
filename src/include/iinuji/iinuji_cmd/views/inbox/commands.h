#pragma once

#include <algorithm>
#include <chrono>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline constexpr std::uint64_t kInboxTransientStatusTtlMs = 3200;

inline std::uint64_t inbox_status_now_ms() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

inline bool
inbox_session_has_pending_request(const CmdState &st,
                                  std::string_view marshal_session_id) {
  return cuwacunu::hero::human_mcp::find_session_index_by_id(
             st.inbox.operator_inbox.actionable_requests, marshal_session_id)
      .has_value();
}

inline bool
inbox_session_has_pending_review(const CmdState &st,
                                 std::string_view marshal_session_id) {
  return cuwacunu::hero::human_mcp::find_session_index_by_id(
             st.inbox.operator_inbox.unacknowledged_summaries, marshal_session_id)
      .has_value();
}

inline bool inbox_session_is_operator_paused(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return session.phase == "paused" && session.pause_kind == "operator";
}

inline bool inbox_session_is_live_phase(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return session.phase == "active" || session.phase == "running_campaign";
}

inline bool inbox_session_is_inbox_member(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return inbox_session_has_pending_request(st, session.marshal_session_id) ||
         inbox_session_has_pending_review(st, session.marshal_session_id) ||
         inbox_session_is_operator_paused(session);
}

inline std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
inbox_session_rows(const CmdState &st) {
  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> rows{};
  rows.reserve(st.inbox.operator_inbox.all_sessions.size());
  for (const auto &session : st.inbox.operator_inbox.all_sessions) {
    if (inbox_session_is_inbox_member(st, session))
      rows.push_back(session);
  }
  return rows;
}

inline std::size_t count_tracked_inbox_sessions(const CmdState &st) {
  return st.inbox.operator_inbox.all_sessions.size();
}

inline std::size_t count_inbox_sessions(const CmdState &st) {
  return inbox_session_rows(st).size();
}

inline std::size_t count_operator_paused_inbox_sessions(const CmdState &st) {
  return static_cast<std::size_t>(
      std::count_if(st.inbox.operator_inbox.all_sessions.begin(),
                    st.inbox.operator_inbox.all_sessions.end(), [](const auto &session) {
                      return inbox_session_is_operator_paused(session);
                    }));
}

inline std::size_t count_pending_request_inbox_sessions(const CmdState &st) {
  return st.inbox.operator_inbox.actionable_requests.size();
}

inline std::size_t count_pending_review_inbox_sessions(const CmdState &st) {
  return st.inbox.operator_inbox.unacknowledged_summaries.size();
}

// Compatibility helper for older code paths that treated idle/finished rows as
// "review state" sessions.
inline std::size_t count_review_state_inbox_sessions(const CmdState &st) {
  return count_pending_review_inbox_sessions(st);
}

inline std::size_t inbox_menu_index(inbox_console_view_t view) {
  (void)view;
  return 0;
}

inline inbox_console_view_t inbox_view_from_menu_index(std::size_t index) {
  (void)index;
  return kInboxView;
}

inline void force_inbox_view(CmdState &st) {
  st.inbox.view = kInboxView;
}

inline void focus_inbox_menu(CmdState &st) {
  force_inbox_view(st);
  st.inbox.focus = kInboxMenuFocus;
}

inline void focus_inbox_worklist(CmdState &st) {
  force_inbox_view(st);
  st.inbox.focus = kInboxFocus;
}

inline std::string current_inbox_selected_session_id(const CmdState &st) {
  const auto rows = inbox_session_rows(st);
  if (rows.empty())
    return {};
  const std::size_t idx =
      std::min(st.inbox.selected_inbox_session, rows.size() - 1);
  return rows[idx].marshal_session_id;
}

inline std::string preferred_inbox_session_id(const CmdState &st) {
  const std::string current = current_inbox_selected_session_id(st);
  if (!current.empty())
    return current;
  const auto inbox_rows = inbox_session_rows(st);
  if (!inbox_rows.empty())
    return inbox_rows.front().marshal_session_id;
  if (!st.inbox.operator_inbox.all_sessions.empty()) {
    return st.inbox.operator_inbox.all_sessions.front().marshal_session_id;
  }
  return {};
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_visible_inbox_session_record(const CmdState &st) {
  const auto rows = inbox_session_rows(st);
  if (rows.empty())
    return std::nullopt;
  const std::size_t idx =
      std::min(st.inbox.selected_inbox_session, rows.size() - 1);
  return rows[idx];
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_visible_live_session_record(const CmdState &st) {
  return selected_visible_inbox_session_record(st);
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_visible_history_session_record(const CmdState &st) {
  return selected_visible_inbox_session_record(st);
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_visible_session_record(const CmdState &st) {
  return selected_visible_inbox_session_record(st);
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_inbox_anchor_session_record(const CmdState &st) {
  const std::string marshal_session_id = preferred_inbox_session_id(st);
  if (marshal_session_id.empty())
    return std::nullopt;
  for (const auto &session : st.inbox.operator_inbox.all_sessions) {
    if (session.marshal_session_id == marshal_session_id)
      return session;
  }
  for (const auto &session : st.inbox.operator_inbox.actionable_requests) {
    if (session.marshal_session_id == marshal_session_id)
      return session;
  }
  for (const auto &session : st.inbox.operator_inbox.unacknowledged_summaries) {
    if (session.marshal_session_id == marshal_session_id)
      return session;
  }
  return std::nullopt;
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_inbox_request_record(const CmdState &st) {
  const auto selected = selected_visible_session_record(st);
  if (!selected.has_value())
    return std::nullopt;
  const auto idx = cuwacunu::hero::human_mcp::find_session_index_by_id(
      st.inbox.operator_inbox.actionable_requests, selected->marshal_session_id);
  if (!idx.has_value())
    return std::nullopt;
  return st.inbox.operator_inbox.actionable_requests[*idx];
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_inbox_review_record(const CmdState &st) {
  const auto selected = selected_visible_session_record(st);
  if (!selected.has_value())
    return std::nullopt;
  const auto idx = cuwacunu::hero::human_mcp::find_session_index_by_id(
      st.inbox.operator_inbox.unacknowledged_summaries, selected->marshal_session_id);
  if (!idx.has_value())
    return std::nullopt;
  return st.inbox.operator_inbox.unacknowledged_summaries[*idx];
}

struct inbox_detail_request_t {
  inbox_detail_kind_t kind{inbox_detail_kind_t::None};
  cuwacunu::hero::marshal::marshal_session_record_t session{};
};

inline void clear_inbox_detail_cache(CmdState &st) {
  st.inbox.detail_kind = inbox_detail_kind_t::None;
  st.inbox.detail_session_id.clear();
  st.inbox.detail_text.clear();
  st.inbox.detail_error.clear();
  st.inbox.detail_loaded = false;
  st.inbox.detail_requeue = false;
}

inline std::optional<inbox_detail_request_t>
current_inbox_detail_request(const CmdState &st) {
  if (const auto request = selected_inbox_request_record(st);
      request.has_value()) {
    return inbox_detail_request_t{inbox_detail_kind_t::InboxRequest, *request};
  }
  if (const auto review = selected_inbox_review_record(st);
      review.has_value()) {
    return inbox_detail_request_t{inbox_detail_kind_t::InboxReview, *review};
  }
  return std::nullopt;
}

inline bool queue_inbox_detail_refresh(CmdState &st);
inline bool poll_inbox_async_updates(CmdState &st);

inline void
push_inbox_action(std::vector<inbox_popup_action_t> &actions,
                        std::string label, inbox_popup_action_kind_t kind,
                        bool enabled = true, std::string disabled_status = {}) {
  actions.push_back(inbox_popup_action_t{
      .label = std::move(label),
      .kind = kind,
      .enabled = enabled,
      .disabled_status = std::move(disabled_status),
  });
}

inline std::vector<inbox_popup_action_t>
inbox_popup_actions(const CmdState &st) {
  std::vector<inbox_popup_action_t> actions{};
  const auto selected = selected_visible_session_record(st);
  if (!selected.has_value())
    return actions;

  const bool has_pending_request =
      inbox_session_has_pending_request(st, selected->marshal_session_id);
  const bool clarification_request =
      has_pending_request &&
      cuwacunu::hero::human_mcp::is_clarification_pause_kind(
          selected->pause_kind);
  const bool governance_request = has_pending_request && !clarification_request;
  const bool operator_paused = inbox_session_is_operator_paused(*selected);
  const bool live_phase = inbox_session_is_live_phase(*selected);
  const bool pending_review =
      inbox_session_has_pending_review(st, selected->marshal_session_id);
  const bool idle_review = pending_review && selected->phase == "idle";

  push_inbox_action(
      actions, "Pause session", inbox_popup_action_kind_t::PauseSession,
      live_phase, "Pause is only available while the session is active.");
  push_inbox_action(
      actions, "Resume operator pause",
      inbox_popup_action_kind_t::ResumeOperatorPause, operator_paused,
      "Resume is only available for operator-paused sessions.");
  push_inbox_action(
      actions, "Continue session with instruction",
      inbox_popup_action_kind_t::ContinueIdleSession, idle_review,
      "Continue is only available for idle sessions waiting in review.");
  push_inbox_action(
      actions, "Terminate session",
      inbox_popup_action_kind_t::TerminateSession,
      live_phase || operator_paused,
      "Terminate session is only available for active or operator-paused "
      "sessions.");
  push_inbox_action(
      actions, "Answer request", inbox_popup_action_kind_t::AnswerRequest,
      clarification_request,
      "Answer request is only available for clarification requests.");
  push_inbox_action(
      actions, "Resolve request",
      inbox_popup_action_kind_t::ResolveRequest, governance_request,
      "Resolve request is only available for governance requests.");
  push_inbox_action(
      actions, "Terminate request",
      inbox_popup_action_kind_t::TerminateGovernanceRequest,
      governance_request,
      "Terminate request is only available for governance requests.");
  push_inbox_action(
      actions, "Acknowledge review",
      inbox_popup_action_kind_t::AcknowledgeReview, pending_review,
      "Acknowledge review is only available when a review report is pending.");
  return actions;
}

inline bool inbox_has_visible_status(const InboxState &inbox) {
  if (inbox.status.empty())
    return false;
  if (inbox.status_is_error)
    return true;
  if (inbox.status_expires_at_ms == 0)
    return true;
  return inbox_status_now_ms() < inbox.status_expires_at_ms;
}

inline std::string visible_inbox_status(const CmdState &st) {
  return inbox_has_visible_status(st.inbox) ? st.inbox.status : std::string{};
}

inline void set_inbox_status(CmdState &st, std::string status, bool is_error) {
  st.inbox.status = std::move(status);
  st.inbox.status_is_error = is_error;
  if (st.inbox.status.empty()) {
    st.inbox.status_expires_at_ms = 0;
  } else if (is_error) {
    st.inbox.status_expires_at_ms = 0;
  } else {
    st.inbox.status_expires_at_ms =
        inbox_status_now_ms() + kInboxTransientStatusTtlMs;
  }
  if (is_error) {
    st.inbox.ok = false;
    st.inbox.error = st.inbox.status;
  } else {
    st.inbox.error.clear();
  }
}

inline void clamp_inbox_state(CmdState &st);

inline InboxRefreshSnapshot
collect_inbox_refresh_snapshot(cuwacunu::hero::human_mcp::app_context_t app) {
  InboxRefreshSnapshot snapshot{};
  if (app.defaults.marshal_root.empty()) {
    snapshot.error = "Inbox is not configured.";
    return snapshot;
  }

  if (!cuwacunu::hero::human_mcp::collect_human_operator_inbox(
          app, &snapshot.operator_inbox, true, &snapshot.error)) {
    if (snapshot.error.empty()) {
      snapshot.error = "failed to refresh Inbox";
    }
    return snapshot;
  }

  cuwacunu::hero::human_mcp::sort_sessions_newest_first(
      &snapshot.operator_inbox.all_sessions);
  cuwacunu::hero::human_mcp::sort_sessions_newest_first(
      &snapshot.operator_inbox.actionable_requests);
  cuwacunu::hero::human_mcp::sort_sessions_newest_first(
      &snapshot.operator_inbox.unacknowledged_summaries);
  snapshot.ok = true;
  return snapshot;
}

inline bool apply_inbox_refresh_snapshot(CmdState &st,
                                         InboxRefreshSnapshot snapshot) {
  if (!snapshot.ok) {
    set_inbox_status(st, snapshot.error, true);
    return true;
  }

  const auto previous_inbox_rows = inbox_session_rows(st);
  const std::string previous_inbox_session_id =
      (!previous_inbox_rows.empty() &&
       st.inbox.selected_inbox_session < previous_inbox_rows.size())
          ? previous_inbox_rows[st.inbox.selected_inbox_session]
                .marshal_session_id
          : std::string{};

  st.inbox.operator_inbox = std::move(snapshot.operator_inbox);
  force_inbox_view(st);
  st.inbox.ok = true;
  st.inbox.error.clear();

  const auto inbox_rows = inbox_session_rows(st);
  if (!previous_inbox_session_id.empty()) {
    cuwacunu::hero::human_mcp::select_session_by_id(
        inbox_rows, previous_inbox_session_id,
        &st.inbox.selected_inbox_session);
  }
  clamp_inbox_state(st);
  (void)queue_inbox_detail_refresh(st);

  set_inbox_status(st, {}, false);
  return true;
}

inline void clamp_inbox_state(CmdState &st) {
  force_inbox_view(st);
  const auto inbox_rows = inbox_session_rows(st);
  if (inbox_rows.empty()) {
    st.inbox.selected_inbox_session = 0;
  } else if (st.inbox.selected_inbox_session >= inbox_rows.size()) {
    st.inbox.selected_inbox_session = inbox_rows.size() - 1;
  }
}

inline bool refresh_inbox_state(CmdState &st) {
  return apply_inbox_refresh_snapshot(
      st, collect_inbox_refresh_snapshot(st.inbox.app));
}

inline void set_inbox_view(CmdState &st, inbox_console_view_t target_view) {
  (void)target_view;
  force_inbox_view(st);
  clamp_inbox_state(st);
  (void)queue_inbox_detail_refresh(st);
}

inline void cycle_inbox_view(CmdState &st) {
  set_inbox_view(st, kInboxView);
}

inline bool select_prev_inbox_menu_item(CmdState &st) {
  (void)st;
  return false;
}

inline bool select_next_inbox_menu_item(CmdState &st) {
  (void)st;
  return false;
}

inline void cycle_inbox_phase_filter(CmdState &st) {
  cuwacunu::hero::human_mcp::cycle_operator_console_phase_filter(
      &st.inbox.phase_filter);
}

inline bool select_prev_inbox_row(CmdState &st) {
  if (st.inbox.selected_inbox_session == 0)
    return false;
  --st.inbox.selected_inbox_session;
  (void)queue_inbox_detail_refresh(st);
  return true;
}

inline bool select_next_inbox_row(CmdState &st) {
  const auto rows = inbox_session_rows(st);
  if (rows.empty() || st.inbox.selected_inbox_session + 1 >= rows.size()) {
    return false;
  }
  ++st.inbox.selected_inbox_session;
  (void)queue_inbox_detail_refresh(st);
  return true;
}

inline bool queue_inbox_refresh(CmdState &st) {
  if (st.inbox.refresh_pending) {
    st.inbox.refresh_requeue = true;
    return false;
  }
  st.inbox.refresh_pending = true;
  st.inbox.refresh_requeue = false;
  auto app = st.inbox.app;
  st.inbox.refresh_future =
      std::async(std::launch::async,
                 [app = std::move(app)]() mutable -> InboxRefreshSnapshot {
                   return collect_inbox_refresh_snapshot(std::move(app));
                 });
  return true;
}

inline bool queue_inbox_detail_refresh(CmdState &st) {
  const auto request = current_inbox_detail_request(st);
  if (!request.has_value()) {
    clear_inbox_detail_cache(st);
    return false;
  }
  if (st.inbox.detail_loaded && st.inbox.detail_kind == request->kind &&
      st.inbox.detail_session_id == request->session.marshal_session_id) {
    return false;
  }
  st.inbox.detail_kind = request->kind;
  st.inbox.detail_session_id = request->session.marshal_session_id;
  st.inbox.detail_text.clear();
  st.inbox.detail_error.clear();
  st.inbox.detail_loaded = false;
  if (st.inbox.detail_pending) {
    st.inbox.detail_requeue = true;
    return false;
  }
  st.inbox.detail_pending = true;
  st.inbox.detail_requeue = false;
  auto app = st.inbox.app;
  const auto kind = request->kind;
  const auto session = request->session;
  st.inbox.detail_future = std::async(
      std::launch::async,
      [app = std::move(app), kind, session]() mutable -> InboxDetailSnapshot {
        InboxDetailSnapshot snapshot{};
        snapshot.kind = kind;
        snapshot.marshal_session_id = session.marshal_session_id;
        std::string error{};
        switch (kind) {
        case inbox_detail_kind_t::InboxRequest:
          snapshot.ok =
              cuwacunu::hero::human_mcp::read_request_text_for_session(
                  session, &snapshot.text, &error);
          break;
        case inbox_detail_kind_t::InboxReview:
          snapshot.ok =
              cuwacunu::hero::human_mcp::read_summary_text_for_session(
                  session, &snapshot.text, &error);
          break;
        case inbox_detail_kind_t::None:
          snapshot.ok = true;
          break;
        }
        if (!snapshot.ok)
          snapshot.error = std::move(error);
        return snapshot;
      });
  return true;
}

inline bool poll_inbox_async_updates(CmdState &st) {
  bool dirty = false;
  if (st.inbox.refresh_pending && st.inbox.refresh_future.valid() &&
      st.inbox.refresh_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    InboxRefreshSnapshot snapshot = st.inbox.refresh_future.get();
    st.inbox.refresh_pending = false;
    dirty = apply_inbox_refresh_snapshot(st, std::move(snapshot)) || dirty;
    if (st.inbox.refresh_requeue) {
      st.inbox.refresh_requeue = false;
      (void)queue_inbox_refresh(st);
    }
  }
  if (st.inbox.detail_pending && st.inbox.detail_future.valid() &&
      st.inbox.detail_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    InboxDetailSnapshot snapshot = st.inbox.detail_future.get();
    st.inbox.detail_pending = false;
    const auto request = current_inbox_detail_request(st);
    if (request.has_value() && request->kind == snapshot.kind &&
        request->session.marshal_session_id == snapshot.marshal_session_id) {
      st.inbox.detail_kind = snapshot.kind;
      st.inbox.detail_session_id = snapshot.marshal_session_id;
      st.inbox.detail_text = std::move(snapshot.text);
      st.inbox.detail_error = std::move(snapshot.error);
      st.inbox.detail_loaded = true;
      dirty = true;
    }
    if (st.inbox.detail_requeue) {
      st.inbox.detail_requeue = false;
      (void)queue_inbox_detail_refresh(st);
    }
  }
  return dirty;
}

template <class AppendLog>
inline void append_inbox_show_lines(const CmdState &st,
                                    AppendLog &&append_log) {
  append_log("screen=inbox", "show", "#d8d8ff");
  append_log("view=Inbox"
             " focus=" +
                 inbox_focus_label(st.inbox.focus),
             "show", "#d8d8ff");
  if (st.inbox.refresh_pending) {
    append_log("inbox_refresh=background", "show", "#d8d8ff");
  }
  if (st.inbox.detail_pending) {
    append_log("inbox_detail=background", "show", "#d8d8ff");
  }
  append_log(
      "tracked_sessions=" + std::to_string(count_tracked_inbox_sessions(st)) +
          " inbox=" + std::to_string(count_inbox_sessions(st)),
      "show", "#d8d8ff");
  append_log("pending_requests=" +
                 std::to_string(count_pending_request_inbox_sessions(st)) +
                 " pending_reviews=" +
                 std::to_string(count_pending_review_inbox_sessions(st)) +
                 " operator_paused=" +
                 std::to_string(count_operator_paused_inbox_sessions(st)),
             "show", "#d8d8ff");
  const std::string marshal_session_id = preferred_inbox_session_id(st);
  if (!marshal_session_id.empty()) {
    append_log("selected_anchor_session_id=" + marshal_session_id, "show",
               "#d8d8ff");
  }
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
