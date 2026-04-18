#pragma once

#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

#include "hero/runtime_hero/runtime_job.h"
#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline constexpr std::uint64_t kWorkbenchTransientStatusTtlMs = 3200;

inline std::uint64_t workbench_status_now_ms() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::steady_clock::now().time_since_epoch())
          .count());
}

inline bool
workbench_session_has_pending_request(const CmdState &st,
                                      std::string_view marshal_session_id) {
  return operator_session_has_pending_request(st, marshal_session_id);
}

inline bool
workbench_session_has_pending_review(const CmdState &st,
                                     std::string_view marshal_session_id) {
  return operator_session_has_pending_review(st, marshal_session_id);
}

inline bool workbench_session_is_operator_paused(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return operator_session_is_operator_paused(session);
}

inline bool workbench_session_is_working(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return operator_session_is_working(session);
}

inline std::vector<cuwacunu::hero::marshal::marshal_session_record_t>
workbench_rows(const CmdState &st) {
  return operator_action_queue_rows(st);
}

inline std::size_t count_tracked_workbench_sessions(const CmdState &st) {
  return count_tracked_operator_sessions(st);
}

inline std::size_t count_workbench_sessions(const CmdState &st) {
  return count_operator_action_queue_rows(st);
}

inline std::size_t
count_operator_paused_workbench_sessions(const CmdState &st) {
  return count_operator_paused_sessions(st);
}

inline std::size_t
count_pending_request_workbench_sessions(const CmdState &st) {
  return count_pending_operator_requests(st);
}

inline std::size_t count_pending_review_workbench_sessions(const CmdState &st) {
  return count_pending_operator_reviews(st);
}

inline void focus_workbench_navigation(CmdState &st) {
  st.workbench.focus = kWorkbenchNavigationFocus;
}

inline void focus_workbench_worklist(CmdState &st) {
  st.workbench.focus = kWorkbenchWorklistFocus;
}

inline std::size_t workbench_booster_action_count() { return 3u; }

inline void clear_workbench_detail_cache(CmdState &st);
inline bool queue_workbench_detail_refresh(CmdState &st);

inline std::vector<workbench_section_t> workbench_sections() {
  return {kWorkbenchQueueSection, kWorkbenchBoosterSection};
}

inline std::size_t workbench_section_index(workbench_section_t section) {
  return workbench_is_booster_section(section) ? 1u : 0u;
}

inline workbench_section_t workbench_section_from_index(std::size_t index) {
  return index == 0u ? kWorkbenchQueueSection : kWorkbenchBoosterSection;
}

inline bool select_prev_workbench_section(CmdState &st) {
  const std::size_t current = workbench_section_index(st.workbench.section);
  if (current == 0u)
    return false;
  st.workbench.section = workbench_section_from_index(current - 1u);
  clear_workbench_detail_cache(st);
  if (workbench_is_queue_section(st.workbench.section)) {
    (void)queue_workbench_detail_refresh(st);
  }
  return true;
}

inline bool select_next_workbench_section(CmdState &st) {
  const std::size_t current = workbench_section_index(st.workbench.section);
  if (current + 1u >= workbench_sections().size())
    return false;
  st.workbench.section = workbench_section_from_index(current + 1u);
  clear_workbench_detail_cache(st);
  if (workbench_is_queue_section(st.workbench.section)) {
    (void)queue_workbench_detail_refresh(st);
  }
  return true;
}

inline workbench_booster_action_kind_t
workbench_booster_action_from_index(std::size_t index) {
  switch (index) {
  case 0u:
    return workbench_booster_action_kind_t::LaunchMarshal;
  case 1u:
    return workbench_booster_action_kind_t::LaunchCampaign;
  case 2u:
  default:
    return workbench_booster_action_kind_t::DevNukeReset;
  }
}

inline std::string
workbench_booster_action_label(workbench_booster_action_kind_t action) {
  switch (action) {
  case workbench_booster_action_kind_t::LaunchMarshal:
    return "Launch Marshal";
  case workbench_booster_action_kind_t::LaunchCampaign:
    return "Launch Campaign";
  case workbench_booster_action_kind_t::DevNukeReset:
    return "Dev Nuke Reset";
  }
  return "Booster";
}

