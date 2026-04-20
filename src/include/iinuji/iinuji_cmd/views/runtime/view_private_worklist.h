
#include <iomanip>
#include <sstream>
#include <string_view>
#include <vector>

#include "iinuji/iinuji_cmd/views/runtime/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct RuntimeWorklistPanel {
  std::vector<cuwacunu::iinuji::styled_text_line_t> lines{};
  std::optional<std::size_t> selected_line{};
};

struct RuntimeEventViewerPanel {
  std::vector<cuwacunu::iinuji::styled_text_line_t> lines{};
  std::optional<std::size_t> selected_line{};
  std::optional<std::size_t> selected_line_end{};
};

inline std::size_t count_runtime_active_sessions(const CmdState &st) {
  return static_cast<std::size_t>(std::count_if(
      st.runtime.sessions.begin(), st.runtime.sessions.end(),
      [](const auto &session) { return runtime_session_is_active(session); }));
}

inline std::size_t count_runtime_active_campaigns(const CmdState &st) {
  return static_cast<std::size_t>(
      std::count_if(st.runtime.campaigns.begin(), st.runtime.campaigns.end(),
                    [](const auto &campaign) {
                      return runtime_campaign_is_active(campaign);
                    }));
}

inline std::size_t count_runtime_active_jobs(const CmdState &st) {
  return static_cast<std::size_t>(std::count_if(
      st.runtime.jobs.begin(), st.runtime.jobs.end(),
      [](const auto &job) { return runtime_job_is_active(job); }));
}

inline std::string
runtime_session_ref_text(std::string_view marshal_session_id) {
  return marshal_session_id.empty() ? runtime_none_branch_label()
                                    : std::string(marshal_session_id);
}

inline std::string runtime_campaign_ref_text(std::string_view campaign_cursor) {
  return campaign_cursor.empty() ? runtime_none_branch_label()
                                 : std::string(campaign_cursor);
}

inline std::string runtime_job_parent_campaign_cursor(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  return runtime_campaign_cursor_for_job(st.runtime, job.job_cursor);
}

inline std::string runtime_campaign_parent_session_id(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign) {
  return runtime_session_id_for_campaign(st.runtime, campaign.campaign_cursor);
}

inline std::string runtime_job_parent_session_id(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  return runtime_session_id_for_job(st.runtime, job.job_cursor);
}

inline bool runtime_campaign_belongs_to_session(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign,
    std::string_view marshal_session_id) {
  return !marshal_session_id.empty() &&
         runtime_campaign_parent_session_id(st, campaign) == marshal_session_id;
}

inline bool runtime_job_belongs_to_campaign(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_job_record_t &job,
    std::string_view campaign_cursor) {
  return !campaign_cursor.empty() &&
         runtime_job_parent_campaign_cursor(st, job) == campaign_cursor;
}

inline bool runtime_job_belongs_to_session(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_job_record_t &job,
    std::string_view marshal_session_id) {
  return !marshal_session_id.empty() &&
         runtime_job_parent_session_id(st, job) == marshal_session_id;
}

inline bool runtime_campaign_is_detached(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign) {
  return runtime_campaign_parent_session_id(st, campaign).empty();
}

inline bool runtime_job_is_orphan(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  return runtime_job_parent_campaign_cursor(st, job).empty();
}

inline bool runtime_job_is_sessionless(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  return !runtime_job_is_orphan(st, job) &&
         runtime_job_parent_session_id(st, job).empty();
}

inline std::string format_age_since_ms_minutes(std::uint64_t timestamp_ms);

inline std::size_t
count_runtime_campaigns_for_session(const CmdState &st,
                                    std::string_view marshal_session_id) {
  if (const auto it = st.runtime.campaign_indices_by_session.find(
          std::string(marshal_session_id));
      it != st.runtime.campaign_indices_by_session.end()) {
    return it->second.size();
  }
  return 0;
}

inline std::size_t
count_runtime_jobs_for_campaign(const CmdState &st,
                                std::string_view campaign_cursor) {
  if (const auto it =
          st.runtime.job_indices_by_campaign.find(std::string(campaign_cursor));
      it != st.runtime.job_indices_by_campaign.end()) {
    return it->second.size();
  }
  return 0;
}

inline std::size_t
count_runtime_jobs_for_session(const CmdState &st,
                               std::string_view marshal_session_id) {
  if (const auto it = st.runtime.job_indices_by_session.find(
          std::string(marshal_session_id));
      it != st.runtime.job_indices_by_session.end()) {
    return it->second.size();
  }
  return 0;
}

inline std::vector<std::string>
runtime_campaign_lines_for_session(const CmdState &st,
                                   std::string_view marshal_session_id,
                                   std::size_t limit = 5) {
  std::vector<std::string> lines{};
  const auto it = st.runtime.campaign_indices_by_session.find(
      std::string(marshal_session_id));
  if (it == st.runtime.campaign_indices_by_session.end())
    return lines;
  for (const auto idx : it->second) {
    if (idx >= st.runtime.campaigns.size())
      continue;
    const auto &campaign = st.runtime.campaigns[idx];
    lines.push_back(campaign.campaign_cursor + " | " + campaign.state + " | " +
                    format_age_since_ms_minutes(campaign.updated_at_ms));
    if (lines.size() >= limit)
      break;
  }
  return lines;
}

inline std::vector<std::string>
runtime_job_lines_for_campaign(const CmdState &st,
                               std::string_view campaign_cursor,
                               std::size_t limit = 6) {
  std::vector<std::string> lines{};
  const auto it =
      st.runtime.job_indices_by_campaign.find(std::string(campaign_cursor));
  if (it == st.runtime.job_indices_by_campaign.end())
    return lines;
  for (const auto idx : it->second) {
    if (idx >= st.runtime.jobs.size())
      continue;
    const auto &job = st.runtime.jobs[idx];
    lines.push_back(job.job_cursor + " | " + job.state + " | " +
                    format_age_since_ms_minutes(job.updated_at_ms));
    if (lines.size() >= limit)
      break;
  }
  return lines;
}

inline std::vector<std::string>
runtime_job_lines_for_session(const CmdState &st,
                              std::string_view marshal_session_id,
                              std::size_t limit = 6) {
  std::vector<std::string> lines{};
  const auto it =
      st.runtime.job_indices_by_session.find(std::string(marshal_session_id));
  if (it == st.runtime.job_indices_by_session.end())
    return lines;
  for (const auto idx : it->second) {
    if (idx >= st.runtime.jobs.size())
      continue;
    const auto &job = st.runtime.jobs[idx];
    const std::string campaign_cursor =
        runtime_job_parent_campaign_cursor(st, job);
    lines.push_back(
        job.job_cursor + " | " + job.state + " | campaign=" +
        trim_to_width(runtime_campaign_ref_text(campaign_cursor), 22) + " | " +
        format_age_since_ms_minutes(job.updated_at_ms));
    if (lines.size() >= limit)
      break;
  }
  return lines;
}

