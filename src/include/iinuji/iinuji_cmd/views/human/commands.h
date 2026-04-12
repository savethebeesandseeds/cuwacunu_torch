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

inline constexpr std::uint64_t kHumanTransientStatusTtlMs = 3200;

inline std::uint64_t human_status_now_ms() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

inline bool
human_session_has_pending_request(const CmdState &st,
                                  std::string_view marshal_session_id) {
  return cuwacunu::hero::human_mcp::find_session_index_by_id(
             st.human.inbox.actionable_requests, marshal_session_id)
      .has_value();
}

inline bool
human_session_has_pending_review(const CmdState &st,
                                 std::string_view marshal_session_id) {
  return cuwacunu::hero::human_mcp::find_session_index_by_id(
             st.human.inbox.unacknowledged_summaries, marshal_session_id)
      .has_value();
}

inline bool human_session_is_operator_paused(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return session.phase == "paused" && session.pause_kind == "operator";
}

inline bool human_session_is_live_phase(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return session.phase == "active" || session.phase == "running_campaign";
}

inline bool human_session_is_inbox_member(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return human_session_has_pending_request(st, session.marshal_session_id) ||
         human_session_has_pending_review(st, session.marshal_session_id) ||
         human_session_is_operator_paused(session);
}

inline std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
human_inbox_session_rows(const CmdState &st) {
  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> rows{};
  rows.reserve(st.human.inbox.all_sessions.size());
  for (const auto &session : st.human.inbox.all_sessions) {
    if (human_session_is_inbox_member(st, session))
      rows.push_back(session);
  }
  return rows;
}

inline std::size_t count_tracked_human_sessions(const CmdState &st) {
  return st.human.inbox.all_sessions.size();
}

inline std::size_t count_inbox_human_sessions(const CmdState &st) {
  return human_inbox_session_rows(st).size();
}

inline std::size_t count_operator_paused_human_sessions(const CmdState &st) {
  return static_cast<std::size_t>(
      std::count_if(st.human.inbox.all_sessions.begin(),
                    st.human.inbox.all_sessions.end(), [](const auto &session) {
                      return human_session_is_operator_paused(session);
                    }));
}

inline std::size_t count_pending_request_human_sessions(const CmdState &st) {
  return st.human.inbox.actionable_requests.size();
}

inline std::size_t count_pending_review_human_sessions(const CmdState &st) {
  return st.human.inbox.unacknowledged_summaries.size();
}

// Compatibility helper for older code paths that treated idle/finished rows as
// "review state" sessions.
inline std::size_t count_review_state_human_sessions(const CmdState &st) {
  return count_pending_review_human_sessions(st);
}

inline std::size_t human_menu_index(human_console_view_t view) {
  (void)view;
  return 0;
}

inline human_console_view_t human_view_from_menu_index(std::size_t index) {
  (void)index;
  return kHumanInboxView;
}

inline void force_human_inbox_view(CmdState &st) {
  st.human.view = kHumanInboxView;
}

inline void focus_human_menu(CmdState &st) {
  force_human_inbox_view(st);
  st.human.focus = kHumanMenuFocus;
}

inline void focus_human_inbox(CmdState &st) {
  force_human_inbox_view(st);
  st.human.focus = kHumanInboxFocus;
}

inline std::string current_human_selected_session_id(const CmdState &st) {
  const auto rows = human_inbox_session_rows(st);
  if (rows.empty())
    return {};
  const std::size_t idx =
      std::min(st.human.selected_inbox_session, rows.size() - 1);
  return rows[idx].marshal_session_id;
}

