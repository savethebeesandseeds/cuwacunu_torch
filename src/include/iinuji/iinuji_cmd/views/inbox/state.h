#pragma once

#include <cstddef>
#include <cstdint>
#include <future>
#include <string>

#include "hero/human_hero/hero_human_tools.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class inbox_console_view_t : std::uint8_t { Inbox = 0 };

inline constexpr inbox_console_view_t kInboxView =
    inbox_console_view_t::Inbox;
inline constexpr inbox_console_view_t kInboxLiveView = kInboxView;
inline constexpr inbox_console_view_t kInboxHistoryView = kInboxView;

// Compatibility aliases for the old iinuji Human cockpit lane names.
inline constexpr inbox_console_view_t kInboxOverviewView = kInboxView;
inline constexpr inbox_console_view_t kInboxRequestsView = kInboxView;
inline constexpr inbox_console_view_t kInboxReviewsView = kInboxView;

enum class inbox_panel_focus_t : std::uint8_t {
  Menu = 0,
  Inbox = 1,
};

inline constexpr inbox_panel_focus_t kInboxMenuFocus =
    inbox_panel_focus_t::Menu;
inline constexpr inbox_panel_focus_t kInboxFocus =
    inbox_panel_focus_t::Inbox;

inline bool inbox_is_view(inbox_console_view_t view) {
  return view == kInboxView;
}

inline bool inbox_is_overview_view(inbox_console_view_t view) {
  return inbox_is_view(view);
}

inline bool inbox_is_requests_view(inbox_console_view_t view) {
  return inbox_is_view(view);
}

inline bool inbox_is_reviews_view(inbox_console_view_t view) {
  return inbox_is_view(view);
}

inline bool inbox_is_menu_focus(inbox_panel_focus_t focus) {
  return focus == kInboxMenuFocus;
}

inline bool inbox_is_inbox_focus(inbox_panel_focus_t focus) {
  return focus == kInboxFocus;
}

inline std::string inbox_view_label(inbox_console_view_t view) {
  (void)view;
  return "Inbox";
}

inline std::string inbox_focus_label(inbox_panel_focus_t focus) {
  return inbox_is_menu_focus(focus) ? "menu" : "inbox";
}

enum class inbox_detail_kind_t : std::uint8_t {
  None = 0,
  InboxRequest = 1,
  InboxReview = 2,
};

struct InboxRefreshSnapshot {
  bool ok{false};
  std::string error{};
  cuwacunu::hero::human_mcp::human_operator_inbox_t operator_inbox{};
};

struct InboxDetailSnapshot {
  bool ok{false};
  std::string error{};
  inbox_detail_kind_t kind{inbox_detail_kind_t::None};
  std::string marshal_session_id{};
  std::string text{};
};

struct InboxState {
  bool ok{false};
  std::string error{};
  cuwacunu::hero::human_mcp::app_context_t app{};
  cuwacunu::hero::human_mcp::human_operator_inbox_t operator_inbox{};
  inbox_console_view_t view{kInboxView};
  inbox_panel_focus_t focus{kInboxMenuFocus};
  cuwacunu::hero::human_mcp::operator_console_phase_filter_t phase_filter{
      cuwacunu::hero::human_mcp::operator_console_phase_filter_t::all};
  std::size_t selected_inbox_session{0};
  std::string status{};
  bool status_is_error{false};
  std::uint64_t status_expires_at_ms{0};
  bool refresh_pending{false};
  bool refresh_requeue{false};
  std::future<InboxRefreshSnapshot> refresh_future{};
  bool detail_pending{false};
  bool detail_requeue{false};
  std::future<InboxDetailSnapshot> detail_future{};
  inbox_detail_kind_t detail_kind{inbox_detail_kind_t::None};
  std::string detail_session_id{};
  std::string detail_text{};
  std::string detail_error{};
  bool detail_loaded{false};
};

enum class inbox_popup_action_kind_t : std::uint8_t {
  PauseSession = 0,
  ResumeOperatorPause = 1,
  ContinueIdleSession = 2,
  TerminateSession = 3,
  AnswerRequest = 4,
  ResolveRequest = 5,
  TerminateGovernanceRequest = 6,
  AcknowledgeReview = 7,
};

struct inbox_popup_action_t {
  std::string label{};
  inbox_popup_action_kind_t kind =
      inbox_popup_action_kind_t::PauseSession;
  bool enabled{true};
  std::string disabled_status{};
};

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