inline std::string runtime_path_text(std::string_view path) {
  return path.empty() ? std::string("<none>") : std::string(path);
}

inline std::string runtime_excerpt_text(const std::string &text) {
  if (text.empty())
    return std::string("<unavailable>");

  const auto records = runtime_split_jsonl_records(text);
  if (records.empty())
    return std::string("<unavailable>");

  std::ostringstream out{};
  bool first = true;
  for (const auto &record : records) {
    const std::string record_text = trim_copy(record);
    if (record_text.empty())
      continue;
    if (!first)
      out << '\n';
    first = false;
    for (char c : record_text) {
      if (c == '\n' || c == '\r') {
        out << ' ';
      } else {
        out << c;
      }
    }
  }

  const std::string collapsed = out.str();
  return collapsed.empty() ? std::string("<unavailable>") : collapsed;
}

inline void
push_runtime_line(std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
                  std::string text,
                  cuwacunu::iinuji::text_line_emphasis_t emphasis =
                      cuwacunu::iinuji::text_line_emphasis_t::None) {
  out.push_back(cuwacunu::iinuji::styled_text_line_t{
      .text = std::move(text),
      .emphasis = emphasis,
  });
}

inline void push_runtime_segments(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::vector<cuwacunu::iinuji::styled_text_segment_t> segments,
    cuwacunu::iinuji::text_line_emphasis_t fallback_emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::None) {
  out.push_back(cuwacunu::iinuji::make_segmented_styled_text_line(
      std::move(segments), fallback_emphasis));
}

inline void push_runtime_segments_with_background(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::vector<cuwacunu::iinuji::styled_text_segment_t> segments,
    std::string background_color,
    cuwacunu::iinuji::text_line_emphasis_t fallback_emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::None) {
  auto line = cuwacunu::iinuji::make_segmented_styled_text_line(
      std::move(segments), fallback_emphasis);
  line.background_color = std::move(background_color);
  out.push_back(std::move(line));
}

inline void push_runtime_line_with_background(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, std::string text,
    std::string background_color,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::None) {
  out.push_back(cuwacunu::iinuji::styled_text_line_t{
      .text = std::move(text),
      .emphasis = emphasis,
      .segments = {},
      .background_color = std::move(background_color),
  });
}

inline cuwacunu::iinuji::text_line_emphasis_t
runtime_selected_emphasis(bool focused) {
  return focused ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                 : cuwacunu::iinuji::text_line_emphasis_t::Debug;
}

inline std::string runtime_menu_count_text(const CmdState &st,
                                           RuntimeLane lane) {
  std::ostringstream oss;
  if (runtime_is_device_lane(lane)) {
    oss << "1 host / " << st.runtime.device.gpus.size() << " gpu";
    if (st.runtime.device.gpus.size() != 1)
      oss << "s";
  } else if (runtime_is_session_lane(lane)) {
    oss << count_runtime_active_sessions(st) << " active / "
        << st.runtime.sessions.size() << " total";
  } else if (runtime_is_campaign_lane(lane)) {
    oss << count_runtime_active_campaigns(st) << " active / "
        << st.runtime.campaigns.size() << " total";
  } else {
    oss << count_runtime_active_jobs(st) << " active / "
        << st.runtime.jobs.size() << " total";
  }
  return oss.str();
}

inline std::string runtime_menu_row_text(const CmdState &st, RuntimeLane lane) {
  std::ostringstream oss;
  oss << (st.runtime.lane == lane ? "> " : "  ");
  oss << "[" << runtime_lane_label(lane) << "] "
      << runtime_menu_count_text(st, lane);
  return oss.str();
}

inline std::string runtime_scope_badge(bool active) {
  return active ? "[LIVE]" : "[HIST]";
}

inline std::string runtime_scope_status_text(
    cuwacunu::iinuji::text_line_emphasis_t status_emphasis) {
  switch (status_emphasis) {
  case cuwacunu::iinuji::text_line_emphasis_t::Error:
  case cuwacunu::iinuji::text_line_emphasis_t::MutedError:
  case cuwacunu::iinuji::text_line_emphasis_t::Fatal:
    return "err";
  case cuwacunu::iinuji::text_line_emphasis_t::Warning:
  case cuwacunu::iinuji::text_line_emphasis_t::MutedWarning:
    return "warn";
  default:
    return "ok";
  }
}

inline cuwacunu::iinuji::text_line_emphasis_t runtime_scope_status_emphasis(
    cuwacunu::iinuji::text_line_emphasis_t status_emphasis) {
  switch (status_emphasis) {
  case cuwacunu::iinuji::text_line_emphasis_t::Error:
    return cuwacunu::iinuji::text_line_emphasis_t::Error;
  case cuwacunu::iinuji::text_line_emphasis_t::Fatal:
    return cuwacunu::iinuji::text_line_emphasis_t::Fatal;
  case cuwacunu::iinuji::text_line_emphasis_t::MutedError:
    return cuwacunu::iinuji::text_line_emphasis_t::MutedError;
  case cuwacunu::iinuji::text_line_emphasis_t::Warning:
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  case cuwacunu::iinuji::text_line_emphasis_t::MutedWarning:
    return cuwacunu::iinuji::text_line_emphasis_t::MutedWarning;
  case cuwacunu::iinuji::text_line_emphasis_t::Success:
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  case cuwacunu::iinuji::text_line_emphasis_t::Info:
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  case cuwacunu::iinuji::text_line_emphasis_t::Debug:
    return cuwacunu::iinuji::text_line_emphasis_t::Debug;
  case cuwacunu::iinuji::text_line_emphasis_t::Accent:
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  default:
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
}

inline std::string
runtime_scope_badge(bool active,
                    cuwacunu::iinuji::text_line_emphasis_t status_emphasis) {
  return active ? "[LIVE:" + runtime_scope_status_text(status_emphasis) + "]"
                : "[HIST:" + runtime_scope_status_text(status_emphasis) + "]";
}

inline std::string runtime_session_scope_status_text(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    cuwacunu::iinuji::text_line_emphasis_t status_emphasis) {
  if (!runtime_session_is_active(session) && session.lifecycle == "live" &&
      session.activity == "review" && session.finish_reason == "interrupted") {
    return "interrupted";
  }
  return runtime_scope_status_text(status_emphasis);
}

inline std::string runtime_session_scope_badge(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    cuwacunu::iinuji::text_line_emphasis_t status_emphasis) {
  return runtime_session_is_active(session)
             ? "[LIVE:" +
                   runtime_session_scope_status_text(session, status_emphasis) +
                   "]"
             : "[HIST:" +
                   runtime_session_scope_status_text(session, status_emphasis) +
                   "]";
}

inline std::string runtime_campaign_relation_badge(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign) {
  return runtime_campaign_is_detached(st, campaign) ? " [DETACHED]" : "";
}

inline std::string runtime_job_relation_badge(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  if (runtime_job_is_orphan(st, job))
    return " [ORPHAN]";
  if (runtime_job_is_sessionless(st, job))
    return " [NOSESSION]";
  return {};
}

inline std::string runtime_panel_session_label(const CmdState &st) {
  if (st.runtime.session.has_value()) {
    const auto &session = *st.runtime.session;
    const std::string label = session.objective_name.empty()
                                  ? session.marshal_session_id
                                  : session.objective_name;
    return trim_to_width(label, 18);
  }
  if (!st.runtime.selected_session_id.empty()) {
    return trim_to_width(
        runtime_is_none_session_selection(st.runtime.selected_session_id)
            ? runtime_none_branch_label()
            : st.runtime.selected_session_id,
        18);
  }
  return "<marshal>";
}

inline std::string runtime_panel_campaign_label(const CmdState &st) {
  if (st.runtime.campaign.has_value()) {
    return trim_to_width(st.runtime.campaign->campaign_cursor, 18);
  }
  if (!st.runtime.selected_campaign_cursor.empty()) {
    return trim_to_width(
        runtime_is_none_campaign_selection(st.runtime.selected_campaign_cursor)
            ? runtime_none_branch_label()
            : st.runtime.selected_campaign_cursor,
        18);
  }
  return "<campaign>";
}

inline std::string runtime_worklist_panel_title(const CmdState &st) {
  std::string title = "worklist";
  if (runtime_is_device_lane(st.runtime.lane)) {
    return title + " · devices";
  }
  if (runtime_is_session_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active &&
        !st.runtime.selected_campaign_cursor.empty()) {
      title += " · jobs";
      title += " · marshal=" + runtime_panel_session_label(st);
      title += " · campaign=" + runtime_panel_campaign_label(st);
      return title;
    }
    if (st.runtime.campaign_filter_active &&
        !st.runtime.selected_session_id.empty()) {
      return title +
             " · campaigns · marshal=" + runtime_panel_session_label(st);
    }
    return title + " · marshals";
  }
  if (runtime_is_campaign_lane(st.runtime.lane)) {
    if (st.runtime.job_filter_active &&
        !st.runtime.selected_campaign_cursor.empty()) {
      title += " · jobs";
      if (!st.runtime.selected_session_id.empty()) {
        title += " · marshal=" + runtime_panel_session_label(st);
      }
      title += " · campaign=" + runtime_panel_campaign_label(st);
      return title;
    }
    return title + " · campaigns";
  }
  return title + " · jobs";
}

