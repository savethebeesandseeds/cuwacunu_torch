#pragma once

#include <algorithm>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "iinuji/iinuji_cmd/views/common/base.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool operator_session_has_pending_request(
    const CmdState &st, std::string_view marshal_session_id) {
  return cuwacunu::hero::human_mcp::find_session_index_by_id(
             st.workbench.operator_feed.pending_requests, marshal_session_id)
      .has_value();
}

inline bool operator_session_has_pending_review(
    const CmdState &st, std::string_view marshal_session_id) {
  return cuwacunu::hero::human_mcp::find_session_index_by_id(
             st.workbench.operator_feed.pending_reviews, marshal_session_id)
      .has_value();
}

inline bool operator_session_is_operator_paused(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return session.lifecycle == "live" && session.work_gate == "operator_pause";
}

inline bool operator_session_is_working(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return session.lifecycle == "live" &&
         (session.activity == "planning" ||
          session.campaign_status == "launching" ||
          session.campaign_status == "running" ||
          session.campaign_status == "stopping");
}

inline bool operator_session_is_workbench_member(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return operator_session_has_pending_request(st, session.marshal_session_id) ||
         operator_session_has_pending_review(st, session.marshal_session_id) ||
         operator_session_is_operator_paused(session);
}

inline std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
operator_action_queue_rows(const CmdState &st) {
  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> rows{};
  rows.reserve(st.workbench.operator_feed.sessions.size());
  for (const auto &session : st.workbench.operator_feed.sessions) {
    if (operator_session_is_workbench_member(st, session))
      rows.push_back(session);
  }
  return rows;
}

inline std::size_t count_tracked_operator_sessions(const CmdState &st) {
  return st.workbench.operator_feed.sessions.size();
}

inline std::size_t count_operator_action_queue_rows(const CmdState &st) {
  return operator_action_queue_rows(st).size();
}

inline std::size_t count_operator_paused_sessions(const CmdState &st) {
  return static_cast<std::size_t>(std::count_if(
      st.workbench.operator_feed.sessions.begin(),
      st.workbench.operator_feed.sessions.end(), [](const auto &session) {
        return operator_session_is_operator_paused(session);
      }));
}

inline std::size_t count_pending_operator_requests(const CmdState &st) {
  return st.workbench.operator_feed.pending_requests.size();
}

inline std::size_t count_pending_operator_reviews(const CmdState &st) {
  return st.workbench.operator_feed.pending_reviews.size();
}

inline std::optional<std::size_t>
selected_operator_visible_row_index(const CmdState &st) {
  const auto rows = operator_action_queue_rows(st);
  if (rows.empty())
    return std::nullopt;
  return std::min(st.workbench.selected_session_row, rows.size() - 1);
}

inline std::string current_operator_visible_session_id(const CmdState &st) {
  const auto rows = operator_action_queue_rows(st);
  if (rows.empty())
    return {};
  const std::size_t idx =
      std::min(st.workbench.selected_session_row, rows.size() - 1);
  return rows[idx].marshal_session_id;
}

inline std::string preferred_operator_session_id(const CmdState &st) {
  const std::string current = current_operator_visible_session_id(st);
  if (!current.empty())
    return current;
  const auto rows = operator_action_queue_rows(st);
  if (!rows.empty())
    return rows.front().marshal_session_id;
  if (!st.workbench.operator_feed.sessions.empty()) {
    return st.workbench.operator_feed.sessions.front().marshal_session_id;
  }
  return {};
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_operator_visible_session_record(const CmdState &st) {
  const auto rows = operator_action_queue_rows(st);
  if (rows.empty())
    return std::nullopt;
  const std::size_t idx =
      std::min(st.workbench.selected_session_row, rows.size() - 1);
  return rows[idx];
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_operator_session_record(const CmdState &st) {
  const std::string marshal_session_id = preferred_operator_session_id(st);
  if (marshal_session_id.empty())
    return std::nullopt;
  for (const auto &session : st.workbench.operator_feed.sessions) {
    if (session.marshal_session_id == marshal_session_id)
      return session;
  }
  for (const auto &session : st.workbench.operator_feed.pending_requests) {
    if (session.marshal_session_id == marshal_session_id)
      return session;
  }
  for (const auto &session : st.workbench.operator_feed.pending_reviews) {
    if (session.marshal_session_id == marshal_session_id)
      return session;
  }
  return std::nullopt;
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_operator_request_record(const CmdState &st) {
  const auto selected = selected_operator_visible_session_record(st);
  if (!selected.has_value())
    return std::nullopt;
  const auto idx = cuwacunu::hero::human_mcp::find_session_index_by_id(
      st.workbench.operator_feed.pending_requests,
      selected->marshal_session_id);
  if (!idx.has_value())
    return std::nullopt;
  return st.workbench.operator_feed.pending_requests[*idx];
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_operator_review_record(const CmdState &st) {
  const auto selected = selected_operator_visible_session_record(st);
  if (!selected.has_value())
    return std::nullopt;
  const auto idx = cuwacunu::hero::human_mcp::find_session_index_by_id(
      st.workbench.operator_feed.pending_reviews,
      selected->marshal_session_id);
  if (!idx.has_value())
    return std::nullopt;
  return st.workbench.operator_feed.pending_reviews[*idx];
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
