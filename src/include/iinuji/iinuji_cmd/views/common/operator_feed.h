#pragma once

#include <utility>
#include <vector>

#include "hero/human_hero/hero_human_tools.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct operator_session_feed_t {
  std::vector<cuwacunu::hero::marshal::marshal_session_record_t> sessions{};
  std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
      pending_requests{};
  std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
      pending_reviews{};
};

inline operator_session_feed_t operator_session_feed_from_inbox(
    cuwacunu::hero::human_mcp::human_operator_inbox_t inbox) {
  operator_session_feed_t feed{};
  feed.sessions = std::move(inbox.all_sessions);
  feed.pending_requests = std::move(inbox.actionable_requests);
  feed.pending_reviews = std::move(inbox.unacknowledged_summaries);
  return feed;
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