inline std::string
workbench_booster_action_summary(workbench_booster_action_kind_t action) {
  switch (action) {
  case workbench_booster_action_kind_t::LaunchMarshal:
    return "Start a detached Marshal Hero session from an existing objective "
           "bundle under objective roots, or scaffold a new objective bundle "
           "from defaults and open it in the editor before launch.";
  case workbench_booster_action_kind_t::LaunchCampaign:
    return "Prepare a Runtime Hero campaign from the default campaign.dsl, an "
           "existing campaign file, or a temp copy created from default. "
           "Edit the campaign when writable, review binding/session/reset "
           "options, then confirm launch.";
  case workbench_booster_action_kind_t::DevNukeReset:
    return "Danger zone. Snapshot configured runtime state when enabled, then "
           "clear runtime dump folders and reload Config Hero state.";
  }
  return {};
}

inline bool
workbench_booster_action_is_danger(workbench_booster_action_kind_t action) {
  return action == workbench_booster_action_kind_t::DevNukeReset;
}

inline workbench_booster_action_kind_t
selected_workbench_booster_action(const CmdState &st) {
  return workbench_booster_action_from_index(
      st.workbench.selected_booster_action);
}

inline std::optional<std::size_t>
selected_workbench_worklist_line(const CmdState &st) {
  if (workbench_is_queue_section(st.workbench.section)) {
    return selected_operator_visible_row_index(st);
  }
  if (workbench_booster_action_count() == 0)
    return std::nullopt;
  return std::min(st.workbench.selected_booster_action,
                  workbench_booster_action_count() - 1u);
}

inline bool select_prev_workbench_booster_action(CmdState &st) {
  if (st.workbench.selected_booster_action == 0u)
    return false;
  --st.workbench.selected_booster_action;
  return true;
}

inline bool select_next_workbench_booster_action(CmdState &st) {
  if (st.workbench.selected_booster_action + 1u >=
      workbench_booster_action_count()) {
    return false;
  }
  ++st.workbench.selected_booster_action;
  return true;
}

inline std::vector<std::string>
workbench_booster_search_roots(const CmdState &st, bool include_objectives) {
  std::vector<std::string> roots{};
  std::set<std::string> seen{};
  const auto add_root = [&](std::string root) {
    root = trim_copy(root);
    if (root.empty())
      return;
    if (seen.insert(root).second)
      roots.push_back(std::move(root));
  };
  for (const auto &root : st.config.default_roots)
    add_root(root);
  for (const auto &root : st.config.temp_roots)
    add_root(root);
  if (include_objectives) {
    for (const auto &root : st.config.objective_roots)
      add_root(root);
  }
  if (roots.empty()) {
    add_root("src/config/instructions/defaults");
    add_root("src/config/instructions/temp");
    if (include_objectives)
      add_root("src/config/instructions/objectives");
  }
  return roots;
}

inline std::vector<std::string> workbench_objective_roots(const CmdState &st) {
  std::vector<std::string> roots{};
  std::set<std::string> seen{};
  const auto add_root = [&](std::string root) {
    root = trim_copy(root);
    if (root.empty())
      return;
    if (seen.insert(root).second)
      roots.push_back(std::move(root));
  };
  for (const auto &root : st.config.objective_roots)
    add_root(root);
  if (roots.empty()) {
    add_root("src/config/instructions/objectives");
  }
  return roots;
}

inline std::vector<std::string>
workbench_find_paths_with_suffix(const CmdState &st, std::string_view suffix,
                                 bool include_objectives) {
  std::vector<std::string> out{};
  std::set<std::string> seen{};
  for (const auto &root_raw :
       workbench_booster_search_roots(st, include_objectives)) {
    const std::filesystem::path root(root_raw);
    std::error_code ec{};
    if (!std::filesystem::exists(root, ec))
      continue;
    for (std::filesystem::recursive_directory_iterator it(root, ec), end;
         it != end; it.increment(ec)) {
      if (ec)
        break;
      if (!it->is_regular_file(ec))
        continue;
      const std::string path = it->path().lexically_normal().string();
      if (path.size() < suffix.size())
        continue;
      if (path.compare(path.size() - suffix.size(), suffix.size(),
                       std::string(suffix)) != 0) {
        continue;
      }
      if (seen.insert(path).second)
        out.push_back(path);
    }
  }
  std::sort(out.begin(), out.end());
  return out;
}