inline std::string preferred_human_session_id(const CmdState &st) {
  const std::string current = current_human_selected_session_id(st);
  if (!current.empty())
    return current;
  const auto inbox_rows = human_inbox_session_rows(st);
  if (!inbox_rows.empty())
    return inbox_rows.front().marshal_session_id;
  if (!st.human.inbox.all_sessions.empty()) {
    return st.human.inbox.all_sessions.front().marshal_session_id;
  }
  return {};
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_visible_inbox_human_session_record(const CmdState &st) {
  const auto rows = human_inbox_session_rows(st);
  if (rows.empty())
    return std::nullopt;
  const std::size_t idx =
      std::min(st.human.selected_inbox_session, rows.size() - 1);
  return rows[idx];
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_visible_live_human_session_record(const CmdState &st) {
  return selected_visible_inbox_human_session_record(st);
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_visible_history_human_session_record(const CmdState &st) {
  return selected_visible_inbox_human_session_record(st);
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_visible_human_session_record(const CmdState &st) {
  return selected_visible_inbox_human_session_record(st);
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_human_anchor_session_record(const CmdState &st) {
  const std::string marshal_session_id = preferred_human_session_id(st);
  if (marshal_session_id.empty())
    return std::nullopt;
  for (const auto &session : st.human.inbox.all_sessions) {
    if (session.marshal_session_id == marshal_session_id)
      return session;
  }
  for (const auto &session : st.human.inbox.actionable_requests) {
    if (session.marshal_session_id == marshal_session_id)
      return session;
  }
  for (const auto &session : st.human.inbox.unacknowledged_summaries) {
    if (session.marshal_session_id == marshal_session_id)
      return session;
  }
  return std::nullopt;
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_human_request_record(const CmdState &st) {
  const auto selected = selected_visible_human_session_record(st);
  if (!selected.has_value())
    return std::nullopt;
  const auto idx = cuwacunu::hero::human_mcp::find_session_index_by_id(
      st.human.inbox.actionable_requests, selected->marshal_session_id);
  if (!idx.has_value())
    return std::nullopt;
  return st.human.inbox.actionable_requests[*idx];
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_human_review_record(const CmdState &st) {
  const auto selected = selected_visible_human_session_record(st);
  if (!selected.has_value())
    return std::nullopt;
  const auto idx = cuwacunu::hero::human_mcp::find_session_index_by_id(
      st.human.inbox.unacknowledged_summaries, selected->marshal_session_id);
  if (!idx.has_value())
    return std::nullopt;
  return st.human.inbox.unacknowledged_summaries[*idx];
}

struct human_detail_request_t {
  human_detail_kind_t kind{human_detail_kind_t::None};
  cuwacunu::hero::marshal::marshal_session_record_t session{};
};

inline void clear_human_detail_cache(CmdState &st) {
  st.human.detail_kind = human_detail_kind_t::None;
  st.human.detail_session_id.clear();
  st.human.detail_text.clear();
  st.human.detail_error.clear();
  st.human.detail_loaded = false;
  st.human.detail_requeue = false;
}

inline std::optional<human_detail_request_t>
current_human_detail_request(const CmdState &st) {
  if (const auto request = selected_human_request_record(st);
      request.has_value()) {
    return human_detail_request_t{human_detail_kind_t::InboxRequest, *request};
  }
  if (const auto review = selected_human_review_record(st);
      review.has_value()) {
    return human_detail_request_t{human_detail_kind_t::InboxReview, *review};
  }
  return std::nullopt;
}

inline bool queue_human_detail_refresh(CmdState &st);
inline bool poll_human_async_updates(CmdState &st);

inline void
push_human_inbox_action(std::vector<human_inbox_popup_action_t> &actions,
                        std::string label,
                        human_inbox_popup_action_kind_t kind) {
  actions.push_back(human_inbox_popup_action_t{
      .label = std::move(label),
      .kind = kind,
  });
}

inline std::vector<human_inbox_popup_action_t>
human_inbox_popup_actions(const CmdState &st) {
  std::vector<human_inbox_popup_action_t> actions{};
  const auto selected = selected_visible_human_session_record(st);
  if (!selected.has_value())
    return actions;

  if (human_session_has_pending_request(st, selected->marshal_session_id)) {
    if (cuwacunu::hero::human_mcp::is_clarification_pause_kind(
            selected->pause_kind)) {
      push_human_inbox_action(actions, "Answer request",
                              human_inbox_popup_action_kind_t::AnswerRequest);
      return actions;
    }
    push_human_inbox_action(actions, "Resolve request",
                            human_inbox_popup_action_kind_t::ResolveRequest);
    push_human_inbox_action(
        actions, "Terminate request",
        human_inbox_popup_action_kind_t::TerminateGovernanceRequest);
    return actions;
  }

  if (human_session_is_operator_paused(*selected)) {
    push_human_inbox_action(
        actions, "Resume operator pause",
        human_inbox_popup_action_kind_t::ResumeOperatorPause);
    push_human_inbox_action(actions, "Terminate session",
                            human_inbox_popup_action_kind_t::TerminateSession);
    return actions;
  }

  if (human_session_is_live_phase(*selected)) {
    push_human_inbox_action(actions, "Pause session",
                            human_inbox_popup_action_kind_t::PauseSession);
    push_human_inbox_action(actions, "Terminate session",
                            human_inbox_popup_action_kind_t::TerminateSession);
    return actions;
  }

  if (human_session_has_pending_review(st, selected->marshal_session_id)) {
    if (selected->phase == "idle") {
      push_human_inbox_action(
          actions, "Continue session with instruction",
          human_inbox_popup_action_kind_t::ContinueIdleSession);
    }
    push_human_inbox_action(actions, "Acknowledge review",
                            human_inbox_popup_action_kind_t::AcknowledgeReview);
  }
  return actions;
}

inline bool human_has_visible_status(const HumanState &human) {
  if (human.status.empty())
    return false;
  if (human.status_is_error)
    return true;
  if (human.status_expires_at_ms == 0)
    return true;
  return human_status_now_ms() < human.status_expires_at_ms;
}

inline std::string visible_human_status(const CmdState &st) {
  return human_has_visible_status(st.human) ? st.human.status : std::string{};
}

inline void set_human_status(CmdState &st, std::string status, bool is_error) {
  st.human.status = std::move(status);
  st.human.status_is_error = is_error;
  if (st.human.status.empty()) {
    st.human.status_expires_at_ms = 0;
  } else if (is_error) {
    st.human.status_expires_at_ms = 0;
  } else {
    st.human.status_expires_at_ms =
        human_status_now_ms() + kHumanTransientStatusTtlMs;
  }
  if (is_error) {
    st.human.ok = false;
    st.human.error = st.human.status;
  } else {
    st.human.error.clear();
  }
}

inline void clamp_human_state(CmdState &st);

inline HumanRefreshSnapshot
human_collect_refresh_snapshot(cuwacunu::hero::human_mcp::app_context_t app) {
  HumanRefreshSnapshot snapshot{};
  if (app.defaults.marshal_root.empty()) {
    snapshot.error = "Marshal cockpit is not configured.";
    return snapshot;
  }

  if (!cuwacunu::hero::human_mcp::collect_human_operator_inbox(
          app, &snapshot.inbox, true, &snapshot.error)) {
    if (snapshot.error.empty()) {
      snapshot.error = "failed to refresh Marshal cockpit";
    }
    return snapshot;
  }

  cuwacunu::hero::human_mcp::sort_sessions_newest_first(
      &snapshot.inbox.all_sessions);
  cuwacunu::hero::human_mcp::sort_sessions_newest_first(
      &snapshot.inbox.actionable_requests);
  cuwacunu::hero::human_mcp::sort_sessions_newest_first(
      &snapshot.inbox.unacknowledged_summaries);
  snapshot.ok = true;
  return snapshot;
}

inline bool human_apply_refresh_snapshot(CmdState &st,
                                         HumanRefreshSnapshot snapshot) {
  if (!snapshot.ok) {
    set_human_status(st, snapshot.error, true);
    return true;
  }

  const auto previous_inbox_rows = human_inbox_session_rows(st);
  const std::string previous_inbox_session_id =
      (!previous_inbox_rows.empty() &&
       st.human.selected_inbox_session < previous_inbox_rows.size())
          ? previous_inbox_rows[st.human.selected_inbox_session]
                .marshal_session_id
          : std::string{};

  st.human.inbox = std::move(snapshot.inbox);
  force_human_inbox_view(st);
  st.human.ok = true;
  st.human.error.clear();

  const auto inbox_rows = human_inbox_session_rows(st);
  if (!previous_inbox_session_id.empty()) {
    cuwacunu::hero::human_mcp::select_session_by_id(
        inbox_rows, previous_inbox_session_id,
        &st.human.selected_inbox_session);
  }
  clamp_human_state(st);
  (void)queue_human_detail_refresh(st);

  set_human_status(st, {}, false);
  return true;
}

inline void clamp_human_state(CmdState &st) {
  force_human_inbox_view(st);
  const auto inbox_rows = human_inbox_session_rows(st);
  if (inbox_rows.empty()) {
    st.human.selected_inbox_session = 0;
  } else if (st.human.selected_inbox_session >= inbox_rows.size()) {
    st.human.selected_inbox_session = inbox_rows.size() - 1;
  }
}

inline bool refresh_human_state(CmdState &st) {
  return human_apply_refresh_snapshot(
      st, human_collect_refresh_snapshot(st.human.app));
}

inline void set_human_view(CmdState &st, human_console_view_t target_view) {
  (void)target_view;
  force_human_inbox_view(st);
  clamp_human_state(st);
  (void)queue_human_detail_refresh(st);
}

inline void cycle_human_view(CmdState &st) {
  set_human_view(st, kHumanInboxView);
}

inline bool select_prev_human_menu_item(CmdState &st) {
  (void)st;
  return false;
}

inline bool select_next_human_menu_item(CmdState &st) {
  (void)st;
  return false;
}

inline void cycle_human_phase_filter(CmdState &st) {
  cuwacunu::hero::human_mcp::cycle_operator_console_phase_filter(
      &st.human.phase_filter);
}

inline bool select_prev_human_row(CmdState &st) {
  if (st.human.selected_inbox_session == 0)
    return false;
  --st.human.selected_inbox_session;
  (void)queue_human_detail_refresh(st);
  return true;
}

inline bool select_next_human_row(CmdState &st) {
  const auto rows = human_inbox_session_rows(st);
  if (rows.empty() || st.human.selected_inbox_session + 1 >= rows.size()) {
    return false;
  }
  ++st.human.selected_inbox_session;
  (void)queue_human_detail_refresh(st);
  return true;
}

inline bool queue_human_refresh(CmdState &st) {
  if (st.human.refresh_pending) {
    st.human.refresh_requeue = true;
    return false;
  }
  st.human.refresh_pending = true;
  st.human.refresh_requeue = false;
  auto app = st.human.app;
  st.human.refresh_future =
      std::async(std::launch::async,
                 [app = std::move(app)]() mutable -> HumanRefreshSnapshot {
                   return human_collect_refresh_snapshot(std::move(app));
                 });
  return true;
}

inline bool queue_human_detail_refresh(CmdState &st) {
  const auto request = current_human_detail_request(st);
  if (!request.has_value()) {
    clear_human_detail_cache(st);
    return false;
  }
  if (st.human.detail_loaded && st.human.detail_kind == request->kind &&
      st.human.detail_session_id == request->session.marshal_session_id) {
    return false;
  }
  st.human.detail_kind = request->kind;
  st.human.detail_session_id = request->session.marshal_session_id;
  st.human.detail_text.clear();
  st.human.detail_error.clear();
  st.human.detail_loaded = false;
  if (st.human.detail_pending) {
    st.human.detail_requeue = true;
    return false;
  }
  st.human.detail_pending = true;
  st.human.detail_requeue = false;
  auto app = st.human.app;
  const auto kind = request->kind;
  const auto session = request->session;
  st.human.detail_future = std::async(
      std::launch::async,
      [app = std::move(app), kind, session]() mutable -> HumanDetailSnapshot {
        HumanDetailSnapshot snapshot{};
        snapshot.kind = kind;
        snapshot.marshal_session_id = session.marshal_session_id;
        std::string error{};
        switch (kind) {
        case human_detail_kind_t::InboxRequest:
          snapshot.ok =
              cuwacunu::hero::human_mcp::read_request_text_for_session(
                  session, &snapshot.text, &error);
          break;
        case human_detail_kind_t::InboxReview:
          snapshot.ok =
              cuwacunu::hero::human_mcp::read_summary_text_for_session(
                  session, &snapshot.text, &error);
          break;
        case human_detail_kind_t::None:
          snapshot.ok = true;
          break;
        }
        if (!snapshot.ok)
          snapshot.error = std::move(error);
        return snapshot;
      });
  return true;
}

inline bool poll_human_async_updates(CmdState &st) {
  bool dirty = false;
  if (st.human.refresh_pending && st.human.refresh_future.valid() &&
      st.human.refresh_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    HumanRefreshSnapshot snapshot = st.human.refresh_future.get();
    st.human.refresh_pending = false;
    dirty = human_apply_refresh_snapshot(st, std::move(snapshot)) || dirty;
    if (st.human.refresh_requeue) {
      st.human.refresh_requeue = false;
      (void)queue_human_refresh(st);
    }
  }
  if (st.human.detail_pending && st.human.detail_future.valid() &&
      st.human.detail_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    HumanDetailSnapshot snapshot = st.human.detail_future.get();
    st.human.detail_pending = false;
    const auto request = current_human_detail_request(st);
    if (request.has_value() && request->kind == snapshot.kind &&
        request->session.marshal_session_id == snapshot.marshal_session_id) {
      st.human.detail_kind = snapshot.kind;
      st.human.detail_session_id = snapshot.marshal_session_id;
      st.human.detail_text = std::move(snapshot.text);
      st.human.detail_error = std::move(snapshot.error);
      st.human.detail_loaded = true;
      dirty = true;
    }
    if (st.human.detail_requeue) {
      st.human.detail_requeue = false;
      (void)queue_human_detail_refresh(st);
    }
  }
  return dirty;
}

template <class AppendLog>
inline void append_human_show_lines(const CmdState &st,
                                    AppendLog &&append_log) {
  append_log("screen=human", "show", "#d8d8ff");
  append_log("view=Inbox"
             " focus=" +
                 human_focus_label(st.human.focus),
             "show", "#d8d8ff");
  if (st.human.refresh_pending) {
    append_log("human_refresh=background", "show", "#d8d8ff");
  }
  if (st.human.detail_pending) {
    append_log("human_detail=background", "show", "#d8d8ff");
  }
  append_log(
      "tracked_sessions=" + std::to_string(count_tracked_human_sessions(st)) +
          " inbox=" + std::to_string(count_inbox_human_sessions(st)),
      "show", "#d8d8ff");
  append_log("pending_requests=" +
                 std::to_string(count_pending_request_human_sessions(st)) +
                 " pending_reviews=" +
                 std::to_string(count_pending_review_human_sessions(st)) +
                 " operator_paused=" +
                 std::to_string(count_operator_paused_human_sessions(st)),
             "show", "#d8d8ff");
  const std::string marshal_session_id = preferred_human_session_id(st);
  if (!marshal_session_id.empty()) {
    append_log("selected_anchor_session_id=" + marshal_session_id, "show",
               "#d8d8ff");
  }
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
