#pragma once

#include <cstddef>
#include <cstdint>
#include <future>
#include <string>

#include "hero/human_hero/hero_human_tools.h"
#include "iinuji/iinuji_cmd/views/common/operator_feed.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class workbench_section_t : std::uint8_t {
  Queue = 0,
  Booster = 1,
};

inline constexpr workbench_section_t kWorkbenchQueueSection =
    workbench_section_t::Queue;
inline constexpr workbench_section_t kWorkbenchBoosterSection =
    workbench_section_t::Booster;

inline bool workbench_is_queue_section(workbench_section_t section) {
  return section == kWorkbenchQueueSection;
}

inline bool workbench_is_booster_section(workbench_section_t section) {
  return section == kWorkbenchBoosterSection;
}

inline std::string workbench_section_label(workbench_section_t section) {
  return workbench_is_queue_section(section) ? "Inbox" : "Booster";
}

enum class workbench_panel_focus_t : std::uint8_t {
  Navigation = 0,
  Worklist = 1,
};

inline constexpr workbench_panel_focus_t kWorkbenchNavigationFocus =
    workbench_panel_focus_t::Navigation;
inline constexpr workbench_panel_focus_t kWorkbenchWorklistFocus =
    workbench_panel_focus_t::Worklist;

inline bool workbench_is_navigation_focus(workbench_panel_focus_t focus) {
  return focus == kWorkbenchNavigationFocus;
}

inline bool workbench_is_worklist_focus(workbench_panel_focus_t focus) {
  return focus == kWorkbenchWorklistFocus;
}

inline std::string workbench_focus_label(workbench_panel_focus_t focus) {
  return workbench_is_navigation_focus(focus) ? "navigation" : "worklist";
}

enum class workbench_detail_kind_t : std::uint8_t {
  None = 0,
  Request = 1,
  Review = 2,
};

struct WorkbenchRefreshSnapshot {
  bool ok{false};
  std::string error{};
  operator_session_feed_t operator_feed{};
};

struct WorkbenchDetailSnapshot {
  bool ok{false};
  std::string error{};
  workbench_detail_kind_t kind{workbench_detail_kind_t::None};
  std::string marshal_session_id{};
  std::string text{};
};

enum class workbench_booster_action_kind_t : std::uint8_t {
  LaunchMarshal = 0,
  LaunchCampaign = 1,
  DevNukeReset = 2,
};

struct WorkbenchEditorReturnState {
  bool active{false};
  workbench_booster_action_kind_t booster_action{
      workbench_booster_action_kind_t::LaunchMarshal};
  std::string status{};
};

struct WorkbenchMarshalLaunchDraft {
  bool active{false};
  bool launch_pending{false};
  std::string objective_name{};
  std::string objective_dsl_path{};
  std::string objective_md_path{};
  std::string guidance_md_path{};
  std::string campaign_path{};
  std::string marshal_model{};
  std::string marshal_reasoning_effort{};
  std::string launch_status{};
  bool created_new_bundle{false};
};

struct WorkbenchCampaignLaunchDraft {
  bool active{false};
  std::string campaign_path{};
  std::string binding_id{};
  bool reset_runtime_state{false};
  bool created_temp{false};
  bool path_is_writable{false};
};

struct WorkbenchState {
  bool ok{false};
  std::string error{};
  cuwacunu::hero::human_mcp::app_context_t app{};
  operator_session_feed_t operator_feed{};
  workbench_section_t section{kWorkbenchQueueSection};
  workbench_panel_focus_t focus{kWorkbenchNavigationFocus};
  std::size_t selected_session_row{0};
  std::size_t selected_booster_action{0};
  std::string status{};
  bool status_is_error{false};
  std::uint64_t status_expires_at_ms{0};
  bool refresh_pending{false};
  bool refresh_requeue{false};
  std::future<WorkbenchRefreshSnapshot> refresh_future{};
  bool detail_pending{false};
  bool detail_requeue{false};
  std::future<WorkbenchDetailSnapshot> detail_future{};
  workbench_detail_kind_t detail_kind{workbench_detail_kind_t::None};
  std::string detail_session_id{};
  std::string detail_text{};
  std::string detail_error{};
  bool detail_loaded{false};
  WorkbenchEditorReturnState editor_return{};
  WorkbenchMarshalLaunchDraft marshal_launch{};
  WorkbenchCampaignLaunchDraft campaign_launch{};
};

enum class workbench_popup_action_kind_t : std::uint8_t {
  PauseSession = 0,
  ResumeOperatorPause = 1,
  MessageSession = 2,
  TerminateSession = 3,
  AnswerRequest = 4,
  ResolveRequest = 5,
  TerminateGovernanceRequest = 6,
  AcknowledgeReview = 7,
  ArchiveReview = 8,
};

struct workbench_popup_action_t {
  std::string label{};
  workbench_popup_action_kind_t kind =
      workbench_popup_action_kind_t::PauseSession;
  bool enabled{true};
  std::string disabled_status{};
};

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