inline std::vector<std::string> workbench_find_paths_in_roots_with_suffixes(
    const std::vector<std::string> &roots,
    const std::vector<std::string_view> &suffixes) {
  std::vector<std::string> out{};
  std::set<std::string> seen{};
  for (const auto &root_raw : roots) {
    const std::filesystem::path root(root_raw);
    std::error_code ec{};
    if (!std::filesystem::exists(root, ec))
      continue;
    for (std::filesystem::recursive_directory_iterator it(root, ec), end;
         it != end; it.increment(ec)) {
      if (ec)
        break;
      if (!it->is_regular_file(ec))
        continue;
      const std::string path = it->path().lexically_normal().string();
      bool matched = false;
      for (const auto suffix : suffixes) {
        if (path.size() >= suffix.size() &&
            path.compare(path.size() - suffix.size(), suffix.size(),
                         std::string(suffix)) == 0) {
          matched = true;
          break;
        }
      }
      if (!matched)
        continue;
      if (seen.insert(path).second)
        out.push_back(path);
    }
  }
  std::sort(out.begin(), out.end());
  return out;
}

inline std::vector<std::string>
workbench_available_campaign_paths(const CmdState &st) {
  return workbench_find_paths_with_suffix(st, ".campaign.dsl",
                                          /*include_objectives=*/true);
}

inline std::vector<std::string>
workbench_available_objective_dsl_paths(const CmdState &st) {
  return workbench_find_paths_in_roots_with_suffixes(
      workbench_objective_roots(st), {".marshal.dsl", ".objective.dsl"});
}

inline std::string
workbench_preferred_default_campaign_path(const CmdState &st) {
  const auto paths = workbench_available_campaign_paths(st);
  for (const auto &path : paths) {
    if (path.find("default.iitepi.campaign.dsl") != std::string::npos)
      return path;
  }
  return paths.empty() ? std::string{} : paths.front();
}

inline std::string
workbench_preferred_default_objective_dsl_path(const CmdState &st) {
  const auto paths =
      workbench_find_paths_with_suffix(st, ".objective.dsl",
                                       /*include_objectives=*/false);
  for (const auto &path : paths) {
    if (path.find("default.marshal.objective.dsl") != std::string::npos)
      return path;
  }
  return paths.empty() ? std::string{} : paths.front();
}

inline std::string
workbench_preferred_default_objective_md_path(const CmdState &st) {
  const auto paths =
      workbench_find_paths_with_suffix(st, ".objective.md",
                                       /*include_objectives=*/false);
  for (const auto &path : paths) {
    if (path.find("default.marshal.objective.md") != std::string::npos)
      return path;
  }
  return paths.empty() ? std::string{} : paths.front();
}

inline std::string
workbench_preferred_default_guidance_md_path(const CmdState &st) {
  const auto paths =
      workbench_find_paths_with_suffix(st, ".guidance.md",
                                       /*include_objectives=*/false);
  for (const auto &path : paths) {
    if (path.find("default.marshal.guidance.md") != std::string::npos)
      return path;
  }
  return paths.empty() ? std::string{} : paths.front();
}

inline std::string workbench_read_file_text(const std::string &path) {
  std::ifstream ifs(path);
  if (!ifs)
    return {};
  return std::string(std::istreambuf_iterator<char>(ifs),
                     std::istreambuf_iterator<char>());
}

inline std::string workbench_objective_display_name(std::string_view path) {
  std::string name = std::filesystem::path(path).filename().string();
  constexpr std::string_view kMarshalSuffix = ".marshal.dsl";
  constexpr std::string_view kObjectiveSuffix = ".objective.dsl";
  if (name.size() > kMarshalSuffix.size() &&
      name.compare(name.size() - kMarshalSuffix.size(), kMarshalSuffix.size(),
                   std::string(kMarshalSuffix)) == 0) {
    name.resize(name.size() - kMarshalSuffix.size());
  } else if (name.size() > kObjectiveSuffix.size() &&
             name.compare(name.size() - kObjectiveSuffix.size(),
                          kObjectiveSuffix.size(),
                          std::string(kObjectiveSuffix)) == 0) {
    name.resize(name.size() - kObjectiveSuffix.size());
  }
  return name.empty() ? std::string(path) : name;
}