inline cuwacunu::iinuji::text_line_emphasis_t runtime_session_row_emphasis(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (session.finish_reason == "failed") {
    return cuwacunu::iinuji::text_line_emphasis_t::MutedError;
  }
  if (session.finish_reason == "terminated") {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  if (session.finish_reason == "interrupted") {
    if (!runtime_session_is_active(session) &&
        !operator_session_has_pending_review(st, session.marshal_session_id)) {
      return cuwacunu::iinuji::text_line_emphasis_t::Info;
    }
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (session.lifecycle == "live" && session.work_gate != "open") {
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (runtime_session_is_active(session)) {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Info;
}

inline cuwacunu::iinuji::text_line_emphasis_t runtime_campaign_row_emphasis(
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign) {
  const std::string state = to_lower_copy(campaign.state);
  if (state.find("fail") != std::string::npos ||
      state.find("error") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::MutedError;
  }
  if (state == "stopping") {
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (runtime_campaign_is_active(campaign)) {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Info;
}

inline cuwacunu::iinuji::text_line_emphasis_t runtime_job_row_emphasis(
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  const std::string state = to_lower_copy(job.state);
  if (state == "orphaned") {
    return cuwacunu::iinuji::text_line_emphasis_t::MutedWarning;
  }
  if (state.find("fail") != std::string::npos ||
      state.find("error") != std::string::npos ||
      (job.exit_code.has_value() && *job.exit_code != 0)) {
    return cuwacunu::iinuji::text_line_emphasis_t::MutedError;
  }
  if (runtime_job_is_active(job)) {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Info;
}

inline cuwacunu::iinuji::text_line_emphasis_t runtime_session_effort_emphasis(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (session.max_campaign_launches != 0 &&
      session.remaining_campaign_launches == 0) {
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (session.launch_count != 0 || session.checkpoint_count != 0) {
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Debug;
}

inline cuwacunu::iinuji::text_line_emphasis_t
runtime_row_emphasis(cuwacunu::iinuji::text_line_emphasis_t base, bool active,
                     bool focused) {
  if (active) {
    (void)base;
    return runtime_selected_emphasis(focused);
  }
  return base;
}

inline std::string runtime_session_worklist_title(
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    cuwacunu::iinuji::text_line_emphasis_t status_emphasis) {
  return "M " + runtime_session_scope_badge(session, status_emphasis) + " " +
         trim_to_width(session.objective_name.empty()
                           ? std::string("<unnamed objective>")
                           : session.objective_name,
                       24);
}

inline std::string runtime_session_worklist_row(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    cuwacunu::iinuji::text_line_emphasis_t status_emphasis) {
  std::ostringstream oss;
  oss << runtime_session_worklist_title(session, status_emphasis) << " | "
      << trim_to_width(session.marshal_session_id, 24) << " | "
      << format_age_since_ms_minutes(session.updated_at_ms);
  if (cuwacunu::hero::marshal::is_marshal_session_summary_state(session)) {
    oss << " | "
        << (operator_session_has_pending_review(st, session.marshal_session_id)
                ? "review"
                : "ack");
  }
  return trim_to_width(oss.str(), 74);
}

inline std::string runtime_campaign_worklist_title(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign,
    cuwacunu::iinuji::text_line_emphasis_t status_emphasis) {
  return "C " +
         runtime_scope_badge(runtime_campaign_is_active(campaign),
                             status_emphasis) +
         " [" + trim_to_width(campaign.state, 8) + "]" +
         runtime_campaign_relation_badge(st, campaign) + " " +
         trim_to_width(campaign.campaign_cursor, 24);
}

inline std::string runtime_campaign_worklist_row(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign,
    cuwacunu::iinuji::text_line_emphasis_t status_emphasis) {
  std::ostringstream oss;
  oss << runtime_campaign_worklist_title(st, campaign, status_emphasis) << " | "
      << format_age_since_ms_minutes(campaign.updated_at_ms);
  return trim_to_width(oss.str(), 74);
}

inline std::string runtime_job_worklist_title(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_job_record_t &job,
    cuwacunu::iinuji::text_line_emphasis_t status_emphasis) {
  return "J " +
         runtime_scope_badge(runtime_job_is_active(job), status_emphasis) +
         " [" + trim_to_width(job.state, 8) + "]" +
         runtime_job_relation_badge(st, job) + " " +
         trim_to_width(job.job_cursor, 24);
}

inline std::string runtime_job_worklist_row(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_job_record_t &job,
    cuwacunu::iinuji::text_line_emphasis_t status_emphasis) {
  std::ostringstream oss;
  oss << runtime_job_worklist_title(st, job, status_emphasis) << " | "
      << format_age_since_ms_minutes(job.updated_at_ms);
  if (job.exit_code.has_value()) {
    oss << " | exit=" << *job.exit_code;
  }
  return trim_to_width(oss.str(), 74);
}

inline std::string format_age_since_ms_minutes(std::uint64_t timestamp_ms) {
  if (timestamp_ms == 0)
    return "<unknown>";
  const std::uint64_t now_ms = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  if (timestamp_ms >= now_ms)
    return "0m ago";
  const std::uint64_t total_minutes =
      (now_ms - timestamp_ms) / static_cast<std::uint64_t>(1000 * 60);
  if (total_minutes == 0)
    return "0m ago";
  const std::uint64_t minutes = total_minutes % 60;
  const std::uint64_t hours = (total_minutes / 60) % 24;
  const std::uint64_t days = total_minutes / (24 * 60);
  std::ostringstream oss;
  if (days > 0)
    oss << days << "d";
  if (hours > 0)
    oss << hours << "h";
  if (minutes > 0 || (days == 0 && hours == 0)) {
    oss << minutes << "m";
  }
  return oss.str() + " ago";
}

inline void push_runtime_status_line(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, std::string text,
    const std::string &status_badge,
    cuwacunu::iinuji::text_line_emphasis_t fallback_emphasis,
    cuwacunu::iinuji::text_line_emphasis_t status_emphasis) {
  if (status_badge.empty()) {
    push_runtime_line(out, std::move(text), fallback_emphasis);
    return;
  }
  const auto status_pos = text.find(status_badge);
  if (status_pos == std::string::npos) {
    push_runtime_line(out, std::move(text), fallback_emphasis);
    return;
  }
  const auto status_color = runtime_scope_status_emphasis(status_emphasis);
  std::vector<cuwacunu::iinuji::styled_text_segment_t> segments{};
  if (status_pos != 0) {
    segments.push_back(cuwacunu::iinuji::styled_text_segment_t{
        .text = text.substr(0, status_pos), .emphasis = fallback_emphasis});
  }
  segments.push_back(cuwacunu::iinuji::styled_text_segment_t{
      .text = status_badge, .emphasis = status_color});
  const std::size_t status_end = status_pos + status_badge.size();
  if (status_end < text.size()) {
    segments.push_back(cuwacunu::iinuji::styled_text_segment_t{
        .text = text.substr(status_end), .emphasis = fallback_emphasis});
  }
  push_runtime_segments(out, std::move(segments), fallback_emphasis);
}

inline std::string runtime_none_session_worklist_row(const CmdState &st) {
  const std::size_t orphan_campaigns =
      runtime_orphan_campaign_indices(st).size();
  const std::size_t orphan_jobs = runtime_orphan_job_indices(st).size();
  std::ostringstream oss;
  oss << "M [ORPHAN] " << runtime_none_branch_label();
  if (orphan_campaigns != 0) {
    oss << " | campaigns=" << orphan_campaigns;
  }
  if (orphan_jobs != 0) {
    oss << " | jobs=" << orphan_jobs;
  }
  return trim_to_width(oss.str(), 74);
}

inline std::string runtime_none_campaign_worklist_row(const CmdState &st) {
  const std::size_t orphan_jobs = runtime_orphan_job_indices(st).size();
  std::ostringstream oss;
  oss << "C [ORPHAN] " << runtime_none_branch_label();
  if (orphan_jobs != 0) {
    oss << " | jobs=" << orphan_jobs;
  }
  return trim_to_width(oss.str(), 74);
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_navigation_styled_lines(const CmdState &st) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out{};
  push_runtime_line(
      out, runtime_menu_row_text(st, kRuntimeDeviceLane),
      st.runtime.lane == kRuntimeDeviceLane
          ? runtime_selected_emphasis(runtime_is_menu_focus(st.runtime.focus))
          : cuwacunu::iinuji::text_line_emphasis_t::None);
  push_runtime_line(
      out, runtime_menu_row_text(st, kRuntimeSessionLane),
      st.runtime.lane == kRuntimeSessionLane
          ? runtime_selected_emphasis(runtime_is_menu_focus(st.runtime.focus))
          : cuwacunu::iinuji::text_line_emphasis_t::None);
  push_runtime_line(
      out, runtime_menu_row_text(st, kRuntimeCampaignLane),
      st.runtime.lane == kRuntimeCampaignLane
          ? runtime_selected_emphasis(runtime_is_menu_focus(st.runtime.focus))
          : cuwacunu::iinuji::text_line_emphasis_t::None);
  push_runtime_line(
      out, runtime_menu_row_text(st, kRuntimeJobLane),
      st.runtime.lane == kRuntimeJobLane
          ? runtime_selected_emphasis(runtime_is_menu_focus(st.runtime.focus))
          : cuwacunu::iinuji::text_line_emphasis_t::None);
  push_runtime_line(out, {});
  push_runtime_line(out,
                    "anchor=" +
                        (st.runtime.anchor_session_id.empty()
                             ? std::string("<none>")
                             : st.runtime.anchor_session_id) +
                        " | focus=" + runtime_focus_label(st.runtime.focus),
                    cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_runtime_line(
      out,
      runtime_is_menu_focus(st.runtime.focus)
          ? "Enter opens the lower worklist | Tab detail mode"
          : "Left/Right enter or leave nested rows | Enter/a actions",
      cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_runtime_line(out, "Up/Down browse | Esc returns to navigator",
                    cuwacunu::iinuji::text_line_emphasis_t::Debug);
  return out;
}

inline RuntimeRowKind runtime_primary_kind(const CmdState &st) {
  if (runtime_is_device_lane(st.runtime.lane)) {
    return RuntimeRowKind::Device;
  }
  if (runtime_is_session_lane(st.runtime.lane) &&
      st.runtime.job_filter_active &&
      (!st.runtime.selected_job_cursor.empty() || st.runtime.job.has_value())) {
    return RuntimeRowKind::Job;
  }
  if (runtime_is_session_lane(st.runtime.lane) &&
      st.runtime.campaign_filter_active &&
      (!st.runtime.selected_campaign_cursor.empty() ||
       st.runtime.campaign.has_value())) {
    return RuntimeRowKind::Campaign;
  }
  if (runtime_is_campaign_lane(st.runtime.lane) &&
      st.runtime.job_filter_active &&
      (!st.runtime.selected_job_cursor.empty() || st.runtime.job.has_value())) {
    return RuntimeRowKind::Job;
  }
  return st.runtime.lane;
}

inline std::string runtime_ascii_progress_bar(int pct, std::size_t width = 14) {
  const int clamped = std::clamp(pct, 0, 100);
  const std::size_t filled = static_cast<std::size_t>(
      (static_cast<long long>(clamped) * static_cast<long long>(width) + 50) /
      100);
  std::string out{"["};
  for (std::size_t i = 0; i < width; ++i) {
    out += (i < filled) ? "█" : "░";
  }
  out += "]";
  return out;
}

inline std::string runtime_compact_progress_bar(int pct,
                                                std::size_t width = 4) {
  const int clamped = std::clamp(pct, 0, 100);
  const std::size_t filled = static_cast<std::size_t>(
      (static_cast<long long>(clamped) * static_cast<long long>(width) + 50) /
      100);
  std::string out{};
  out.reserve(width);
  for (std::size_t i = 0; i < width; ++i) {
    out += (i < filled) ? "█" : "░";
  }
  return out;
}

inline int runtime_host_load_percent(const RuntimeDeviceSnapshot &device,
                                     double load_value) {
  if (device.cpu_count == 0)
    return 0;
  const double normalized =
      (load_value / static_cast<double>(device.cpu_count)) * 100.0;
  return std::clamp(static_cast<int>(normalized + 0.5), 0, 100);
}

inline int runtime_host_cpu_percent(const RuntimeDeviceSnapshot &device) {
  if (device.cpu_usage_pct >= 0)
    return std::clamp(device.cpu_usage_pct, 0, 100);
  return runtime_host_load_percent(device, device.load1);
}

inline int runtime_host_memory_percent(const RuntimeDeviceSnapshot &device) {
  if (device.mem_total_kib == 0)
    return 0;
  const std::uint64_t used_kib =
      device.mem_total_kib > device.mem_available_kib
          ? device.mem_total_kib - device.mem_available_kib
          : 0;
  return std::clamp(
      static_cast<int>((100.0 * static_cast<double>(used_kib)) /
                           static_cast<double>(device.mem_total_kib) +
                       0.5),
      0, 100);
}

inline int runtime_gpu_memory_percent(const RuntimeDeviceGpuSnapshot &gpu) {
  if (gpu.memory_total_mib == 0)
    return std::max(0, gpu.util_mem_pct);
  return std::clamp(
      static_cast<int>((100.0 * static_cast<double>(gpu.memory_used_mib)) /
                           static_cast<double>(gpu.memory_total_mib) +
                       0.5),
      0, 100);
}

inline std::uint64_t
runtime_device_snapshot_age_ms(const RuntimeDeviceSnapshot &device) {
  if (device.collected_at_ms == 0)
    return 0;
  const std::uint64_t now_ms = runtime_wall_now_ms();
  return now_ms >= device.collected_at_ms ? now_ms - device.collected_at_ms : 0;
}

inline cuwacunu::iinuji::text_line_emphasis_t
runtime_device_age_emphasis(const RuntimeDeviceSnapshot &device) {
  const std::uint64_t age_ms = runtime_device_snapshot_age_ms(device);
  if (device.collected_at_ms == 0) {
    return cuwacunu::iinuji::text_line_emphasis_t::Debug;
  }
  if (age_ms >= 8000)
    return cuwacunu::iinuji::text_line_emphasis_t::MutedError;
  if (age_ms >= 3500)
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  if (age_ms >= 1500)
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  return cuwacunu::iinuji::text_line_emphasis_t::Success;
}

inline cuwacunu::iinuji::text_line_emphasis_t
runtime_percent_emphasis(int pct) {
  if (pct >= 90)
    return cuwacunu::iinuji::text_line_emphasis_t::MutedError;
  if (pct >= 70)
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  if (pct >= 35)
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  return cuwacunu::iinuji::text_line_emphasis_t::Success;
}

inline std::string runtime_device_host_worklist_label() { return "HOST"; }

inline std::string
runtime_device_gpu_worklist_label(const RuntimeDeviceGpuSnapshot &gpu) {
  return "GPU" + (gpu.index.empty() ? std::string("?") : gpu.index);
}

inline std::size_t runtime_device_worklist_index_width(const CmdState &st) {
  std::size_t total_rows = 1 + st.runtime.device.gpus.size();
  std::size_t width = 1;
  while (total_rows >= 10) {
    total_rows /= 10;
    ++width;
  }
  return width;
}

inline std::string runtime_device_worklist_index_text(std::size_t ordinal,
                                                      std::size_t width) {
  std::ostringstream oss;
  oss << "[" << std::setw(static_cast<int>(width)) << ordinal << "] ";
  return oss.str();
}

inline constexpr std::size_t kRuntimeDeviceWorklistMetricBarWidth = 8;

inline std::string runtime_device_worklist_metric_text(std::string_view key,
                                                       int pct) {
  const int clamped_pct = std::clamp(pct, 0, 100);
  return std::string(key) + " [" +
         runtime_compact_progress_bar(clamped_pct,
                                      kRuntimeDeviceWorklistMetricBarWidth) +
         "] " + std::to_string(clamped_pct) + "%";
}

inline cuwacunu::iinuji::styled_text_line_t
runtime_device_host_worklist_row(const CmdState &st, std::size_t ordinal,
                                 std::size_t index_width, bool active,
                                 bool focused) {
  const auto &device = st.runtime.device;
  const auto text_emphasis = active
                                 ? runtime_selected_emphasis(focused)
                                 : cuwacunu::iinuji::text_line_emphasis_t::Info;
  std::string row = std::string(active ? "> " : "  ") +
                    runtime_device_worklist_index_text(ordinal, index_width) +
                    runtime_device_host_worklist_label() + " " +
                    runtime_device_worklist_metric_text(
                        "C", runtime_host_cpu_percent(device));
  if (device.mem_total_kib != 0) {
    row += " " + runtime_device_worklist_metric_text(
                     "M", runtime_host_memory_percent(device));
  }
  return cuwacunu::iinuji::styled_text_line_t{
      .text = std::move(row),
      .emphasis = text_emphasis,
  };
}

inline cuwacunu::iinuji::styled_text_line_t
runtime_device_gpu_worklist_row(const RuntimeDeviceGpuSnapshot &gpu,
                                std::size_t ordinal, std::size_t index_width,
                                bool active, bool focused) {
  const auto text_emphasis = active
                                 ? runtime_selected_emphasis(focused)
                                 : cuwacunu::iinuji::text_line_emphasis_t::Info;
  std::string row =
      std::string(active ? "> " : "  ") +
      runtime_device_worklist_index_text(ordinal, index_width) +
      runtime_device_gpu_worklist_label(gpu) + " " +
      runtime_device_worklist_metric_text("C", std::max(0, gpu.util_gpu_pct)) +
      " " +
      runtime_device_worklist_metric_text("M", runtime_gpu_memory_percent(gpu));
  return cuwacunu::iinuji::styled_text_line_t{
      .text = std::move(row),
      .emphasis = text_emphasis,
  };
}

inline RuntimeWorklistPanel make_runtime_worklist_panel(const CmdState &st) {
  RuntimeWorklistPanel panel{};
  auto &out = panel.lines;
  const bool focused = runtime_is_worklist_focus(st.runtime.focus);

  if (runtime_is_device_lane(st.runtime.lane)) {
    const std::size_t selected = runtime_selected_device_index(st).value_or(0);
    const std::size_t index_width = runtime_device_worklist_index_width(st);
    const bool host_active = (selected == 0);
    if (host_active)
      panel.selected_line = out.size();
    out.push_back(runtime_device_host_worklist_row(st, 1, index_width,
                                                   host_active, focused));
    for (std::size_t i = 0; i < st.runtime.device.gpus.size(); ++i) {
      const auto &gpu = st.runtime.device.gpus[i];
      const bool active = (selected == i + 1);
      if (active)
        panel.selected_line = out.size();
      out.push_back(runtime_device_gpu_worklist_row(gpu, i + 2, index_width,
                                                    active, focused));
    }
    return panel;
  }

  if (runtime_is_session_lane(st.runtime.lane)) {
    if (st.runtime.sessions.empty() && !runtime_has_none_session_row(st)) {
      push_runtime_line(out, "No runtime marshals.",
                        cuwacunu::iinuji::text_line_emphasis_t::Info);
      return panel;
    }
    const std::size_t selected = runtime_selected_session_index(st).value_or(0);
    const auto child_campaigns = runtime_child_campaign_indices(st);
    const auto child_jobs = runtime_child_job_indices(st);
    const bool show_none_campaign_child =
        runtime_child_has_none_campaign_row(st);
    const std::size_t selected_campaign =
        runtime_selected_child_campaign_index(st).value_or(0);
    const std::size_t selected_job =
        runtime_selected_child_job_index(st).value_or(0);
    for (std::size_t i = 0; i < st.runtime.sessions.size(); ++i) {
      const auto &session = st.runtime.sessions[i];
      const bool active = (i == selected) && !st.runtime.campaign_filter_active;
      const auto session_status_emphasis =
          runtime_session_row_emphasis(st, session);
      const auto emphasis = runtime_row_emphasis(
          cuwacunu::iinuji::text_line_emphasis_t::Info, active, focused);
      const std::string prefix = (i == selected && !child_campaigns.empty())
                                     ? "v "
                                     : (active ? "> " : "  ");
      if (active)
        panel.selected_line = out.size();
      push_runtime_status_line(
          out,
          prefix + "[" + std::to_string(i + 1) + "] " +
              runtime_session_worklist_row(st, session,
                                           session_status_emphasis),
          runtime_scope_badge(runtime_session_is_active(session),
                              session_status_emphasis),
          emphasis, session_status_emphasis);
      if (i != selected)
        continue;
      for (std::size_t j = 0; j < child_campaigns.size(); ++j) {
        const auto &campaign = st.runtime.campaigns[child_campaigns[j]];
        const bool child_active = st.runtime.campaign_filter_active &&
                                  !st.runtime.job_filter_active &&
                                  (j == selected_campaign);
        const auto campaign_status_emphasis =
            runtime_campaign_row_emphasis(campaign);
        const auto child_emphasis =
            runtime_row_emphasis(cuwacunu::iinuji::text_line_emphasis_t::Info,
                                 child_active, focused);
        const bool show_jobs = st.runtime.campaign_filter_active &&
                               (j == selected_campaign) && !child_jobs.empty();
        const std::string child_prefix =
            show_jobs ? "    v " : (child_active ? "    > " : "      ");
        if (child_active)
          panel.selected_line = out.size();
        push_runtime_status_line(
            out,
            child_prefix + "[" + std::to_string(j + 1) + "] " +
                runtime_campaign_worklist_row(st, campaign,
                                              campaign_status_emphasis),
            runtime_scope_badge(runtime_campaign_is_active(campaign),
                                campaign_status_emphasis),
            child_emphasis, campaign_status_emphasis);
        if (!show_jobs)
          continue;
        for (std::size_t k = 0; k < child_jobs.size(); ++k) {
          const auto &job = st.runtime.jobs[child_jobs[k]];
          const bool job_active =
              st.runtime.job_filter_active && (k == selected_job);
          const auto job_status_emphasis = runtime_job_row_emphasis(job);
          const auto job_emphasis =
              runtime_row_emphasis(cuwacunu::iinuji::text_line_emphasis_t::Info,
                                   job_active, focused);
          if (job_active)
            panel.selected_line = out.size();
          push_runtime_status_line(
              out,
              std::string(job_active ? "        > " : "          ") + "[" +
                  std::to_string(k + 1) + "] " +
                  runtime_job_worklist_row(st, job, job_status_emphasis),
              runtime_scope_badge(runtime_job_is_active(job),
                                  job_status_emphasis),
              job_emphasis, job_status_emphasis);
        }
      }
    }
    if (runtime_has_none_session_row(st)) {
      const std::size_t none_index = st.runtime.sessions.size();
      const bool active =
          (none_index == selected) && !st.runtime.campaign_filter_active;
      const auto emphasis = runtime_row_emphasis(
          cuwacunu::iinuji::text_line_emphasis_t::Info, active, focused);
      const bool expanded =
          none_index == selected &&
          (!child_campaigns.empty() || show_none_campaign_child);
      const std::string prefix = expanded ? "v " : (active ? "> " : "  ");
      if (active)
        panel.selected_line = out.size();
      push_runtime_status_line(
          out,
          prefix + "[" + std::to_string(none_index + 1) + "] " +
              runtime_none_session_worklist_row(st),
          "[ORPHAN]", emphasis,
          cuwacunu::iinuji::text_line_emphasis_t::MutedWarning);
      if (none_index == selected) {
        for (std::size_t j = 0; j < child_campaigns.size(); ++j) {
          const auto &campaign = st.runtime.campaigns[child_campaigns[j]];
          const bool child_active = st.runtime.campaign_filter_active &&
                                    !st.runtime.job_filter_active &&
                                    (j == selected_campaign);
          const auto campaign_status_emphasis =
              runtime_campaign_row_emphasis(campaign);
          const auto child_emphasis =
              runtime_row_emphasis(cuwacunu::iinuji::text_line_emphasis_t::Info,
                                   child_active, focused);
          const bool show_jobs = st.runtime.campaign_filter_active &&
                                 (j == selected_campaign) &&
                                 !child_jobs.empty();
          const std::string child_prefix =
              show_jobs ? "    v " : (child_active ? "    > " : "      ");
          if (child_active)
            panel.selected_line = out.size();
          push_runtime_status_line(
              out,
              child_prefix + "[" + std::to_string(j + 1) + "] " +
                  runtime_campaign_worklist_row(st, campaign,
                                                campaign_status_emphasis),
              runtime_scope_badge(runtime_campaign_is_active(campaign),
                                  campaign_status_emphasis),
              child_emphasis, campaign_status_emphasis);
          if (!show_jobs)
            continue;
          for (std::size_t k = 0; k < child_jobs.size(); ++k) {
            const auto &job = st.runtime.jobs[child_jobs[k]];
            const bool job_active =
                st.runtime.job_filter_active && (k == selected_job);
            const auto job_status_emphasis = runtime_job_row_emphasis(job);
            const auto job_emphasis = runtime_row_emphasis(
                cuwacunu::iinuji::text_line_emphasis_t::Info, job_active,
                focused);
            if (job_active)
              panel.selected_line = out.size();
            push_runtime_status_line(
                out,
                std::string(job_active ? "        > " : "          ") + "[" +
                    std::to_string(k + 1) + "] " +
                    runtime_job_worklist_row(st, job, job_status_emphasis),
                runtime_scope_badge(runtime_job_is_active(job),
                                    job_status_emphasis),
                job_emphasis, job_status_emphasis);
          }
        }
        if (show_none_campaign_child) {
          const std::size_t none_child_index = child_campaigns.size();
          const bool child_active = st.runtime.campaign_filter_active &&
                                    !st.runtime.job_filter_active &&
                                    (none_child_index == selected_campaign);
          const auto child_emphasis =
              runtime_row_emphasis(cuwacunu::iinuji::text_line_emphasis_t::Info,
                                   child_active, focused);
          const bool show_jobs = st.runtime.campaign_filter_active &&
                                 (none_child_index == selected_campaign) &&
                                 !child_jobs.empty();
          const std::string child_prefix =
              show_jobs ? "    v " : (child_active ? "    > " : "      ");
          if (child_active)
            panel.selected_line = out.size();
          push_runtime_status_line(
              out,
              child_prefix + "[" + std::to_string(none_child_index + 1) + "] " +
                  runtime_none_campaign_worklist_row(st),
              "[ORPHAN]", child_emphasis,
              cuwacunu::iinuji::text_line_emphasis_t::MutedWarning);
          if (show_jobs) {
            for (std::size_t k = 0; k < child_jobs.size(); ++k) {
              const auto &job = st.runtime.jobs[child_jobs[k]];
              const bool job_active =
                  st.runtime.job_filter_active && (k == selected_job);
              const auto job_status_emphasis = runtime_job_row_emphasis(job);
              const auto job_emphasis = runtime_row_emphasis(
                  cuwacunu::iinuji::text_line_emphasis_t::Info, job_active,
                  focused);
              if (job_active)
                panel.selected_line = out.size();
              push_runtime_status_line(
                  out,
                  std::string(job_active ? "        > " : "          ") + "[" +
                      std::to_string(k + 1) + "] " +
                      runtime_job_worklist_row(st, job, job_status_emphasis),
                  runtime_scope_badge(runtime_job_is_active(job),
                                      job_status_emphasis),
                  job_emphasis, job_status_emphasis);
            }
          }
        }
      }
    }
    return panel;
  }

  if (runtime_is_campaign_lane(st.runtime.lane)) {
    if (st.runtime.campaigns.empty() && !runtime_has_none_campaign_row(st)) {
      push_runtime_line(out, "No runtime campaigns.",
                        cuwacunu::iinuji::text_line_emphasis_t::Info);
      return panel;
    }
    const std::size_t selected =
        runtime_selected_campaign_index(st).value_or(0);
    const auto child = runtime_child_job_indices(st);
    const std::size_t child_selected =
        runtime_selected_child_job_index(st).value_or(0);
    for (std::size_t i = 0; i < st.runtime.campaigns.size(); ++i) {
      const auto &campaign = st.runtime.campaigns[i];
      const bool active = (i == selected) && !st.runtime.job_filter_active;
      const auto campaign_status_emphasis =
          runtime_campaign_row_emphasis(campaign);
      const auto emphasis = runtime_row_emphasis(
          cuwacunu::iinuji::text_line_emphasis_t::Info, active, focused);
      const std::string prefix =
          (i == selected && !child.empty()) ? "v " : (active ? "> " : "  ");
      if (active)
        panel.selected_line = out.size();
      push_runtime_status_line(
          out,
          prefix + "[" + std::to_string(i + 1) + "] " +
              runtime_campaign_worklist_row(st, campaign,
                                            campaign_status_emphasis),
          runtime_scope_badge(runtime_campaign_is_active(campaign),
                              campaign_status_emphasis),
          emphasis, campaign_status_emphasis);
      if (i != selected)
        continue;
      for (std::size_t j = 0; j < child.size(); ++j) {
        const auto &job = st.runtime.jobs[child[j]];
        const bool child_active =
            st.runtime.job_filter_active && (j == child_selected);
        const auto job_status_emphasis = runtime_job_row_emphasis(job);
        const auto child_emphasis =
            runtime_row_emphasis(cuwacunu::iinuji::text_line_emphasis_t::Info,
                                 child_active, focused);
        if (child_active)
          panel.selected_line = out.size();
        push_runtime_status_line(
            out,
            std::string(child_active ? "    > " : "      ") + "[" +
                std::to_string(j + 1) + "] " +
                runtime_job_worklist_row(st, job, job_status_emphasis),
            runtime_scope_badge(runtime_job_is_active(job),
                                job_status_emphasis),
            child_emphasis, job_status_emphasis);
      }
    }
    if (runtime_has_none_campaign_row(st)) {
      const std::size_t none_index = st.runtime.campaigns.size();
      const bool active =
          (none_index == selected) && !st.runtime.job_filter_active;
      const auto emphasis = runtime_row_emphasis(
          cuwacunu::iinuji::text_line_emphasis_t::Info, active, focused);
      const bool expanded = (none_index == selected) && !child.empty();
      const std::string prefix = expanded ? "v " : (active ? "> " : "  ");
      if (active)
        panel.selected_line = out.size();
      push_runtime_status_line(
          out,
          prefix + "[" + std::to_string(none_index + 1) + "] " +
              runtime_none_campaign_worklist_row(st),
          "[ORPHAN]", emphasis,
          cuwacunu::iinuji::text_line_emphasis_t::MutedWarning);
      if (none_index == selected) {
        for (std::size_t j = 0; j < child.size(); ++j) {
          const auto &job = st.runtime.jobs[child[j]];
          const bool child_active =
              st.runtime.job_filter_active && (j == child_selected);
          const auto job_status_emphasis = runtime_job_row_emphasis(job);
          const auto child_emphasis =
              runtime_row_emphasis(cuwacunu::iinuji::text_line_emphasis_t::Info,
                                   child_active, focused);
          if (child_active)
            panel.selected_line = out.size();
          push_runtime_status_line(
              out,
              std::string(child_active ? "    > " : "      ") + "[" +
                  std::to_string(j + 1) + "] " +
                  runtime_job_worklist_row(st, job, job_status_emphasis),
              runtime_scope_badge(runtime_job_is_active(job),
                                  job_status_emphasis),
              child_emphasis, job_status_emphasis);
        }
      }
    }
    return panel;
  }

  if (st.runtime.jobs.empty()) {
    push_runtime_line(out, "No runtime jobs.",
                      cuwacunu::iinuji::text_line_emphasis_t::Info);
    return panel;
  }
  const std::size_t selected = runtime_selected_job_index(st).value_or(0);
  for (std::size_t i = 0; i < st.runtime.jobs.size(); ++i) {
    const auto &job = st.runtime.jobs[i];
    const bool active = (i == selected);
    const auto job_status_emphasis = runtime_job_row_emphasis(job);
    const auto emphasis = runtime_row_emphasis(
        cuwacunu::iinuji::text_line_emphasis_t::Info, active, focused);
    if (active)
      panel.selected_line = out.size();
    push_runtime_status_line(
        out,
        std::string(active ? "> " : "  ") + "[" + std::to_string(i + 1) + "] " +
            runtime_job_worklist_row(st, job, job_status_emphasis),
        runtime_scope_badge(runtime_job_is_active(job), job_status_emphasis),
        emphasis, job_status_emphasis);
  }
  return panel;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_worklist_styled_lines(const CmdState &st) {
  return make_runtime_worklist_panel(st).lines;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_left_styled_lines(const CmdState &st) {
  auto out = make_runtime_navigation_styled_lines(st);
  push_runtime_line(out, {});
  const auto worklist = make_runtime_worklist_panel(st).lines;
  out.insert(out.end(), worklist.begin(), worklist.end());
  return out;
}

inline std::string runtime_styled_lines_to_text(
    const std::vector<cuwacunu::iinuji::styled_text_line_t> &lines) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i > 0)
      oss << '\n';
    oss << cuwacunu::iinuji::styled_text_line_text(lines[i]);
  }
  return oss.str();
}

inline std::string make_runtime_left(const CmdState &st) {
  return runtime_styled_lines_to_text(make_runtime_left_styled_lines(st));
}

inline std::string runtime_detail_rule() { return panel_rule('-', 56); }

inline void append_runtime_detail_section(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::string_view title,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Accent) {
  push_runtime_line(out, runtime_detail_rule(), emphasis);
  push_runtime_line(out, std::string(title), emphasis);
  push_runtime_line(out, runtime_detail_rule(), emphasis);
}

inline void append_runtime_detail_text(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, std::string text,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::None,
    std::string_view prefix = {}) {
  if (text.empty())
    text = "<none>";
  std::size_t start = 0;
  while (start <= text.size()) {
    const std::size_t end = text.find('\n', start);
    const std::string_view line =
        end == std::string::npos
            ? std::string_view(text).substr(start)
            : std::string_view(text).substr(start, end - start);
    push_runtime_line(out, std::string(prefix) + std::string(line), emphasis);
    if (end == std::string::npos)
      break;
    start = end + 1;
    if (start == text.size())
      break;
  }
}

inline std::string runtime_detail_meta_prefix(std::string_view key) {
  constexpr std::size_t kRuntimeDetailKeyWidth = 26;
  std::string prefix = std::string(key) + ":";
  if (prefix.size() < kRuntimeDetailKeyWidth) {
    prefix.append(kRuntimeDetailKeyWidth - prefix.size(), ' ');
  } else {
    prefix.push_back(' ');
  }
  return prefix;
}

inline std::vector<std::vector<cuwacunu::iinuji::styled_text_segment_t>>
split_runtime_detail_segments(
    const std::vector<cuwacunu::iinuji::styled_text_segment_t> &segments) {
  std::vector<std::vector<cuwacunu::iinuji::styled_text_segment_t>> lines(1);
  for (const auto &segment : segments) {
    std::size_t start = 0;
    while (start <= segment.text.size()) {
      const std::size_t end = segment.text.find('\n', start);
      const std::string_view part =
          end == std::string::npos
              ? std::string_view(segment.text).substr(start)
              : std::string_view(segment.text).substr(start, end - start);
      if (!part.empty()) {
        lines.back().push_back(cuwacunu::iinuji::styled_text_segment_t{
            .text = std::string(part),
            .emphasis = segment.emphasis,
        });
      }
      if (end == std::string::npos)
        break;
      lines.emplace_back();
      start = end + 1;
      if (start == segment.text.size())
        break;
    }
  }
  return lines;
}

inline void append_runtime_detail_segments(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::string_view key,
    std::vector<cuwacunu::iinuji::styled_text_segment_t> segments,
    cuwacunu::iinuji::text_line_emphasis_t key_emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Info) {
  bool has_visible_text = false;
  for (const auto &segment : segments) {
    if (!segment.text.empty()) {
      has_visible_text = true;
      break;
    }
  }
  if (!has_visible_text) {
    segments = {cuwacunu::iinuji::styled_text_segment_t{
        .text = "<none>",
        .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Debug,
    }};
  }

  const std::string prefix = runtime_detail_meta_prefix(key);
  const std::string continuation(prefix.size(), ' ');
  const auto lines = split_runtime_detail_segments(segments);
  for (std::size_t i = 0; i < lines.size(); ++i) {
    std::vector<cuwacunu::iinuji::styled_text_segment_t> row{};
    row.push_back(cuwacunu::iinuji::styled_text_segment_t{
        .text = (i == 0 ? prefix : continuation),
        .emphasis = key_emphasis,
    });
    row.insert(row.end(), lines[i].begin(), lines[i].end());
    push_runtime_segments(out, std::move(row), key_emphasis);
  }
}

inline void append_runtime_detail_meta(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::string_view key, std::string value,
    cuwacunu::iinuji::text_line_emphasis_t key_emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Info,
    cuwacunu::iinuji::text_line_emphasis_t value_emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Debug) {
  if (value.empty())
    value = "<none>";
  append_runtime_detail_segments(out, key,
                                 {cuwacunu::iinuji::styled_text_segment_t{
                                     .text = std::move(value),
                                     .emphasis = value_emphasis,
                                 }},
                                 key_emphasis);
}

inline void append_runtime_note_block(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, std::string note,
    std::string_view title = "Note",
    cuwacunu::iinuji::text_line_emphasis_t title_emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Info,
    cuwacunu::iinuji::text_line_emphasis_t body_emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::None) {
  if (trim_copy(note).empty())
    return;
  push_runtime_line(out, {});
  append_runtime_detail_section(out, title, title_emphasis);
  append_runtime_detail_text(out, std::move(note), body_emphasis);
}

inline cuwacunu::iinuji::text_line_emphasis_t
runtime_path_emphasis(std::string_view path) {
  return path.empty() ? cuwacunu::iinuji::text_line_emphasis_t::Debug
                      : cuwacunu::iinuji::text_line_emphasis_t::Info;
}

inline void append_runtime_device_updated_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const CmdState &st) {
  const auto &device = st.runtime.device;
  if (device.collected_at_ms == 0) {
    append_runtime_detail_meta(out, "updated", "<unknown>",
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }
  const auto age_emphasis = runtime_device_age_emphasis(device);
  const bool stale = runtime_device_snapshot_age_ms(device) >= 3500;
  append_runtime_detail_segments(
      out, "updated",
      {
          cuwacunu::iinuji::styled_text_segment_t{
              .text = format_time_marker_ms(device.collected_at_ms),
              .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Debug,
          },
          cuwacunu::iinuji::styled_text_segment_t{
              .text =
                  "  " + format_age_since_ms_minutes(device.collected_at_ms),
              .emphasis = age_emphasis,
          },
          cuwacunu::iinuji::styled_text_segment_t{
              .text = st.runtime.device_pending
                          ? "  [live]"
                          : (stale ? "  [stale]" : "  [fresh]"),
              .emphasis = st.runtime.device_pending
                              ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                              : age_emphasis,
          },
      });
}

