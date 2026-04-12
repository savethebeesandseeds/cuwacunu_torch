#pragma once

#include <cstddef>
#include <cstdint>
#include <future>
#include <string>

#include "hero/human_hero/hero_human_tools.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class human_console_view_t : std::uint8_t { Inbox = 0 };

inline constexpr human_console_view_t kHumanInboxView =
    human_console_view_t::Inbox;
inline constexpr human_console_view_t kHumanLiveView = kHumanInboxView;
inline constexpr human_console_view_t kHumanHistoryView = kHumanInboxView;

// Compatibility aliases for the old iinuji Human cockpit lane names.
inline constexpr human_console_view_t kHumanOverviewView = kHumanInboxView;
inline constexpr human_console_view_t kHumanRequestsView = kHumanInboxView;
inline constexpr human_console_view_t kHumanReviewsView = kHumanInboxView;

enum class human_panel_focus_t : std::uint8_t {
  Menu = 0,
  Inbox = 1,
};

inline constexpr human_panel_focus_t kHumanMenuFocus =
    human_panel_focus_t::Menu;
inline constexpr human_panel_focus_t kHumanInboxFocus =
    human_panel_focus_t::Inbox;

inline bool human_is_inbox_view(human_console_view_t view) {
  return view == kHumanInboxView;
}

inline bool human_is_overview_view(human_console_view_t view) {
  return human_is_inbox_view(view);
}

inline bool human_is_requests_view(human_console_view_t view) {
  return human_is_inbox_view(view);
}

inline bool human_is_reviews_view(human_console_view_t view) {
  return human_is_inbox_view(view);
}

inline bool human_is_menu_focus(human_panel_focus_t focus) {
  return focus == kHumanMenuFocus;
}

inline bool human_is_inbox_focus(human_panel_focus_t focus) {
  return focus == kHumanInboxFocus;
}

inline std::string human_view_label(human_console_view_t view) {
  (void)view;
  return "Inbox";
}

inline std::string human_focus_label(human_panel_focus_t focus) {
  return human_is_menu_focus(focus) ? "menu" : "inbox";
}

enum class human_detail_kind_t : std::uint8_t {
  None = 0,
  InboxRequest = 1,
  InboxReview = 2,
};

struct HumanRefreshSnapshot {
  bool ok{false};
  std::string error{};
  cuwacunu::hero::human_mcp::human_operator_inbox_t inbox{};
};

struct HumanDetailSnapshot {
  bool ok{false};
  std::string error{};
  human_detail_kind_t kind{human_detail_kind_t::None};
  std::string marshal_session_id{};
  std::string text{};
};

struct HumanState {
  bool ok{false};
  std::string error{};
  cuwacunu::hero::human_mcp::app_context_t app{};
  cuwacunu::hero::human_mcp::human_operator_inbox_t inbox{};
  human_console_view_t view{kHumanInboxView};
  human_panel_focus_t focus{kHumanMenuFocus};
  cuwacunu::hero::human_mcp::operator_console_phase_filter_t phase_filter{
      cuwacunu::hero::human_mcp::operator_console_phase_filter_t::all};
  std::size_t selected_inbox_session{0};
  std::string status{};
  bool status_is_error{false};
  std::uint64_t status_expires_at_ms{0};
  bool refresh_pending{false};
  bool refresh_requeue{false};
  std::future<HumanRefreshSnapshot> refresh_future{};
  bool detail_pending{false};
  bool detail_requeue{false};
  std::future<HumanDetailSnapshot> detail_future{};
  human_detail_kind_t detail_kind{human_detail_kind_t::None};
  std::string detail_session_id{};
  std::string detail_text{};
  std::string detail_error{};
  bool detail_loaded{false};
};

enum class human_inbox_popup_action_kind_t : std::uint8_t {
  PauseSession = 0,
  ResumeOperatorPause = 1,
  ContinueIdleSession = 2,
  TerminateSession = 3,
  AnswerRequest = 4,
  ResolveRequest = 5,
  TerminateGovernanceRequest = 6,
  AcknowledgeReview = 7,
};

struct human_inbox_popup_action_t {
  std::string label{};
  human_inbox_popup_action_kind_t kind =
      human_inbox_popup_action_kind_t::PauseSession;
};

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
