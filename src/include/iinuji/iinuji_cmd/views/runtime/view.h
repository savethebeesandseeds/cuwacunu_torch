#pragma once

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
                    format_age_since_ms(campaign.updated_at_ms));
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
                    format_age_since_ms(job.updated_at_ms));
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
        trim_to_width(runtime_campaign_ref_text(campaign_cursor), 22));
    if (lines.size() >= limit)
      break;
  }
  return lines;
}

inline std::string runtime_path_text(std::string_view path) {
  return path.empty() ? std::string("<none>") : std::string(path);
}

inline std::string runtime_excerpt_text(const std::string &text) {
  return text.empty() ? std::string("<unavailable>") : text;
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
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (session.finish_reason == "failed" ||
      session.finish_reason == "terminated") {
    return cuwacunu::iinuji::text_line_emphasis_t::MutedError;
  }
  if (session.phase == "paused") {
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
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
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
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return "M " + runtime_scope_badge(runtime_session_is_active(session)) + " " +
         trim_to_width(session.objective_name.empty()
                           ? std::string("<unnamed objective>")
                           : session.objective_name,
                       24);
}

inline std::string runtime_session_worklist_row(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  (void)st;
  std::ostringstream oss;
  oss << runtime_session_worklist_title(session) << " | "
      << trim_to_width(session.marshal_session_id, 24) << " | "
      << format_age_since_ms(session.updated_at_ms);
  return trim_to_width(oss.str(), 74);
}

inline std::string runtime_campaign_worklist_title(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign) {
  return "C " + runtime_scope_badge(runtime_campaign_is_active(campaign)) +
         " [" + trim_to_width(campaign.state, 8) + "]" +
         runtime_campaign_relation_badge(st, campaign) + " " +
         trim_to_width(campaign.campaign_cursor, 24);
}

inline std::string runtime_campaign_worklist_row(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_campaign_record_t &campaign) {
  std::ostringstream oss;
  oss << runtime_campaign_worklist_title(st, campaign) << " | "
      << format_age_since_ms(campaign.updated_at_ms);
  return trim_to_width(oss.str(), 74);
}

inline std::string runtime_job_worklist_title(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  return "J " + runtime_scope_badge(runtime_job_is_active(job)) + " [" +
         trim_to_width(job.state, 8) + "]" +
         runtime_job_relation_badge(st, job) + " " +
         trim_to_width(job.job_cursor, 24);
}

inline std::string runtime_job_worklist_row(
    const CmdState &st,
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  std::ostringstream oss;
  oss << runtime_job_worklist_title(st, job) << " | "
      << format_age_since_ms(job.updated_at_ms);
  if (job.exit_code.has_value()) {
    oss << " | exit=" << *job.exit_code;
  }
  return trim_to_width(oss.str(), 74);
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
      const auto emphasis = runtime_row_emphasis(
          runtime_session_row_emphasis(session), active, focused);
      const std::string prefix = (i == selected && !child_campaigns.empty())
                                     ? "v "
                                     : (active ? "> " : "  ");
      if (active)
        panel.selected_line = out.size();
      push_runtime_line(out,
                        prefix + "[" + std::to_string(i + 1) + "] " +
                            runtime_session_worklist_row(st, session),
                        emphasis);
      if (i != selected)
        continue;
      for (std::size_t j = 0; j < child_campaigns.size(); ++j) {
        const auto &campaign = st.runtime.campaigns[child_campaigns[j]];
        const bool child_active = st.runtime.campaign_filter_active &&
                                  !st.runtime.job_filter_active &&
                                  (j == selected_campaign);
        const auto child_emphasis = runtime_row_emphasis(
            runtime_campaign_row_emphasis(campaign), child_active, focused);
        const bool show_jobs = st.runtime.campaign_filter_active &&
                               (j == selected_campaign) && !child_jobs.empty();
        const std::string child_prefix =
            show_jobs ? "    v " : (child_active ? "    > " : "      ");
        if (child_active)
          panel.selected_line = out.size();
        push_runtime_line(out,
                          child_prefix + "[" + std::to_string(j + 1) + "] " +
                              runtime_campaign_worklist_row(st, campaign),
                          child_emphasis);
        if (!show_jobs)
          continue;
        for (std::size_t k = 0; k < child_jobs.size(); ++k) {
          const auto &job = st.runtime.jobs[child_jobs[k]];
          const bool job_active =
              st.runtime.job_filter_active && (k == selected_job);
          const auto job_emphasis = runtime_row_emphasis(
              runtime_job_row_emphasis(job), job_active, focused);
          if (job_active)
            panel.selected_line = out.size();
          push_runtime_line(
              out,
              std::string(job_active ? "        > " : "          ") + "[" +
                  std::to_string(k + 1) + "] " +
                  runtime_job_worklist_row(st, job),
              job_emphasis);
        }
      }
    }
    if (runtime_has_none_session_row(st)) {
      const std::size_t none_index = st.runtime.sessions.size();
      const bool active =
          (none_index == selected) && !st.runtime.campaign_filter_active;
      const auto emphasis = runtime_row_emphasis(
          cuwacunu::iinuji::text_line_emphasis_t::Warning, active, focused);
      const bool expanded =
          none_index == selected &&
          (!child_campaigns.empty() || show_none_campaign_child);
      const std::string prefix = expanded ? "v " : (active ? "> " : "  ");
      if (active)
        panel.selected_line = out.size();
      push_runtime_line(out,
                        prefix + "[" + std::to_string(none_index + 1) + "] " +
                            runtime_none_session_worklist_row(st),
                        emphasis);
      if (none_index == selected) {
        for (std::size_t j = 0; j < child_campaigns.size(); ++j) {
          const auto &campaign = st.runtime.campaigns[child_campaigns[j]];
          const bool child_active = st.runtime.campaign_filter_active &&
                                    !st.runtime.job_filter_active &&
                                    (j == selected_campaign);
          const auto child_emphasis = runtime_row_emphasis(
              runtime_campaign_row_emphasis(campaign), child_active, focused);
          const bool show_jobs = st.runtime.campaign_filter_active &&
                                 (j == selected_campaign) &&
                                 !child_jobs.empty();
          const std::string child_prefix =
              show_jobs ? "    v " : (child_active ? "    > " : "      ");
          if (child_active)
            panel.selected_line = out.size();
          push_runtime_line(out,
                            child_prefix + "[" + std::to_string(j + 1) + "] " +
                                runtime_campaign_worklist_row(st, campaign),
                            child_emphasis);
          if (!show_jobs)
            continue;
          for (std::size_t k = 0; k < child_jobs.size(); ++k) {
            const auto &job = st.runtime.jobs[child_jobs[k]];
            const bool job_active =
                st.runtime.job_filter_active && (k == selected_job);
            const auto job_emphasis = runtime_row_emphasis(
                runtime_job_row_emphasis(job), job_active, focused);
            if (job_active)
              panel.selected_line = out.size();
            push_runtime_line(
                out,
                std::string(job_active ? "        > " : "          ") + "[" +
                    std::to_string(k + 1) + "] " +
                    runtime_job_worklist_row(st, job),
                job_emphasis);
          }
        }
        if (show_none_campaign_child) {
          const std::size_t none_child_index = child_campaigns.size();
          const bool child_active = st.runtime.campaign_filter_active &&
                                    !st.runtime.job_filter_active &&
                                    (none_child_index == selected_campaign);
          const auto child_emphasis = runtime_row_emphasis(
              cuwacunu::iinuji::text_line_emphasis_t::Warning, child_active,
              focused);
          const bool show_jobs = st.runtime.campaign_filter_active &&
                                 (none_child_index == selected_campaign) &&
                                 !child_jobs.empty();
          const std::string child_prefix =
              show_jobs ? "    v " : (child_active ? "    > " : "      ");
          if (child_active)
            panel.selected_line = out.size();
          push_runtime_line(out,
                            child_prefix + "[" +
                                std::to_string(none_child_index + 1) + "] " +
                                runtime_none_campaign_worklist_row(st),
                            child_emphasis);
          if (show_jobs) {
            for (std::size_t k = 0; k < child_jobs.size(); ++k) {
              const auto &job = st.runtime.jobs[child_jobs[k]];
              const bool job_active =
                  st.runtime.job_filter_active && (k == selected_job);
              const auto job_emphasis = runtime_row_emphasis(
                  runtime_job_row_emphasis(job), job_active, focused);
              if (job_active)
                panel.selected_line = out.size();
              push_runtime_line(
                  out,
                  std::string(job_active ? "        > " : "          ") + "[" +
                      std::to_string(k + 1) + "] " +
                      runtime_job_worklist_row(st, job),
                  job_emphasis);
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
      const auto emphasis = runtime_row_emphasis(
          runtime_campaign_row_emphasis(campaign), active, focused);
      const std::string prefix =
          (i == selected && !child.empty()) ? "v " : (active ? "> " : "  ");
      if (active)
        panel.selected_line = out.size();
      push_runtime_line(out,
                        prefix + "[" + std::to_string(i + 1) + "] " +
                            runtime_campaign_worklist_row(st, campaign),
                        emphasis);
      if (i != selected)
        continue;
      for (std::size_t j = 0; j < child.size(); ++j) {
        const auto &job = st.runtime.jobs[child[j]];
        const bool child_active =
            st.runtime.job_filter_active && (j == child_selected);
        const auto child_emphasis = runtime_row_emphasis(
            runtime_job_row_emphasis(job), child_active, focused);
        if (child_active)
          panel.selected_line = out.size();
        push_runtime_line(out,
                          std::string(child_active ? "    > " : "      ") +
                              "[" + std::to_string(j + 1) + "] " +
                              runtime_job_worklist_row(st, job),
                          child_emphasis);
      }
    }
    if (runtime_has_none_campaign_row(st)) {
      const std::size_t none_index = st.runtime.campaigns.size();
      const bool active =
          (none_index == selected) && !st.runtime.job_filter_active;
      const auto emphasis = runtime_row_emphasis(
          cuwacunu::iinuji::text_line_emphasis_t::Warning, active, focused);
      const bool expanded = (none_index == selected) && !child.empty();
      const std::string prefix = expanded ? "v " : (active ? "> " : "  ");
      if (active)
        panel.selected_line = out.size();
      push_runtime_line(out,
                        prefix + "[" + std::to_string(none_index + 1) + "] " +
                            runtime_none_campaign_worklist_row(st),
                        emphasis);
      if (none_index == selected) {
        for (std::size_t j = 0; j < child.size(); ++j) {
          const auto &job = st.runtime.jobs[child[j]];
          const bool child_active =
              st.runtime.job_filter_active && (j == child_selected);
          const auto child_emphasis = runtime_row_emphasis(
              runtime_job_row_emphasis(job), child_active, focused);
          if (child_active)
            panel.selected_line = out.size();
          push_runtime_line(out,
                            std::string(child_active ? "    > " : "      ") +
                                "[" + std::to_string(j + 1) + "] " +
                                runtime_job_worklist_row(st, job),
                            child_emphasis);
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
    const auto emphasis =
        runtime_row_emphasis(runtime_job_row_emphasis(job), active, focused);
    if (active)
      panel.selected_line = out.size();
    push_runtime_line(out,
                      std::string(active ? "> " : "  ") + "[" +
                          std::to_string(i + 1) + "] " +
                          runtime_job_worklist_row(st, job),
                      emphasis);
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
              .text = "  " + format_age_since_ms(device.collected_at_ms),
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

struct runtime_pid_job_match_t {
  const cuwacunu::hero::runtime::runtime_job_record_t *job{nullptr};
  std::string role{};
};

inline std::optional<runtime_pid_job_match_t>
runtime_find_job_for_pid(const CmdState &st, std::uint64_t pid) {
  if (pid == 0)
    return std::nullopt;
  if (const auto target_it = st.runtime.target_job_index_by_pid.find(pid);
      target_it != st.runtime.target_job_index_by_pid.end() &&
      target_it->second < st.runtime.jobs.size()) {
    return runtime_pid_job_match_t{.job = &st.runtime.jobs[target_it->second],
                                   .role = "target"};
  }
  if (const auto runner_it = st.runtime.runner_job_index_by_pid.find(pid);
      runner_it != st.runtime.runner_job_index_by_pid.end() &&
      runner_it->second < st.runtime.jobs.size()) {
    return runtime_pid_job_match_t{.job = &st.runtime.jobs[runner_it->second],
                                   .role = "runner"};
  }
  return std::nullopt;
}

inline std::string runtime_job_display_label(
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  return job.binding_id.empty() ? trim_to_width(job.job_cursor, 32)
                                : trim_to_width(job.binding_id, 32);
}

inline bool runtime_selected_none_session(const CmdState &st) {
  return runtime_is_none_session_selection(st.runtime.selected_session_id);
}

inline bool runtime_selected_none_campaign(const CmdState &st) {
  return runtime_is_none_campaign_selection(
      st.runtime.selected_campaign_cursor);
}

inline std::string runtime_selected_device_label(const CmdState &st);

inline std::string runtime_inventory_summary_text(const CmdState &st) {
  std::ostringstream oss;
  oss << count_runtime_active_sessions(st) << "/" << st.runtime.sessions.size()
      << " marshals | " << count_runtime_active_campaigns(st) << "/"
      << st.runtime.campaigns.size() << " campaigns | "
      << count_runtime_active_jobs(st) << "/" << st.runtime.jobs.size()
      << " jobs";
  return oss.str();
}

inline std::string runtime_job_process_text(
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  std::vector<std::string> tokens{};
  if (job.runner_pid.has_value()) {
    tokens.push_back("runner=" + std::to_string(*job.runner_pid));
  }
  if (job.target_pid.has_value()) {
    tokens.push_back("target=" + std::to_string(*job.target_pid));
  }
  if (job.target_pgid.has_value()) {
    tokens.push_back("pgid=" + std::to_string(*job.target_pgid));
  }
  if (job.exit_code.has_value()) {
    tokens.push_back("exit=" + std::to_string(*job.exit_code));
  }
  if (job.term_signal.has_value()) {
    tokens.push_back("signal=" + std::to_string(*job.term_signal));
  }
  if (tokens.empty())
    return "<none>";
  std::ostringstream oss;
  for (std::size_t i = 0; i < tokens.size(); ++i) {
    if (i > 0)
      oss << " | ";
    oss << tokens[i];
  }
  return oss.str();
}

inline std::string runtime_format_binary_kib(std::uint64_t kib) {
  std::ostringstream oss;
  if (kib >= 1024ull * 1024ull) {
    oss << std::fixed << std::setprecision(1)
        << (static_cast<double>(kib) / (1024.0 * 1024.0)) << " GiB";
    return oss.str();
  }
  if (kib >= 1024ull) {
    oss << std::fixed << std::setprecision(1)
        << (static_cast<double>(kib) / 1024.0) << " MiB";
    return oss.str();
  }
  oss << kib << " KiB";
  return oss.str();
}

inline std::string runtime_format_binary_mib(std::uint64_t mib) {
  std::ostringstream oss;
  if (mib >= 1024ull) {
    oss << std::fixed << std::setprecision(1)
        << (static_cast<double>(mib) / 1024.0) << " GiB";
    return oss.str();
  }
  oss << mib << " MiB";
  return oss.str();
}

inline std::string runtime_format_scalar(double value, int precision = 2) {
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(precision) << value;
  return oss.str();
}

inline std::string runtime_format_ratio_percent(std::uint64_t used,
                                                std::uint64_t total) {
  if (total == 0)
    return "?%";
  const auto pct = static_cast<int>(
      (100.0 * static_cast<double>(used)) / static_cast<double>(total) + 0.5);
  return std::to_string(std::clamp(pct, 0, 100)) + "%";
}

inline std::string runtime_selected_device_label(const CmdState &st) {
  const std::size_t selected = runtime_selected_device_index(st).value_or(0);
  if (selected == 0) {
    return "HOST cpu/ram";
  }
  const std::size_t gpu_index = selected - 1;
  if (gpu_index >= st.runtime.device.gpus.size())
    return "<gpu>";
  const auto &gpu = st.runtime.device.gpus[gpu_index];
  return "GPU" + (gpu.index.empty() ? std::string("?") : gpu.index) + " cuda";
}

inline std::size_t
runtime_gpu_compute_process_count(const CmdState &st,
                                  const RuntimeDeviceGpuSnapshot &gpu) {
  return static_cast<std::size_t>(std::count_if(
      st.runtime.device.gpu_processes.begin(),
      st.runtime.device.gpu_processes.end(),
      [&](const auto &proc) { return proc.gpu_uuid == gpu.uuid; }));
}

inline std::string runtime_gpu_label_for_uuid(const CmdState &st,
                                              std::string_view gpu_uuid) {
  for (const auto &gpu : st.runtime.device.gpus) {
    if (gpu.uuid != gpu_uuid)
      continue;
    return "gpu" + (gpu.index.empty() ? std::string("?") : gpu.index);
  }
  return gpu_uuid.empty() ? std::string("gpu?") : std::string(gpu_uuid);
}

inline std::vector<const RuntimeDeviceGpuProcessSnapshot *>
runtime_gpu_process_refs(const CmdState &st,
                         const RuntimeDeviceGpuSnapshot &gpu) {
  std::vector<const RuntimeDeviceGpuProcessSnapshot *> refs{};
  for (const auto &proc : st.runtime.device.gpu_processes) {
    if (proc.gpu_uuid != gpu.uuid)
      continue;
    refs.push_back(&proc);
  }
  return refs;
}

inline std::vector<const RuntimeDeviceGpuProcessSnapshot *>
runtime_gpu_process_refs_for_pid(const CmdState &st, std::uint64_t pid) {
  std::vector<const RuntimeDeviceGpuProcessSnapshot *> refs{};
  if (pid == 0)
    return refs;
  for (const auto &proc : st.runtime.device.gpu_processes) {
    if (proc.pid != pid)
      continue;
    refs.push_back(&proc);
  }
  return refs;
}

inline void append_runtime_device_bar_meta(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::string_view key, int pct, std::string suffix,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Info) {
  const int clamped_pct = std::clamp(pct, 0, 100);
  const auto bar_emphasis =
      emphasis == cuwacunu::iinuji::text_line_emphasis_t::Info
          ? runtime_percent_emphasis(clamped_pct)
          : emphasis;
  append_runtime_detail_segments(
      out, key,
      {
          cuwacunu::iinuji::styled_text_segment_t{
              .text = runtime_ascii_progress_bar(clamped_pct),
              .emphasis = bar_emphasis,
          },
          cuwacunu::iinuji::styled_text_segment_t{
              .text = " " + std::to_string(clamped_pct) + "%",
              .emphasis = bar_emphasis,
          },
          cuwacunu::iinuji::styled_text_segment_t{
              .text = "  " + std::move(suffix),
              .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Debug,
          },
      });
}

inline void append_runtime_selected_host_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const CmdState &st) {
  const auto &device = st.runtime.device;
  if (!device.ok && device.collected_at_ms == 0 && device.error.empty()) {
    append_runtime_detail_text(out, "device snapshot pending...",
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }
  append_runtime_detail_meta(out, "type", "host cpu/ram",
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_runtime_detail_meta(out, "host",
                             device.host_name.empty() ? std::string("<host>")
                                                      : device.host_name,
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Success);
  append_runtime_detail_meta(out, "cpu_count", std::to_string(device.cpu_count),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Accent);
  if (device.cpu_usage_pct >= 0) {
    append_runtime_device_bar_meta(out, "cpu", runtime_host_cpu_percent(device),
                                   "sampled from /proc/stat");
    if (device.cpu_iowait_pct >= 0) {
      append_runtime_device_bar_meta(
          out, "iowait", device.cpu_iowait_pct,
          "share of sampled cpu time spent waiting on io",
          cuwacunu::iinuji::text_line_emphasis_t::Debug);
    }
  } else {
    append_runtime_detail_meta(out, "cpu", "awaiting second sample",
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
  }
  append_runtime_detail_meta(out, "loadavg",
                             runtime_format_scalar(device.load1) + " / " +
                                 runtime_format_scalar(device.load5) + " / " +
                                 runtime_format_scalar(device.load15),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Debug);
  if (device.mem_total_kib != 0) {
    const std::uint64_t used_kib =
        device.mem_total_kib > device.mem_available_kib
            ? device.mem_total_kib - device.mem_available_kib
            : 0;
    append_runtime_device_bar_meta(
        out, "ram", runtime_host_memory_percent(device),
        runtime_format_binary_kib(used_kib) + " / " +
            runtime_format_binary_kib(device.mem_total_kib));
  }
  append_runtime_detail_meta(
      out, "observed_gpus",
      std::to_string(device.gpus.size()) + " queried cuda device" +
          (device.gpus.size() == 1 ? "" : "s"),
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      device.gpus.empty() ? cuwacunu::iinuji::text_line_emphasis_t::Debug
                          : cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_runtime_detail_meta(out, "runtime", runtime_inventory_summary_text(st),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Debug);
  append_runtime_device_updated_detail(out, st);
  if (!device.gpu_error.empty()) {
    append_runtime_detail_meta(out, "gpu_probe", device.gpu_error,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  if (!device.error.empty()) {
    append_runtime_detail_meta(out, "device_error", device.error,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
}

inline void append_runtime_selected_gpu_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    const RuntimeDeviceGpuSnapshot &gpu) {
  const auto proc_refs = runtime_gpu_process_refs(st, gpu);
  append_runtime_detail_meta(
      out, "type",
      "gpu" + (gpu.index.empty() ? std::string("?") : gpu.index) + " cuda",
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_runtime_detail_meta(out, "name",
                             gpu.name.empty() ? std::string("<gpu>") : gpu.name,
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Success);
  append_runtime_device_bar_meta(
      out, "core", std::max(0, gpu.util_gpu_pct),
      "compute proc " +
          std::to_string(runtime_gpu_compute_process_count(st, gpu)));
  const int mem_pct = runtime_gpu_memory_percent(gpu);
  append_runtime_device_bar_meta(
      out, "memory", mem_pct,
      runtime_format_binary_mib(gpu.memory_used_mib) + " / " +
          runtime_format_binary_mib(gpu.memory_total_mib));
  if (gpu.temperature_c >= 0) {
    append_runtime_detail_meta(out, "temperature",
                               std::to_string(gpu.temperature_c) + " °C",
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  if (!gpu.uuid.empty()) {
    append_runtime_detail_meta(out, "uuid", gpu.uuid,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
  }
  append_runtime_detail_meta(
      out, "processes", std::to_string(proc_refs.size()) + " attached",
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      proc_refs.empty() ? cuwacunu::iinuji::text_line_emphasis_t::Debug
                        : cuwacunu::iinuji::text_line_emphasis_t::Accent);
  for (const auto *proc : proc_refs) {
    if (proc == nullptr)
      continue;
    const auto job_match = runtime_find_job_for_pid(st, proc->pid);
    const auto proc_state = runtime_probe_pid_resources(proc->pid);
    std::ostringstream oss;
    oss << "pid " << proc->pid;
    if (job_match.has_value() && job_match->job != nullptr) {
      oss << "  " << job_match->role << " "
          << runtime_job_display_label(*job_match->job);
    } else if (!proc_state.name.empty()) {
      oss << "  " << proc_state.name;
    }
    if (proc_state.rss_kib != 0) {
      oss << "  rss " << runtime_format_binary_kib(proc_state.rss_kib);
    }
    if (proc->used_memory_mib != 0) {
      oss << "  vram " << runtime_format_binary_mib(proc->used_memory_mib);
    }
    append_runtime_detail_meta(
        out, "proc", oss.str(), cuwacunu::iinuji::text_line_emphasis_t::Info,
        job_match.has_value() ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                              : cuwacunu::iinuji::text_line_emphasis_t::Debug);
  }
  append_runtime_device_updated_detail(out, st);
  if (!st.runtime.device.error.empty()) {
    append_runtime_detail_meta(out, "device_error", st.runtime.device.error,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
}

inline void append_runtime_selected_device_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const CmdState &st) {
  const std::size_t selected = runtime_selected_device_index(st).value_or(0);
  if (selected == 0) {
    append_runtime_selected_host_detail(out, st);
    return;
  }
  const std::size_t gpu_index = selected - 1;
  if (gpu_index >= st.runtime.device.gpus.size()) {
    append_runtime_detail_text(out, "unavailable",
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }
  append_runtime_selected_gpu_detail(out, st,
                                     st.runtime.device.gpus[gpu_index]);
}

inline void append_runtime_none_session_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const CmdState &st) {
  append_runtime_detail_meta(out, "state", "orphan branch",
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_runtime_detail_meta(out, "marshal_session_id",
                             runtime_none_branch_label(),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_runtime_detail_meta(
      out, "campaigns",
      std::to_string(runtime_orphan_campaign_indices(st).size()),
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_runtime_detail_meta(
      out, "jobs", std::to_string(runtime_orphan_job_indices(st).size()),
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  if (st.runtime.campaign.has_value()) {
    append_runtime_detail_meta(
        out, "selected_campaign",
        runtime_campaign_ref_text(st.runtime.campaign->campaign_cursor),
        cuwacunu::iinuji::text_line_emphasis_t::Info,
        runtime_campaign_row_emphasis(*st.runtime.campaign));
  } else if (runtime_selected_none_campaign(st)) {
    append_runtime_detail_meta(out, "selected_campaign",
                               runtime_none_branch_label(),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  if (st.runtime.job.has_value()) {
    append_runtime_detail_meta(out, "selected_job",
                               runtime_job_display_label(*st.runtime.job),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               runtime_job_row_emphasis(*st.runtime.job));
  }
  append_runtime_note_block(
      out,
      "This synthetic marshal branch groups campaigns without a linked "
      "marshal and jobs whose lineage does not resolve back to a marshal.",
      "Branch Note", cuwacunu::iinuji::text_line_emphasis_t::Accent,
      cuwacunu::iinuji::text_line_emphasis_t::Debug);
}

inline void append_runtime_session_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    bool primary) {
  append_runtime_detail_section(
      out, primary ? "Selected Marshal" : "Linked Marshal",
      primary ? cuwacunu::iinuji::text_line_emphasis_t::Accent
              : cuwacunu::iinuji::text_line_emphasis_t::Info);
  if (runtime_selected_none_session(st)) {
    append_runtime_none_session_detail(out, st);
    return;
  }
  if (!st.runtime.session.has_value()) {
    append_runtime_detail_text(out, "unavailable",
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }
  const auto &session = *st.runtime.session;
  append_runtime_detail_meta(out, "state", runtime_session_phase_text(session),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_session_row_emphasis(session));
  append_runtime_detail_meta(out, "objective",
                             session.objective_name.empty()
                                 ? std::string("<unnamed objective>")
                                 : session.objective_name);
  append_runtime_detail_meta(out, "marshal_session_id",
                             session.marshal_session_id);
  if (!session.active_campaign_cursor.empty()) {
    append_runtime_detail_meta(
        out, "active_campaign",
        runtime_campaign_ref_text(session.active_campaign_cursor));
  }
  append_runtime_detail_meta(out, "campaigns",
                             std::to_string(count_runtime_campaigns_for_session(
                                 st, session.marshal_session_id)));
  append_runtime_detail_meta(out, "jobs",
                             std::to_string(count_runtime_jobs_for_session(
                                 st, session.marshal_session_id)));
  append_runtime_detail_meta(out, "started_at",
                             format_time_marker_ms(session.started_at_ms));
  append_runtime_detail_meta(out, "updated_at",
                             format_time_marker_ms(session.updated_at_ms));
  if (session.finished_at_ms.has_value()) {
    append_runtime_detail_meta(
        out, "finished_at",
        format_optional_time_marker_ms(session.finished_at_ms));
  }
  if (session.pause_kind == "operator") {
    append_runtime_detail_meta(out, "pause_kind", session.pause_kind,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  if (!session.finish_reason.empty() && session.finish_reason != "none") {
    append_runtime_detail_meta(out, "finish_reason", session.finish_reason,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  if (!session.last_warning.empty()) {
    append_runtime_detail_meta(out, "last_warning", session.last_warning,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  if (!session.phase_detail.empty()) {
    append_runtime_note_block(out, session.phase_detail, "Marshal Note",
                              cuwacunu::iinuji::text_line_emphasis_t::Accent,
                              cuwacunu::iinuji::text_line_emphasis_t::None);
  }
}

inline void append_runtime_none_campaign_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const CmdState &st) {
  append_runtime_detail_meta(out, "state", "orphan branch",
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_runtime_detail_meta(out, "campaign_cursor",
                             runtime_none_branch_label(),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_runtime_detail_meta(out, "parent_marshal", runtime_none_branch_label(),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_runtime_detail_meta(
      out, "job_count", std::to_string(runtime_orphan_job_indices(st).size()),
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  if (st.runtime.job.has_value()) {
    append_runtime_detail_meta(out, "selected_job",
                               runtime_job_display_label(*st.runtime.job),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               runtime_job_row_emphasis(*st.runtime.job));
  }
  append_runtime_note_block(
      out,
      "This synthetic campaign branch groups jobs that do not resolve to a "
      "runtime campaign.",
      "Branch Note", cuwacunu::iinuji::text_line_emphasis_t::Accent,
      cuwacunu::iinuji::text_line_emphasis_t::Debug);
}

inline void append_runtime_campaign_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    bool primary) {
  append_runtime_detail_section(
      out, primary ? "Selected Campaign" : "Linked Campaign",
      primary ? cuwacunu::iinuji::text_line_emphasis_t::Accent
              : cuwacunu::iinuji::text_line_emphasis_t::Info);
  if (runtime_selected_none_campaign(st)) {
    append_runtime_none_campaign_detail(out, st);
    return;
  }
  if (!st.runtime.campaign.has_value()) {
    append_runtime_detail_text(out, "unavailable",
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }
  const auto &campaign = *st.runtime.campaign;
  append_runtime_detail_meta(out, "state", campaign.state,
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_campaign_row_emphasis(campaign));
  append_runtime_detail_meta(out, "campaign_cursor", campaign.campaign_cursor);
  const std::string parent_session_id =
      runtime_campaign_parent_session_id(st, campaign);
  append_runtime_detail_meta(out, "parent_marshal",
                             runtime_session_ref_text(parent_session_id));
  append_runtime_detail_meta(out, "current_run",
                             campaign.current_run_index.has_value()
                                 ? std::to_string(*campaign.current_run_index)
                                 : std::string("<none>"));
  append_runtime_detail_meta(out, "bind_count",
                             std::to_string(campaign.run_bind_ids.size()));
  append_runtime_detail_meta(out, "job_count",
                             std::to_string(count_runtime_jobs_for_campaign(
                                 st, campaign.campaign_cursor)));
  if (campaign.runner_pid.has_value()) {
    append_runtime_detail_meta(out, "runner_pid",
                               std::to_string(*campaign.runner_pid));
  }
  append_runtime_detail_meta(out, "started_at",
                             format_time_marker_ms(campaign.started_at_ms));
  append_runtime_detail_meta(out, "updated_at",
                             format_time_marker_ms(campaign.updated_at_ms));
  if (campaign.finished_at_ms.has_value()) {
    append_runtime_detail_meta(
        out, "finished_at",
        format_optional_time_marker_ms(campaign.finished_at_ms));
  }
  if (!campaign.state_detail.empty()) {
    append_runtime_note_block(out, campaign.state_detail, "Campaign Note",
                              cuwacunu::iinuji::text_line_emphasis_t::Info,
                              cuwacunu::iinuji::text_line_emphasis_t::None);
  }
}

inline cuwacunu::iinuji::text_line_emphasis_t runtime_job_device_emphasis(
    const cuwacunu::hero::runtime::runtime_job_record_t &job) {
  if (job.resolved_device.empty()) {
    return cuwacunu::iinuji::text_line_emphasis_t::Debug;
  }
  if (job.cuda_required &&
      job.resolved_device.find("cpu") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::MutedError;
  }
  if (job.resolved_device.find("cuda") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Debug;
}

inline void append_runtime_job_gpu_attachment_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    std::string_view label, const std::optional<std::uint64_t> &pid) {
  if (!pid.has_value())
    return;
  const auto refs = runtime_gpu_process_refs_for_pid(st, *pid);
  for (const auto *ref : refs) {
    if (ref == nullptr)
      continue;
    std::ostringstream oss;
    oss << label << " " << runtime_gpu_label_for_uuid(st, ref->gpu_uuid);
    if (ref->used_memory_mib != 0) {
      oss << " " << runtime_format_binary_mib(ref->used_memory_mib);
    }
    append_runtime_detail_meta(out, "gpu_attach", oss.str(),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Accent);
  }
}

inline void append_runtime_job_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    bool primary) {
  append_runtime_detail_section(
      out, primary ? "Selected Job" : "Linked Job",
      primary ? cuwacunu::iinuji::text_line_emphasis_t::Accent
              : cuwacunu::iinuji::text_line_emphasis_t::Info);
  if (!st.runtime.job.has_value()) {
    append_runtime_detail_text(out, "unavailable",
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }
  const auto &job = *st.runtime.job;
  append_runtime_detail_meta(out, "state", job.state,
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_job_row_emphasis(job));
  append_runtime_detail_meta(out, "binding_id",
                             job.binding_id.empty() ? std::string("<none>")
                                                    : job.binding_id);
  append_runtime_detail_meta(out, "job_cursor", job.job_cursor);
  const std::string parent_campaign =
      runtime_job_parent_campaign_cursor(st, job);
  const std::string parent_session = runtime_job_parent_session_id(st, job);
  append_runtime_detail_meta(out, "parent_campaign",
                             runtime_campaign_ref_text(parent_campaign));
  append_runtime_detail_meta(out, "parent_marshal",
                             runtime_session_ref_text(parent_session));
  const std::string device_request = job.requested_device.empty()
                                         ? std::string("<default>")
                                         : job.requested_device;
  const std::string device_resolved = job.resolved_device.empty()
                                          ? std::string("<unknown>")
                                          : job.resolved_device;
  append_runtime_detail_meta(out, "device",
                             device_request + " -> " + device_resolved,
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_job_device_emphasis(job));
  if (!job.device_error.empty()) {
    append_runtime_detail_meta(out, "device_error", job.device_error,
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
  append_runtime_detail_meta(out, "process", runtime_job_process_text(job),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_job_row_emphasis(job));
  append_runtime_job_gpu_attachment_detail(out, st, "runner", job.runner_pid);
  append_runtime_job_gpu_attachment_detail(out, st, "target", job.target_pid);
  append_runtime_detail_meta(out, "started_at",
                             format_time_marker_ms(job.started_at_ms));
  append_runtime_detail_meta(out, "updated_at",
                             format_time_marker_ms(job.updated_at_ms));
  if (job.finished_at_ms.has_value()) {
    append_runtime_detail_meta(
        out, "finished_at", format_optional_time_marker_ms(job.finished_at_ms));
  }
  if (!job.state_detail.empty()) {
    append_runtime_note_block(out, job.state_detail, "Job Note",
                              cuwacunu::iinuji::text_line_emphasis_t::Info,
                              cuwacunu::iinuji::text_line_emphasis_t::None);
  }
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_manifest_primary_styled_lines(const CmdState &st,
                                           RuntimeRowKind primary_kind);

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_manifest_primary_styled_lines(const CmdState &st,
                                           RuntimeRowKind primary_kind) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out{};
  switch (primary_kind) {
  case RuntimeRowKind::Device:
    append_runtime_selected_device_detail(out, st);
    break;
  case RuntimeRowKind::Session:
    append_runtime_session_detail(out, st, true);
    break;
  case RuntimeRowKind::Campaign:
    append_runtime_campaign_detail(out, st, true);
    break;
  case RuntimeRowKind::Job:
    append_runtime_job_detail(out, st, true);
    break;
  }
  return out;
}

inline void append_runtime_artifact_section(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::string_view title, const std::string &path, std::string note,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Info) {
  append_runtime_detail_section(out, title, emphasis);
  append_runtime_detail_meta(out, "path", runtime_path_text(path),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_path_emphasis(path));
  append_runtime_detail_text(out, std::move(note),
                             cuwacunu::iinuji::text_line_emphasis_t::Debug);
}

inline void append_runtime_excerpt_section(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::string_view title, const std::string &path, const std::string &excerpt,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Info) {
  append_runtime_detail_section(out, title, emphasis);
  append_runtime_detail_meta(out, "path", runtime_path_text(path),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_path_emphasis(path));
  append_runtime_detail_text(out, runtime_excerpt_text(excerpt),
                             cuwacunu::iinuji::text_line_emphasis_t::Debug);
}

inline bool runtime_try_parse_json_object_line(const std::string &raw_line,
                                               cmd_json_value_t *out) {
  if (out == nullptr)
    return false;
  *out = cmd_json_value_t{};
  try {
    *out = cuwacunu::piaabo::JsonParser(raw_line).parse();
  } catch (...) {
    return false;
  }
  return out->type == cmd_json_type_t::OBJECT;
}

inline std::string runtime_json_first_string(const cmd_json_value_t *object,
                                             std::string_view key_a,
                                             std::string_view key_b = {},
                                             std::string_view key_c = {},
                                             std::string_view key_d = {},
                                             std::string_view key_e = {}) {
  const std::string_view keys[] = {key_a, key_b, key_c, key_d, key_e};
  for (const auto key : keys) {
    if (key.empty())
      continue;
    const std::string value = cmd_json_string(cmd_json_field(object, key));
    if (!value.empty())
      return value;
  }
  return {};
}

inline std::string runtime_compact_log_text(std::string text,
                                            std::size_t limit = 116) {
  text = trim_copy(text);
  if (text.empty())
    return "<empty>";
  return trim_to_width(text, limit);
}

inline cuwacunu::iinuji::text_line_emphasis_t
runtime_marshal_event_emphasis(std::string_view event_name) {
  return runtime_marshaled_event_emphasis_from_name(event_name);
}

inline std::string runtime_event_viewer_selected_background(
    cuwacunu::iinuji::text_line_emphasis_t emphasis, bool body_line) {
  switch (emphasis) {
  case cuwacunu::iinuji::text_line_emphasis_t::Fatal:
  case cuwacunu::iinuji::text_line_emphasis_t::Error:
  case cuwacunu::iinuji::text_line_emphasis_t::MutedError:
    return body_line ? "#181015" : "#22131a";
  case cuwacunu::iinuji::text_line_emphasis_t::Warning:
  case cuwacunu::iinuji::text_line_emphasis_t::MutedWarning:
    return body_line ? "#17140f" : "#201a12";
  case cuwacunu::iinuji::text_line_emphasis_t::Success:
    return body_line ? "#111712" : "#152017";
  case cuwacunu::iinuji::text_line_emphasis_t::Accent:
    return body_line ? "#14131a" : "#1a1722";
  case cuwacunu::iinuji::text_line_emphasis_t::Info:
  case cuwacunu::iinuji::text_line_emphasis_t::Debug:
  case cuwacunu::iinuji::text_line_emphasis_t::None:
    return body_line ? "#131722" : "#171c29";
  }
  return body_line ? "#131722" : "#171c29";
}

inline cuwacunu::iinuji::text_line_emphasis_t
runtime_event_metadata_emphasis(std::string_view key, std::string_view value) {
  const std::string merged =
      to_lower_copy(std::string(key) + " " + std::string(value));
  if (merged.find("fail") != std::string::npos ||
      merged.find("error") != std::string::npos ||
      merged.find("terminate") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::MutedError;
  }
  if (merged.find("pause") != std::string::npos ||
      merged.find("warn") != std::string::npos ||
      merged.find("park") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (merged.find("checkpoint") != std::string::npos ||
      merged.find("launch") != std::string::npos ||
      merged.find("campaign") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Debug;
}

inline RuntimeEventViewerPanel
make_runtime_event_viewer_panel(const CmdState &st) {
  RuntimeEventViewerPanel panel{};
  auto &out = panel.lines;
  const auto &entries = st.runtime.marshal_event_viewer.entries;

  push_runtime_segments(
      out,
      {{.text = "timeline ",
        .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info},
       {.text = std::to_string(entries.size()) + " events",
        .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Accent},
       {.text =
            " | live=" +
            std::string(st.runtime.log_viewer_live_follow ? "follow" : "hold"),
        .emphasis = st.runtime.log_viewer_live_follow
                        ? cuwacunu::iinuji::text_line_emphasis_t::Success
                        : cuwacunu::iinuji::text_line_emphasis_t::Warning}},
      cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_runtime_segments(
      out,
      {{.text = "path ",
        .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info},
       {.text = runtime_path_text(st.runtime.log_viewer_path),
        .emphasis = runtime_path_emphasis(st.runtime.log_viewer_path)}},
      cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_runtime_line(out, {});

  if (entries.empty()) {
    push_runtime_line(out, "No marshal events recorded yet.",
                      cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return panel;
  }

  const std::size_t selected = std::min(
      st.runtime.marshal_event_viewer.selected_entry, entries.size() - 1);

  for (std::size_t i = 0; i < entries.size(); ++i) {
    const auto &entry = entries[i];
    const bool active = (i == selected);
    const std::string header_bg =
        active ? runtime_event_viewer_selected_background(entry.emphasis, false)
               : std::string{};
    const std::string body_bg =
        active ? runtime_event_viewer_selected_background(entry.emphasis, true)
               : std::string{};
    if (active)
      panel.selected_line = out.size();

    std::vector<cuwacunu::iinuji::styled_text_segment_t> header{};
    header.push_back(
        {.text = std::string(active ? "> " : "  "),
         .emphasis = active ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                            : cuwacunu::iinuji::text_line_emphasis_t::None});
    header.push_back(
        {.text = "[" + std::to_string(i + 1) + "] ",
         .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Debug});
    header.push_back(
        {.text = "[" + entry.event_name + "] ", .emphasis = entry.emphasis});
    if (entry.timestamp_ms != 0) {
      header.push_back(
          {.text = format_age_since_ms(entry.timestamp_ms),
           .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info});
      header.push_back(
          {.text = " | " + format_time_marker_ms(entry.timestamp_ms),
           .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Debug});
    }
    push_runtime_segments_with_background(
        out, std::move(header), header_bg,
        cuwacunu::iinuji::text_line_emphasis_t::None);

    push_runtime_line_with_background(
        out,
        std::string("    ") +
            runtime_compact_log_text(
                entry.summary.empty() ? entry.raw_line : entry.summary, 160),
        body_bg,
        (entry.emphasis == cuwacunu::iinuji::text_line_emphasis_t::Warning ||
         entry.emphasis == cuwacunu::iinuji::text_line_emphasis_t::MutedError)
            ? entry.emphasis
            : cuwacunu::iinuji::text_line_emphasis_t::None);

    if (!entry.metadata.empty()) {
      std::vector<cuwacunu::iinuji::styled_text_segment_t> meta{};
      meta.push_back(
          {.text = "    ",
           .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None});
      for (std::size_t j = 0; j < entry.metadata.size(); ++j) {
        if (j != 0) {
          meta.push_back(
              {.text = " | ",
               .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info});
        }
        meta.push_back(
            {.text = entry.metadata[j].first + "=",
             .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info});
        meta.push_back(
            {.text = entry.metadata[j].second,
             .emphasis = runtime_event_metadata_emphasis(
                 entry.metadata[j].first, entry.metadata[j].second)});
      }
      push_runtime_segments_with_background(
          out, std::move(meta), body_bg,
          cuwacunu::iinuji::text_line_emphasis_t::Info);
    }

    push_runtime_line_with_background(
        out, "", body_bg, cuwacunu::iinuji::text_line_emphasis_t::None);
  }
  return panel;
}

inline cuwacunu::iinuji::text_line_emphasis_t
runtime_codex_line_emphasis(std::string_view kind) {
  const std::string key = to_lower_copy(std::string(kind));
  if (key.find("stderr") != std::string::npos ||
      key.find("error") != std::string::npos ||
      key.find("fail") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (key.find("tool") != std::string::npos ||
      key.find("reason") != std::string::npos ||
      key.find("think") != std::string::npos ||
      key.find("plan") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  }
  if (key.find("done") != std::string::npos ||
      key.find("complete") != std::string::npos ||
      key.find("result") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  if (key.find("message") != std::string::npos ||
      key.find("assistant") != std::string::npos ||
      key.find("stdout") != std::string::npos) {
    return cuwacunu::iinuji::text_line_emphasis_t::Info;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Debug;
}

inline void append_runtime_marshaled_event_lines(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const std::vector<std::string> &lines) {
  for (const auto &raw_line : lines) {
    cmd_json_value_t parsed{};
    if (!runtime_try_parse_json_object_line(raw_line, &parsed)) {
      append_runtime_detail_text(out, runtime_compact_log_text(raw_line),
                                 cuwacunu::iinuji::text_line_emphasis_t::Debug,
                                 "  ");
      continue;
    }
    const std::uint64_t timestamp_ms =
        cmd_json_u64(cmd_json_field(&parsed, "timestamp_ms"));
    const std::string event_name =
        runtime_json_first_string(&parsed, "event", "type", "kind");
    const std::string detail = runtime_json_first_string(
        &parsed, "detail", "summary", "text", "message", "status");
    const auto event_emphasis = runtime_marshal_event_emphasis(event_name);
    const auto detail_emphasis =
        event_emphasis == cuwacunu::iinuji::text_line_emphasis_t::Warning ||
                event_emphasis ==
                    cuwacunu::iinuji::text_line_emphasis_t::MutedError
            ? event_emphasis
            : cuwacunu::iinuji::text_line_emphasis_t::None;
    std::vector<cuwacunu::iinuji::styled_text_segment_t> segments{};
    segments.push_back(
        {.text = "  ",
         .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None});
    if (timestamp_ms != 0) {
      segments.push_back(
          {.text = "[" + format_age_since_ms(timestamp_ms) + "] ",
           .emphasis = cuwacunu::iinuji::text_line_emphasis_t::Debug});
    }
    segments.push_back(
        {.text = "[" +
                 (event_name.empty() ? std::string("event") : event_name) +
                 "] ",
         .emphasis = event_emphasis});
    segments.push_back(
        {.text = runtime_compact_log_text(detail.empty() ? raw_line : detail),
         .emphasis = detail_emphasis});
    push_runtime_segments(out, std::move(segments),
                          cuwacunu::iinuji::text_line_emphasis_t::None);
  }
}

inline void append_runtime_codex_stream_lines(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const std::vector<std::string> &lines, std::string_view fallback_kind) {
  for (const auto &raw_line : lines) {
    cmd_json_value_t parsed{};
    if (!runtime_try_parse_json_object_line(raw_line, &parsed)) {
      append_runtime_detail_text(out, runtime_compact_log_text(raw_line),
                                 runtime_codex_line_emphasis(fallback_kind),
                                 "  ");
      continue;
    }

    std::string kind =
        runtime_json_first_string(&parsed, "type", "event", "stream", "kind");
    if (kind.empty())
      kind = std::string(fallback_kind);
    std::string detail = runtime_json_first_string(
        &parsed, "text", "detail", "message", "summary", "status");
    const std::string role = runtime_json_first_string(&parsed, "role");
    const std::string tool_name =
        runtime_json_first_string(&parsed, "tool_name");
    const auto line_index =
        cmd_json_optional_i64(cmd_json_field(&parsed, "line_index"));

    if (detail.empty() && !tool_name.empty()) {
      detail = "tool=" + tool_name;
    } else if (!tool_name.empty()) {
      detail = tool_name + " -> " + detail;
    }
    if (detail.empty())
      detail = raw_line;

    std::string label = kind;
    if (!role.empty() && to_lower_copy(role) != to_lower_copy(kind)) {
      label += "/" + role;
    }
    if (line_index.has_value()) {
      label += "#" + std::to_string(*line_index);
    }

    std::vector<cuwacunu::iinuji::styled_text_segment_t> segments{};
    segments.push_back(
        {.text = "  ",
         .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None});
    segments.push_back({.text = "[" + label + "] ",
                        .emphasis = runtime_codex_line_emphasis(kind)});
    segments.push_back(
        {.text = runtime_compact_log_text(detail),
         .emphasis = cuwacunu::iinuji::text_line_emphasis_t::None});
    push_runtime_segments(out, std::move(segments),
                          cuwacunu::iinuji::text_line_emphasis_t::None);
  }
}

inline void append_runtime_session_log_section(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::string_view title, const std::string &path, std::size_t tail_lines,
    std::string_view missing_label, std::string_view fallback_kind,
    cuwacunu::iinuji::text_line_emphasis_t section_emphasis,
    bool parse_as_events) {
  append_runtime_detail_section(out, title, section_emphasis);
  append_runtime_detail_meta(out, "path", runtime_path_text(path),
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             runtime_path_emphasis(path));

  if (path.empty()) {
    append_runtime_detail_text(out,
                               std::string(missing_label) +
                                   " path is not recorded for this session.",
                               cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }

  const auto lines = runtime_tail_text_file_lines(path, tail_lines);
  if (lines.empty()) {
    const bool live = runtime_session_is_active(session);
    append_runtime_detail_text(
        out,
        live ? std::string("waiting for ") + std::string(missing_label) + "..."
             : std::string(missing_label) + " unavailable.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }

  if (parse_as_events) {
    append_runtime_marshaled_event_lines(out, lines);
  } else {
    append_runtime_codex_stream_lines(out, lines, fallback_kind);
  }
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_log_styled_lines(const CmdState &st, RuntimeRowKind primary_kind) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out{};
  if (primary_kind == RuntimeRowKind::Device) {
    append_runtime_selected_device_detail(out, st);
    push_runtime_line(out, {});
    append_runtime_detail_section(out, "Device Logs",
                                  cuwacunu::iinuji::text_line_emphasis_t::Info);
    append_runtime_detail_text(
        out,
        "device lane has no log artifact; stay on manifest for live meters",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return out;
  }
  if (primary_kind == RuntimeRowKind::Session &&
      runtime_selected_none_session(st)) {
    append_runtime_detail_section(
        out, "Orphan Marshal Branch",
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
    append_runtime_detail_text(
        out,
        "This synthetic marshal row has no marshal-owned logs. Orphan "
        "campaign and job logs are shown below when a child is selected.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    push_runtime_line(out, {});
  }
  if (primary_kind == RuntimeRowKind::Session &&
      st.runtime.session.has_value()) {
    append_runtime_session_log_section(
        out, *st.runtime.session, "Marshal Event Timeline",
        st.runtime.session->events_path, 8, "marshal events", "event",
        cuwacunu::iinuji::text_line_emphasis_t::Info, true);
    push_runtime_line(out, {});
    append_runtime_session_log_section(
        out, *st.runtime.session, "Codex Stdout",
        st.runtime.session->codex_stdout_path, 5, "codex stdout", "stdout",
        cuwacunu::iinuji::text_line_emphasis_t::Accent, false);
    push_runtime_line(out, {});
    append_runtime_session_log_section(
        out, *st.runtime.session, "Codex Stderr",
        st.runtime.session->codex_stderr_path, 5, "codex stderr", "stderr",
        cuwacunu::iinuji::text_line_emphasis_t::Warning, false);
    push_runtime_line(out, {});
  }
  if (primary_kind == RuntimeRowKind::Campaign &&
      runtime_selected_none_campaign(st)) {
    append_runtime_detail_section(
        out, "Orphan Campaign Branch",
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
    append_runtime_detail_text(
        out,
        "This synthetic campaign row has no campaign-owned logs. Orphan job "
        "logs are shown below when a child is selected.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    push_runtime_line(out, {});
  }
  if (st.runtime.campaign.has_value()) {
    append_runtime_artifact_section(
        out, "Campaign stdout", st.runtime.campaign->stdout_path,
        "campaign log tailing remains owned by Runtime Hero",
        cuwacunu::iinuji::text_line_emphasis_t::Info);
    push_runtime_line(out, {});
    append_runtime_artifact_section(
        out, "Campaign stderr", st.runtime.campaign->stderr_path,
        "campaign log tailing remains owned by Runtime Hero",
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
    push_runtime_line(out, {});
  }
  if (st.runtime.job.has_value()) {
    append_runtime_excerpt_section(
        out, "Job stdout", st.runtime.job->stdout_path,
        st.runtime.job_stdout_excerpt,
        cuwacunu::iinuji::text_line_emphasis_t::Info);
    push_runtime_line(out, {});
    append_runtime_excerpt_section(
        out, "Job stderr", st.runtime.job->stderr_path,
        st.runtime.job_stderr_excerpt,
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
    return out;
  }
  if (runtime_selected_none_campaign(st) || runtime_selected_none_session(st)) {
    append_runtime_detail_section(out, "Job Logs",
                                  cuwacunu::iinuji::text_line_emphasis_t::Info);
    append_runtime_detail_text(
        out, "Select an orphan job under <none> to inspect stdout and stderr.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return out;
  }
  append_runtime_detail_section(
      out, "Job stderr", cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_runtime_detail_text(out, "job log unavailable",
                             cuwacunu::iinuji::text_line_emphasis_t::Debug);
  return out;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_trace_styled_lines(const CmdState &st,
                                RuntimeRowKind primary_kind) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out{};
  if (primary_kind == RuntimeRowKind::Device) {
    append_runtime_selected_device_detail(out, st);
    push_runtime_line(out, {});
    append_runtime_detail_section(out, "Device Trace",
                                  cuwacunu::iinuji::text_line_emphasis_t::Info);
    append_runtime_detail_text(
        out,
        "device lane has no trace artifact; stay on manifest for live meters",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return out;
  }
  if (primary_kind == RuntimeRowKind::Session &&
      runtime_selected_none_session(st)) {
    append_runtime_detail_section(
        out, "Orphan Marshal Branch",
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
    append_runtime_detail_text(
        out,
        "This synthetic marshal row has no marshal checkpoint trace. Orphan "
        "job traces are shown below when a child is selected.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    push_runtime_line(out, {});
  }
  if (primary_kind == RuntimeRowKind::Session &&
      st.runtime.session.has_value()) {
    append_runtime_detail_section(out, "Marshal Checkpoints",
                                  cuwacunu::iinuji::text_line_emphasis_t::Info);
    append_runtime_detail_meta(
        out, "latest_intent_checkpoint",
        runtime_path_text(st.runtime.session->last_intent_checkpoint_path),
        cuwacunu::iinuji::text_line_emphasis_t::Info,
        runtime_path_emphasis(st.runtime.session->last_intent_checkpoint_path));
    append_runtime_detail_text(
        out, "marshal checkpoint tailing remains owned by Marshal Hero",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    push_runtime_line(out, {});
    append_runtime_detail_meta(
        out, "latest_mutation_checkpoint",
        runtime_path_text(st.runtime.session->last_mutation_checkpoint_path),
        cuwacunu::iinuji::text_line_emphasis_t::Info,
        runtime_path_emphasis(
            st.runtime.session->last_mutation_checkpoint_path));
    append_runtime_detail_text(
        out, "marshal checkpoint tailing remains owned by Marshal Hero",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    push_runtime_line(out, {});
  }
  if (primary_kind == RuntimeRowKind::Campaign &&
      runtime_selected_none_campaign(st)) {
    append_runtime_detail_section(
        out, "Orphan Campaign Branch",
        cuwacunu::iinuji::text_line_emphasis_t::Warning);
    append_runtime_detail_text(
        out,
        "This synthetic campaign row has no campaign trace. Orphan job traces "
        "are shown below when a child is selected.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    push_runtime_line(out, {});
  }
  if (st.runtime.job.has_value()) {
    append_runtime_excerpt_section(
        out, "Job Trace", st.runtime.job->trace_path,
        st.runtime.job_trace_excerpt,
        cuwacunu::iinuji::text_line_emphasis_t::Info);
    return out;
  }
  if (runtime_selected_none_campaign(st) || runtime_selected_none_session(st)) {
    append_runtime_detail_section(out, "Job Trace",
                                  cuwacunu::iinuji::text_line_emphasis_t::Info);
    append_runtime_detail_text(
        out, "Select an orphan job under <none> to inspect trace output.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return out;
  }
  append_runtime_detail_section(out, "Job Trace",
                                cuwacunu::iinuji::text_line_emphasis_t::Info);
  append_runtime_detail_text(out, "job trace unavailable",
                             cuwacunu::iinuji::text_line_emphasis_t::Debug);
  return out;
}

inline bool runtime_show_device_history_panel(const CmdState &st) {
  return !runtime_log_viewer_is_open(st) &&
         runtime_primary_kind(st) == RuntimeRowKind::Device;
}

inline bool runtime_show_secondary_panel(const CmdState &st) {
  return runtime_show_device_history_panel(st);
}

inline std::string runtime_primary_panel_title(const CmdState &st) {
  if (runtime_log_viewer_is_open(st)) {
    if (!runtime_log_viewer_supports_follow(st)) {
      return " " + runtime_log_viewer_title(st) + " [viewer] ";
    }
    return " " + runtime_log_viewer_title(st) +
           (st.runtime.log_viewer_live_follow ? " [viewer][live] "
                                              : " [viewer][hold] ");
  }
  switch (runtime_primary_kind(st)) {
  case RuntimeRowKind::Device:
    return " details ";
  case RuntimeRowKind::Session:
    return " selected marshal ";
  case RuntimeRowKind::Campaign:
    return " selected campaign ";
  case RuntimeRowKind::Job:
    return " selected job ";
  }
  return " runtime detail ";
}

inline std::string runtime_secondary_panel_title(const CmdState &st) {
  if (runtime_show_device_history_panel(st)) {
    return " history ";
  }
  return {};
}

struct runtime_styled_box_metrics_t {
  int text_h{0};
  int total_rows{0};
  int max_scroll_y{0};
  int selected_row{0};
};

inline runtime_styled_box_metrics_t measure_runtime_styled_box(
    const std::vector<cuwacunu::iinuji::styled_text_line_t> &lines, int width,
    int height, bool wrap, std::optional<std::size_t> selected_line) {
  runtime_styled_box_metrics_t out{};
  if (width <= 0 || height <= 0)
    return out;

  int reserve_v = 0;
  int reserve_h = 0;
  int text_w = width;
  int text_h = height;
  int total_rows = 0;
  int max_line_len = 0;

  auto measure_lines = [&](int current_width, int *out_total_rows,
                           int *out_max_line_len,
                           std::vector<int> *out_line_offsets) {
    const int safe_w = std::max(1, current_width);
    int rows = 0;
    int line_len = 0;
    if (out_line_offsets)
      out_line_offsets->assign(lines.size(), 0);
    for (std::size_t i = 0; i < lines.size(); ++i) {
      if (out_line_offsets)
        (*out_line_offsets)[i] = rows;
      if (cuwacunu::iinuji::ansi::has_esc(lines[i].text)) {
        cuwacunu::iinuji::ansi::style_t base{};
        const auto physical_lines = split_lines_keep_empty(lines[i].text);
        if (physical_lines.empty()) {
          ++rows;
          continue;
        }
        for (const auto &physical_line : physical_lines) {
          std::vector<cuwacunu::iinuji::ansi::row_t> wrapped_rows{};
          cuwacunu::iinuji::ansi::hard_wrap(physical_line, safe_w, base, 0,
                                            wrapped_rows);
          if (wrapped_rows.empty()) {
            ++rows;
            continue;
          }
          for (std::size_t j = 0; j < wrapped_rows.size(); ++j) {
            line_len = std::max(line_len, wrapped_rows[j].len);
            ++rows;
            if (!wrap)
              break;
          }
          if (!wrap)
            break;
        }
        continue;
      }
      const auto chunks = wrap ? wrap_text(lines[i].text, safe_w)
                               : split_lines_keep_empty(lines[i].text);
      if (chunks.empty()) {
        ++rows;
        continue;
      }
      for (std::size_t j = 0; j < chunks.size(); ++j) {
        line_len = std::max(line_len, static_cast<int>(chunks[j].size()));
        ++rows;
        if (!wrap)
          break;
      }
    }
    if (out_total_rows)
      *out_total_rows = rows;
    if (out_max_line_len)
      *out_max_line_len = line_len;
  };

  for (int it = 0; it < 3; ++it) {
    text_w = std::max(0, width - reserve_v);
    text_h = std::max(0, height - reserve_h);
    if (text_w <= 0 || text_h <= 0)
      return out;

    measure_lines(text_w, &total_rows, &max_line_len, nullptr);

    const bool need_h = (!wrap && max_line_len > text_w);
    const int reserve_h_new = need_h ? 1 : 0;
    const int text_h_if = std::max(0, height - reserve_h_new);
    const bool need_v = total_rows > text_h_if;
    const int reserve_v_new = need_v ? 1 : 0;
    if (reserve_h_new == reserve_h && reserve_v_new == reserve_v)
      break;
    reserve_h = reserve_h_new;
    reserve_v = reserve_v_new;
  }

  text_w = std::max(0, width - reserve_v);
  text_h = std::max(0, height - reserve_h);
  if (text_w <= 0 || text_h <= 0)
    return out;

  std::vector<int> line_offsets{};
  measure_lines(text_w, &total_rows, &max_line_len, &line_offsets);

  out.text_h = text_h;
  out.total_rows = total_rows;
  out.max_scroll_y = std::max(0, total_rows - text_h);
  if (selected_line.has_value() && *selected_line < line_offsets.size()) {
    out.selected_row = line_offsets[*selected_line];
  }
  return out;
}

inline const RuntimeDeviceGpuSnapshot *
runtime_find_gpu_in_snapshot(const RuntimeDeviceSnapshot &snapshot,
                             const RuntimeDeviceGpuSnapshot &current_gpu) {
  for (const auto &gpu : snapshot.gpus) {
    if (!current_gpu.uuid.empty() && gpu.uuid == current_gpu.uuid)
      return &gpu;
  }
  for (const auto &gpu : snapshot.gpus) {
    if (!current_gpu.index.empty() && gpu.index == current_gpu.index)
      return &gpu;
  }
  return nullptr;
}

inline std::vector<std::vector<std::pair<double, double>>>
make_runtime_device_history_series(const CmdState &st) {
  std::vector<std::vector<std::pair<double, double>>> series{};
  if (st.runtime.device_history.empty())
    return series;

  const std::size_t selected = runtime_selected_device_index(st).value_or(0);
  if (selected == 0) {
    std::vector<std::pair<double, double>> cpu{};
    std::vector<std::pair<double, double>> ram{};
    for (std::size_t i = 0; i < st.runtime.device_history.size(); ++i) {
      const auto &snapshot = st.runtime.device_history[i];
      const double x = static_cast<double>(i);
      cpu.push_back(
          {x, static_cast<double>(runtime_host_cpu_percent(snapshot))});
      if (snapshot.mem_total_kib != 0) {
        ram.push_back(
            {x, static_cast<double>(runtime_host_memory_percent(snapshot))});
      }
    }
    if (!cpu.empty())
      series.push_back(std::move(cpu));
    if (!ram.empty())
      series.push_back(std::move(ram));
    return series;
  }

  const std::size_t gpu_index = selected - 1;
  if (gpu_index >= st.runtime.device.gpus.size())
    return series;
  const auto &current_gpu = st.runtime.device.gpus[gpu_index];

  std::vector<std::pair<double, double>> core{};
  std::vector<std::pair<double, double>> memory{};
  for (std::size_t i = 0; i < st.runtime.device_history.size(); ++i) {
    const auto &snapshot = st.runtime.device_history[i];
    const auto *gpu = runtime_find_gpu_in_snapshot(snapshot, current_gpu);
    if (gpu == nullptr)
      continue;
    const double x = static_cast<double>(i);
    core.push_back({x, static_cast<double>(std::max(0, gpu->util_gpu_pct))});
    memory.push_back(
        {x, static_cast<double>(runtime_gpu_memory_percent(*gpu))});
  }
  if (!core.empty())
    series.push_back(std::move(core));
  if (!memory.empty())
    series.push_back(std::move(memory));
  return series;
}

inline std::vector<cuwacunu::iinuji::plot_series_cfg_t>
make_runtime_device_history_plot_cfg(const CmdState &st) {
  std::vector<cuwacunu::iinuji::plot_series_cfg_t> cfg{};
  if (runtime_selected_device_index(st).value_or(0) == 0) {
    cfg.push_back(cuwacunu::iinuji::plot_series_cfg_t{
        .color_fg = "#f0d060",
        .mode = cuwacunu::iinuji::plot_mode_t::Line,
    });
    cfg.push_back(cuwacunu::iinuji::plot_series_cfg_t{
        .color_fg = "#67c1ff",
        .mode = cuwacunu::iinuji::plot_mode_t::Line,
    });
    return cfg;
  }
  cfg.push_back(cuwacunu::iinuji::plot_series_cfg_t{
      .color_fg = "#7fe08a",
      .mode = cuwacunu::iinuji::plot_mode_t::Line,
  });
  cfg.push_back(cuwacunu::iinuji::plot_series_cfg_t{
      .color_fg = "#67c1ff",
      .mode = cuwacunu::iinuji::plot_mode_t::Line,
  });
  return cfg;
}

inline cuwacunu::iinuji::plotbox_opts_t
make_runtime_device_history_plot_opts() {
  cuwacunu::iinuji::plotbox_opts_t opts{};
  opts.draw_axes = false;
  opts.draw_grid = true;
  opts.baseline0 = false;
  opts.y_ticks = 4;
  opts.x_ticks = 0;
  opts.y_min = 0.0;
  opts.y_max = 100.0;
  opts.margin_left = 0;
  opts.margin_right = 0;
  opts.margin_top = 0;
  opts.margin_bot = 0;
  return opts;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_primary_styled_lines(const CmdState &st) {
  if (runtime_event_viewer_is_open(st)) {
    return make_runtime_event_viewer_panel(st).lines;
  }
  std::vector<cuwacunu::iinuji::styled_text_line_t> out{};
  const RuntimeRowKind primary_kind = runtime_primary_kind(st);
  std::vector<cuwacunu::iinuji::styled_text_line_t> body{};
  switch (st.runtime.detail_mode) {
  case RuntimeDetailMode::Manifest:
    body = make_runtime_manifest_primary_styled_lines(st, primary_kind);
    break;
  case RuntimeDetailMode::Log:
    body = make_runtime_log_styled_lines(st, primary_kind);
    break;
  case RuntimeDetailMode::Trace:
    body = make_runtime_trace_styled_lines(st, primary_kind);
    break;
  }
  if (body.empty())
    body = make_runtime_manifest_primary_styled_lines(st, primary_kind);
  out.insert(out.end(), body.begin(), body.end());
  return out;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_runtime_right_styled_lines(const CmdState &st) {
  return make_runtime_primary_styled_lines(st);
}

inline std::string make_runtime_right(const CmdState &st) {
  return runtime_styled_lines_to_text(make_runtime_right_styled_lines(st));
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