inline std::vector<std::string>
workbench_campaign_binding_ids(const std::string &campaign_path) {
  std::vector<std::string> out{};
  const std::string text = workbench_read_file_text(campaign_path);
  if (text.empty())
    return out;
  std::set<std::string> seen{};
  std::size_t pos = 0u;
  while ((pos = text.find("BIND ", pos)) != std::string::npos) {
    pos += 5u;
    while (pos < text.size() &&
           std::isspace(static_cast<unsigned char>(text[pos]))) {
      ++pos;
    }
    const std::size_t start = pos;
    while (pos < text.size()) {
      const char ch = text[pos];
      if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_' ||
            ch == '-' || ch == '.')) {
        break;
      }
      ++pos;
    }
    if (pos > start) {
      const std::string bind_id = text.substr(start, pos - start);
      if (seen.insert(bind_id).second)
        out.push_back(bind_id);
    }
  }
  return out;
}

inline std::string workbench_suggest_temp_file_name(std::string stem,
                                                    std::string suffix) {
  const auto now_ms = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  return std::move(stem) + "." +
         cuwacunu::hero::runtime::base36_lower_u64(now_ms) + std::move(suffix);
}

inline std::filesystem::path
workbench_temp_absolute_path(const CmdState &st,
                             const std::string &relative_path) {
  if (!st.config.temp_roots.empty()) {
    return std::filesystem::path(st.config.temp_roots.front()) / relative_path;
  }
  return std::filesystem::path("src/config/instructions/temp") / relative_path;
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_visible_workbench_session_record(const CmdState &st) {
  return selected_operator_visible_session_record(st);
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_workbench_request_record(const CmdState &st) {
  return selected_operator_request_record(st);
}

inline std::optional<cuwacunu::hero::marshal::marshal_session_record_t>
selected_workbench_review_record(const CmdState &st) {
  return selected_operator_review_record(st);
}

struct workbench_detail_request_t {
  workbench_detail_kind_t kind{workbench_detail_kind_t::None};
  cuwacunu::hero::marshal::marshal_session_record_t session{};
};

inline void clear_workbench_detail_cache(CmdState &st) {
  st.workbench.detail_kind = workbench_detail_kind_t::None;
  st.workbench.detail_session_id.clear();
  st.workbench.detail_text.clear();
  st.workbench.detail_error.clear();
  st.workbench.detail_loaded = false;
  st.workbench.detail_requeue = false;
}

inline std::optional<workbench_detail_request_t>
current_workbench_detail_request(const CmdState &st) {
  if (workbench_is_booster_section(st.workbench.section))
    return std::nullopt;
  if (const auto request = selected_workbench_request_record(st);
      request.has_value()) {
    return workbench_detail_request_t{workbench_detail_kind_t::Request,
                                      *request};
  }
  if (const auto review = selected_workbench_review_record(st);
      review.has_value()) {
    return workbench_detail_request_t{workbench_detail_kind_t::Review, *review};
  }
  return std::nullopt;
}

inline bool queue_workbench_detail_refresh(CmdState &st);
inline bool poll_workbench_async_updates(CmdState &st);

inline void
push_workbench_action(std::vector<workbench_popup_action_t> &actions,
                      std::string label, workbench_popup_action_kind_t kind,
                      bool enabled = true, std::string disabled_status = {}) {
  actions.push_back(workbench_popup_action_t{
      .label = std::move(label),
      .kind = kind,
      .enabled = enabled,
      .disabled_status = std::move(disabled_status),
  });
}

inline std::vector<workbench_popup_action_t>
workbench_popup_actions(const CmdState &st) {
  std::vector<workbench_popup_action_t> actions{};
  const auto selected = selected_operator_visible_session_record(st);
  if (!selected.has_value())
    return actions;

  const bool has_pending_request =
      workbench_session_has_pending_request(st, selected->marshal_session_id);
  const bool clarification_request =
      has_pending_request &&
      cuwacunu::hero::human_mcp::is_clarification_work_gate(
          selected->work_gate);
  const bool governance_request = has_pending_request && !clarification_request;
  const bool operator_paused = workbench_session_is_operator_paused(*selected);
  const bool working_session = workbench_session_is_working(*selected);
  const bool pending_review =
      workbench_session_has_pending_review(st, selected->marshal_session_id);
  const bool review_continue_ready = pending_review &&
                                     selected->lifecycle == "live" &&
                                     selected->activity == "review";
  const bool review_archive_ready =
      review_continue_ready && trim_copy(selected->campaign_cursor).empty() &&
      !cuwacunu::hero::marshal::marshal_session_has_active_campaign(*selected);

  push_workbench_action(
      actions, "Pause session", workbench_popup_action_kind_t::PauseSession,
      working_session, "Pause is only available while the session is active.");
  push_workbench_action(
      actions, "Resume operator pause",
      workbench_popup_action_kind_t::ResumeOperatorPause, operator_paused,
      "Resume is only available for operator-paused sessions.");
  push_workbench_action(
      actions, "Message session", workbench_popup_action_kind_t::MessageSession,
      review_continue_ready,
      "Messaging from Workbench is available while a review-ready report is "
      "still pending. Historic acknowledged reports can still be messaged from "
      "Runtime.");
  push_workbench_action(
      actions, "Terminate session",
      workbench_popup_action_kind_t::TerminateSession,
      working_session || operator_paused,
      "Terminate session is only available for active or operator-paused "
      "sessions.");
  push_workbench_action(
      actions, "Answer request", workbench_popup_action_kind_t::AnswerRequest,
      clarification_request,
      "Answer request is only available for clarification requests.");
  push_workbench_action(
      actions, "Resolve request", workbench_popup_action_kind_t::ResolveRequest,
      governance_request,
      "Resolve request is only available for governance requests.");
  push_workbench_action(
      actions, "Terminate request",
      workbench_popup_action_kind_t::TerminateGovernanceRequest,
      governance_request,
      "Terminate request is only available for governance requests.");
  push_workbench_action(
      actions, "Acknowledge review",
      workbench_popup_action_kind_t::AcknowledgeReview, pending_review,
      "Acknowledge review is only available when a review report is pending.");
  push_workbench_action(
      actions, "Archive and release objective",
      workbench_popup_action_kind_t::ArchiveReview, review_archive_ready,
      "Archive and release objective is only available while a review-ready "
      "live session still owns the objective.");
  return actions;
}

inline bool workbench_has_visible_status(const WorkbenchState &workbench) {
  if (workbench.status.empty())
    return false;
  if (workbench.status_is_error)
    return true;
  if (workbench.status_expires_at_ms == 0)
    return true;
  return workbench_status_now_ms() < workbench.status_expires_at_ms;
}

inline std::string visible_workbench_status(const CmdState &st) {
  return workbench_has_visible_status(st.workbench) ? st.workbench.status
                                                    : std::string{};
}

inline void set_workbench_status(CmdState &st, std::string status,
                                 bool is_error) {
  st.workbench.status = std::move(status);
  st.workbench.status_is_error = is_error;
  if (st.workbench.status.empty()) {
    st.workbench.status_expires_at_ms = 0;
  } else if (is_error) {
    st.workbench.status_expires_at_ms = 0;
  } else {
    st.workbench.status_expires_at_ms =
        workbench_status_now_ms() + kWorkbenchTransientStatusTtlMs;
  }
  if (is_error) {
    st.workbench.ok = false;
    st.workbench.error = st.workbench.status;
  } else {
    st.workbench.error.clear();
  }
}

inline void clamp_workbench_state(CmdState &st);

inline WorkbenchRefreshSnapshot collect_workbench_refresh_snapshot(
    cuwacunu::hero::human_mcp::app_context_t app) {
  WorkbenchRefreshSnapshot snapshot{};
  if (app.defaults.marshal_root.empty()) {
    snapshot.error = "Workbench is not configured.";
    return snapshot;
  }

  cuwacunu::hero::human_mcp::human_operator_inbox_t inbox{};
  if (!cuwacunu::hero::human_mcp::collect_human_operator_inbox(
          app, &inbox, true, &snapshot.error)) {
    if (snapshot.error.empty()) {
      snapshot.error = "failed to refresh Workbench";
    }
    return snapshot;
  }

  cuwacunu::hero::human_mcp::sort_sessions_newest_first(&inbox.all_sessions);
  cuwacunu::hero::human_mcp::sort_sessions_newest_first(
      &inbox.actionable_requests);
  cuwacunu::hero::human_mcp::sort_sessions_newest_first(
      &inbox.unacknowledged_summaries);
  snapshot.operator_feed = operator_session_feed_from_inbox(std::move(inbox));
  snapshot.ok = true;
  return snapshot;
}

inline bool
apply_workbench_refresh_snapshot(CmdState &st,
                                 WorkbenchRefreshSnapshot snapshot) {
  if (!snapshot.ok) {
    set_workbench_status(st, snapshot.error, true);
    return true;
  }

  const auto previous_workbench_rows = workbench_rows(st);
  const std::string previous_workbench_session_id =
      (!previous_workbench_rows.empty() &&
       st.workbench.selected_session_row < previous_workbench_rows.size())
          ? previous_workbench_rows[st.workbench.selected_session_row]
                .marshal_session_id
          : std::string{};

  st.workbench.operator_feed = std::move(snapshot.operator_feed);
  st.workbench.ok = true;
  st.workbench.error.clear();

  const auto rows = workbench_rows(st);
  if (!previous_workbench_session_id.empty()) {
    cuwacunu::hero::human_mcp::select_session_by_id(
        rows, previous_workbench_session_id,
        &st.workbench.selected_session_row);
  }
  clamp_workbench_state(st);
  (void)queue_workbench_detail_refresh(st);

  set_workbench_status(st, {}, false);
  return true;
}

inline void clamp_workbench_state(CmdState &st) {
  if (st.workbench.selected_booster_action >=
      workbench_booster_action_count()) {
    st.workbench.selected_booster_action =
        workbench_booster_action_count() == 0
            ? 0
            : workbench_booster_action_count() - 1;
  }
  const auto rows = workbench_rows(st);
  if (rows.empty()) {
    st.workbench.selected_session_row = 0;
  } else if (st.workbench.selected_session_row >= rows.size()) {
    st.workbench.selected_session_row = rows.size() - 1;
  }
}

inline bool refresh_workbench_state(CmdState &st) {
  return apply_workbench_refresh_snapshot(
      st, collect_workbench_refresh_snapshot(st.workbench.app));
}

inline bool select_prev_workbench_row(CmdState &st) {
  if (st.workbench.selected_session_row == 0)
    return false;
  --st.workbench.selected_session_row;
  (void)queue_workbench_detail_refresh(st);
  return true;
}

inline bool select_next_workbench_row(CmdState &st) {
  const auto rows = workbench_rows(st);
  if (rows.empty() || st.workbench.selected_session_row + 1 >= rows.size()) {
    return false;
  }
  ++st.workbench.selected_session_row;
  (void)queue_workbench_detail_refresh(st);
  return true;
}

inline bool queue_workbench_refresh(CmdState &st) {
  if (st.workbench.refresh_pending) {
    st.workbench.refresh_requeue = true;
    return false;
  }
  st.workbench.refresh_pending = true;
  st.workbench.refresh_requeue = false;
  auto app = st.workbench.app;
  st.workbench.refresh_future =
      std::async(std::launch::async,
                 [app = std::move(app)]() mutable -> WorkbenchRefreshSnapshot {
                   return collect_workbench_refresh_snapshot(std::move(app));
                 });
  return true;
}

inline bool queue_workbench_detail_refresh(CmdState &st) {
  const auto request = current_workbench_detail_request(st);
  if (!request.has_value()) {
    clear_workbench_detail_cache(st);
    return false;
  }
  if (st.workbench.detail_loaded && st.workbench.detail_kind == request->kind &&
      st.workbench.detail_session_id == request->session.marshal_session_id) {
    return false;
  }
  st.workbench.detail_kind = request->kind;
  st.workbench.detail_session_id = request->session.marshal_session_id;
  st.workbench.detail_text.clear();
  st.workbench.detail_error.clear();
  st.workbench.detail_loaded = false;
  if (st.workbench.detail_pending) {
    st.workbench.detail_requeue = true;
    return false;
  }
  st.workbench.detail_pending = true;
  st.workbench.detail_requeue = false;
  auto app = st.workbench.app;
  const auto kind = request->kind;
  const auto session = request->session;
  st.workbench.detail_future = std::async(
      std::launch::async,
      [app = std::move(app), kind,
       session]() mutable -> WorkbenchDetailSnapshot {
        WorkbenchDetailSnapshot snapshot{};
        snapshot.kind = kind;
        snapshot.marshal_session_id = session.marshal_session_id;
        std::string error{};
        switch (kind) {
        case workbench_detail_kind_t::Request:
          snapshot.ok =
              cuwacunu::hero::human_mcp::read_request_text_for_session(
                  session, &snapshot.text, &error);
          break;
        case workbench_detail_kind_t::Review:
          snapshot.ok =
              cuwacunu::hero::human_mcp::read_summary_text_for_session(
                  session, &snapshot.text, &error);
          break;
        case workbench_detail_kind_t::None:
          snapshot.ok = true;
          break;
        }
        if (!snapshot.ok)
          snapshot.error = std::move(error);
        return snapshot;
      });
  return true;
}

inline bool poll_workbench_async_updates(CmdState &st) {
  bool dirty = false;
  if (st.workbench.refresh_pending && st.workbench.refresh_future.valid() &&
      st.workbench.refresh_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    WorkbenchRefreshSnapshot snapshot = st.workbench.refresh_future.get();
    st.workbench.refresh_pending = false;
    dirty = apply_workbench_refresh_snapshot(st, std::move(snapshot)) || dirty;
    if (st.workbench.refresh_requeue) {
      st.workbench.refresh_requeue = false;
      (void)queue_workbench_refresh(st);
    }
  }
  if (st.workbench.detail_pending && st.workbench.detail_future.valid() &&
      st.workbench.detail_future.wait_for(std::chrono::milliseconds(0)) ==
          std::future_status::ready) {
    WorkbenchDetailSnapshot snapshot = st.workbench.detail_future.get();
    st.workbench.detail_pending = false;
    const auto request = current_workbench_detail_request(st);
    if (request.has_value() && request->kind == snapshot.kind &&
        request->session.marshal_session_id == snapshot.marshal_session_id) {
      st.workbench.detail_kind = snapshot.kind;
      st.workbench.detail_session_id = snapshot.marshal_session_id;
      st.workbench.detail_text = std::move(snapshot.text);
      st.workbench.detail_error = std::move(snapshot.error);
      st.workbench.detail_loaded = true;
      dirty = true;
    }
    if (st.workbench.detail_requeue) {
      st.workbench.detail_requeue = false;
      (void)queue_workbench_detail_refresh(st);
    }
  }
  return dirty;
}

template <class AppendLog>
inline void append_workbench_show_lines(const CmdState &st,
                                        AppendLog &&append_log) {
  append_log("screen=workbench", "show", "#d8d8ff");
  append_log("focus=" + workbench_focus_label(st.workbench.focus), "show",
             "#d8d8ff");
  if (st.workbench.refresh_pending) {
    append_log("workbench_refresh=background", "show", "#d8d8ff");
  }
  if (st.workbench.detail_pending) {
    append_log("workbench_detail=background", "show", "#d8d8ff");
  }
  append_log("tracked_sessions=" +
                 std::to_string(count_tracked_workbench_sessions(st)) +
                 " workbench=" + std::to_string(count_workbench_sessions(st)),
             "show", "#d8d8ff");
  append_log("pending_requests=" +
                 std::to_string(count_pending_request_workbench_sessions(st)) +
                 " pending_reviews=" +
                 std::to_string(count_pending_review_workbench_sessions(st)) +
                 " operator_paused=" +
                 std::to_string(count_operator_paused_workbench_sessions(st)),
             "show", "#d8d8ff");
  const std::string marshal_session_id = preferred_operator_session_id(st);
  if (!marshal_session_id.empty()) {
    append_log("selected_anchor_session_id=" + marshal_session_id, "show",
               "#d8d8ff");
  }
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
