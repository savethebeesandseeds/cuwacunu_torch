#pragma once

#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "iinuji/iinuji_cmd/views/workbench/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool workbench_has_request_for_session(const CmdState &st,
                                          std::string_view marshal_session_id) {
  return workbench_session_has_pending_request(st, marshal_session_id);
}

inline bool workbench_has_review_for_session(const CmdState &st,
                                         std::string_view marshal_session_id) {
  return workbench_session_has_pending_review(st, marshal_session_id);
}

inline std::string workbench_state_text(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return cuwacunu::hero::human_mcp::operator_session_state_detail(
      session, workbench_has_review_for_session(st, session.marshal_session_id));
}

inline cuwacunu::iinuji::text_line_emphasis_t
workbench_selected_emphasis(bool focused) {
  return focused ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                 : cuwacunu::iinuji::text_line_emphasis_t::Debug;
}

inline void
push_workbench_line(std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
                std::string text,
                cuwacunu::iinuji::text_line_emphasis_t emphasis =
                    cuwacunu::iinuji::text_line_emphasis_t::None) {
  out.push_back(cuwacunu::iinuji::styled_text_line_t{
      .text = std::move(text),
      .emphasis = emphasis,
  });
}

inline std::string workbench_section_title(const CmdState &st) {
  return workbench_is_queue_section(st.workbench.section) ? "Worklist"
                                                          : "Booster";
}

inline std::string workbench_operator_label(const CmdState &st) {
  return st.workbench.app.defaults.operator_id.empty()
             ? std::string("<unset>")
             : st.workbench.app.defaults.operator_id;
}

inline std::string workbench_lifecycle_badge(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (session.lifecycle == "live" && session.activity == "review") {
    if (session.finish_reason == "interrupted")
      return "[INTERRUPTED]";
    return "[IDLE]";
  }
  if (session.lifecycle == "terminal") {
    if (session.finish_reason == "success")
      return "[DONE]";
    if (session.finish_reason == "exhausted")
      return "[EXHAUSTED]";
    if (session.finish_reason == "failed")
      return "[FAILED]";
    if (session.finish_reason == "terminated")
      return "[TERMINATED]";
    return "[DONE]";
  }
  return cuwacunu::hero::human_mcp::session_state_badge(session);
}

inline cuwacunu::iinuji::text_line_emphasis_t workbench_session_row_emphasis(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (workbench_has_request_for_session(st, session.marshal_session_id)) {
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (session.finish_reason == "interrupted") {
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (workbench_session_is_operator_paused(session)) {
    return cuwacunu::iinuji::text_line_emphasis_t::Info;
  }
  if (workbench_has_review_for_session(st, session.marshal_session_id)) {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::None;
}

inline cuwacunu::iinuji::text_line_emphasis_t
workbench_row_emphasis(cuwacunu::iinuji::text_line_emphasis_t base, bool active,
                   bool focused) {
  if (active) {
    (void)base;
    (void)focused;
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  }
  return base;
}

inline std::string workbench_worklist_objective(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return session.objective_name.empty() ? std::string("<unnamed objective>")
                                        : session.objective_name;
}

inline std::string workbench_middle_compact(std::string value,
                                        std::size_t max_width) {
  if (value.size() <= max_width)
    return value;
  if (max_width <= 3)
    return value.substr(0, max_width);
  const std::size_t keep = max_width - 3;
  const std::size_t left = keep / 2;
  const std::size_t right = keep - left;
  return value.substr(0, left) + "..." + value.substr(value.size() - right);
}

inline std::string workbench_worklist_session_id(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string marshal_session_id = session.marshal_session_id.empty()
                                       ? std::string("<none>")
                                       : session.marshal_session_id;
  if (marshal_session_id.rfind("marshal.", 0) == 0) {
    marshal_session_id.erase(0, std::string("marshal.").size());
  }
  return workbench_middle_compact(std::move(marshal_session_id), 16);
}

inline std::string workbench_worklist_extra_info(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (workbench_has_request_for_session(st, session.marshal_session_id)) {
    return "request=" +
           trim_to_width(
               std::string(cuwacunu::hero::human_mcp::human_request_kind_label(
                   session.work_gate)),
               14);
  }
  if (workbench_session_is_operator_paused(session)) {
    return "pause=operator";
  }
  if (workbench_has_review_for_session(st, session.marshal_session_id)) {
    if (!session.finish_reason.empty() && session.finish_reason != "none") {
      return "review=" + trim_to_width(session.finish_reason, 14);
    }
    return "review=pending";
  }
  if (session.finish_reason == "interrupted") {
    return "state=interrupted";
  }
  return "state=" + trim_to_width(session.lifecycle.empty() ? std::string("workbench")
                                                        : session.lifecycle,
                                  14);
}

inline std::string workbench_next_action_text(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return cuwacunu::hero::human_mcp::operator_session_action_hint(
      session, workbench_has_review_for_session(st, session.marshal_session_id));
}

inline std::string
workbench_emphasis_ansi_prefix(cuwacunu::iinuji::text_line_emphasis_t emphasis,
                           bool dim = false) {
  std::string out = "\x1b[0m";
  if (dim)
    out += "\x1b[2m";
  switch (emphasis) {
  case cuwacunu::iinuji::text_line_emphasis_t::Accent:
    out += "\x1b[38;2;200;156;58m";
    if (!dim)
      out += "\x1b[1m";
    break;
  case cuwacunu::iinuji::text_line_emphasis_t::Success:
    out += "\x1b[38;2;77;122;82m";
    break;
  case cuwacunu::iinuji::text_line_emphasis_t::Fatal:
    out += "\x1b[38;2;255;0;0m";
    if (!dim)
      out += "\x1b[1m";
    break;
  case cuwacunu::iinuji::text_line_emphasis_t::Error:
    out += "\x1b[38;2;198;28;65m";
    if (!dim)
      out += "\x1b[1m";
    break;
  case cuwacunu::iinuji::text_line_emphasis_t::MutedError:
    out += "\x1b[38;2;159;90;99m";
    break;
  case cuwacunu::iinuji::text_line_emphasis_t::Warning:
    out += "\x1b[38;2;200;146;44m";
    if (!dim)
      out += "\x1b[1m";
    break;
  case cuwacunu::iinuji::text_line_emphasis_t::MutedWarning:
    out += "\x1b[38;2;200;146;44m";
    break;
  case cuwacunu::iinuji::text_line_emphasis_t::Info:
    out += "\x1b[38;2;150;152;154m";
    break;
  case cuwacunu::iinuji::text_line_emphasis_t::Debug:
    out += "\x1b[38;2;63;134;199m";
    break;
  case cuwacunu::iinuji::text_line_emphasis_t::None:
    out += "\x1b[38;2;208;208;208m";
    break;
  }
  return out;
}

inline std::string
workbench_ansi_segment(std::string_view text,
                   cuwacunu::iinuji::text_line_emphasis_t emphasis,
                   bool dim = false) {
  if (text.empty())
    return {};
  return workbench_emphasis_ansi_prefix(emphasis, dim) + std::string(text) +
         "\x1b[0m";
}

inline std::string workbench_pad_trim_to_width(const std::string &value,
                                           std::size_t width) {
  std::string out = trim_to_width(value, static_cast<int>(width));
  if (out.size() < width)
    out.append(width - out.size(), ' ');
  return out;
}

inline std::size_t workbench_worklist_index_width(std::size_t total_rows) {
  std::size_t width = 1;
  while (total_rows >= 10) {
    total_rows /= 10;
    ++width;
  }
  return std::max<std::size_t>(2, width);
}

inline std::string workbench_navigation_row_text(const CmdState &st,
                                                 workbench_section_t section) {
  std::ostringstream oss;
  oss << (st.workbench.section == section ? "> " : "  ") << "["
      << workbench_section_label(section) << "] ";
  if (workbench_is_queue_section(section)) {
    oss << count_workbench_sessions(st) << " waiting";
  } else {
    oss << workbench_booster_action_count() << " utilities";
  }
  return oss.str();
}

inline std::string workbench_booster_row_text(
    workbench_booster_action_kind_t action, bool active) {
  return std::string(active ? "> " : "  ") +
         workbench_booster_action_label(action);
}

inline std::string workbench_worklist_row_text(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::size_t row_index, std::size_t total_rows, bool active,
    cuwacunu::iinuji::text_line_emphasis_t emphasis) {
  constexpr std::size_t kWorkbenchObjectiveWidth = 28;
  constexpr std::size_t kWorkbenchSessionWidth = 16;
  constexpr std::size_t kWorkbenchExtraWidth = 22;

  std::ostringstream lead;
  lead << (active ? "> " : "  ") << "["
       << std::setw(static_cast<int>(workbench_worklist_index_width(total_rows)))
       << (row_index + 1) << "] "
       << workbench_pad_trim_to_width(workbench_worklist_objective(session),
                                  kWorkbenchObjectiveWidth);

  std::string meta = workbench_pad_trim_to_width(workbench_worklist_session_id(session),
                                             kWorkbenchSessionWidth);
  const std::string extra = workbench_worklist_extra_info(st, session);
  if (!extra.empty()) {
    meta += " | " + workbench_pad_trim_to_width(extra, kWorkbenchExtraWidth);
  }

  return workbench_ansi_segment(lead.str(), emphasis, false) +
         workbench_ansi_segment(" | " + meta, emphasis, true);
}

inline std::string workbench_detail_rule() { return panel_rule('-', 56); }

inline void append_workbench_detail_section(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::string_view title,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Accent) {
  push_workbench_line(out, workbench_detail_rule(), emphasis);
  push_workbench_line(out, std::string(title), emphasis);
  push_workbench_line(out, workbench_detail_rule(), emphasis);
}

inline void
append_workbench_detail_text(std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
                         std::string text,
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
    push_workbench_line(out, std::string(prefix) + std::string(line), emphasis);
    if (end == std::string::npos)
      break;
    start = end + 1;
    if (start == text.size())
      break;
  }
}

inline std::string workbench_detail_meta_prefix(std::string_view key) {
  constexpr std::size_t kWorkbenchDetailKeyWidth = 18;
  std::string prefix = std::string(key) + ":";
  if (prefix.size() < kWorkbenchDetailKeyWidth) {
    prefix.append(kWorkbenchDetailKeyWidth - prefix.size(), ' ');
  } else {
    prefix.push_back(' ');
  }
  return prefix;
}

inline void
append_workbench_detail_meta(std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
                         std::string_view key, std::string value,
                         cuwacunu::iinuji::text_line_emphasis_t key_emphasis =
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                         cuwacunu::iinuji::text_line_emphasis_t value_emphasis =
                             cuwacunu::iinuji::text_line_emphasis_t::Debug) {
  if (value.empty())
    value = "<none>";
  const std::string prefix = workbench_detail_meta_prefix(key);
  const auto line_emphasis =
      (value_emphasis == cuwacunu::iinuji::text_line_emphasis_t::None ||
       value_emphasis == cuwacunu::iinuji::text_line_emphasis_t::Debug)
          ? key_emphasis
          : value_emphasis;
  std::size_t start = 0;
  while (start <= value.size()) {
    const std::size_t end = value.find('\n', start);
    const std::string_view line =
        end == std::string::npos
            ? std::string_view(value).substr(start)
            : std::string_view(value).substr(start, end - start);
    push_workbench_line(out,
                    (start == 0 ? prefix : std::string(prefix.size(), ' ')) +
                        std::string(line),
                    line_emphasis);
    if (end == std::string::npos)
      break;
    start = end + 1;
    if (start == value.size())
      break;
  }
}

inline cuwacunu::iinuji::text_line_emphasis_t
workbench_detail_session_state_emphasis(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (session.finish_reason == "failed" ||
      session.finish_reason == "terminated") {
    return cuwacunu::iinuji::text_line_emphasis_t::Error;
  }
  if (workbench_has_request_for_session(st, session.marshal_session_id)) {
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (workbench_has_review_for_session(st, session.marshal_session_id) ||
      session.finish_reason == "success") {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  if (workbench_session_is_operator_paused(session) ||
      (session.lifecycle == "live" && session.activity == "review") ||
      session.lifecycle == "terminal") {
    return cuwacunu::iinuji::text_line_emphasis_t::Info;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Debug;
}

inline void append_workbench_common_session_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  append_workbench_detail_meta(out, "operator_state",
                           workbench_lifecycle_badge(session) + " " +
                               workbench_state_text(st, session),
                           cuwacunu::iinuji::text_line_emphasis_t::Info,
                           workbench_detail_session_state_emphasis(st, session));
  append_workbench_detail_meta(out, "next_action",
                           workbench_next_action_text(st, session),
                           cuwacunu::iinuji::text_line_emphasis_t::Info,
                           cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_workbench_detail_meta(out, "objective", session.objective_name);
  append_workbench_detail_meta(out, "marshal_session_id",
                           session.marshal_session_id);
  append_workbench_detail_meta(out, "authority_scope", session.authority_scope);
  append_workbench_detail_meta(out, "active_campaign",
                           session.campaign_cursor.empty()
                               ? std::string("<none>")
                               : session.campaign_cursor);
  append_workbench_detail_meta(out, "campaign_history",
                           std::to_string(session.campaign_cursors.size()) +
                               " linked campaigns");
  append_workbench_detail_meta(out, "started_at",
                           format_time_marker_ms(session.started_at_ms));
  append_workbench_detail_meta(out, "updated_at",
                           format_time_marker_ms(session.updated_at_ms));
  append_workbench_detail_meta(
      out, "finished_at",
      format_optional_time_marker_ms(session.finished_at_ms));
  if (!session.last_codex_action.empty()) {
    append_workbench_detail_meta(out, "last_intent", session.last_codex_action);
  }
  if (!session.last_warning.empty()) {
    append_workbench_detail_meta(out, "last_warning", session.last_warning,
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
}

inline void append_workbench_request_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &request) {
  append_workbench_detail_section(
      out, "Workbench Request",
      cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_workbench_detail_meta(out, "focus", "worklist");
  append_workbench_detail_meta(out, "operator_state",
                               workbench_state_text(st, request),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_workbench_detail_meta(out, "next_action",
                               workbench_next_action_text(st, request),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_workbench_detail_meta(
      out, "request_kind",
      std::string(cuwacunu::hero::human_mcp::human_request_kind_label(
          request.work_gate)),
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      cuwacunu::iinuji::text_line_emphasis_t::Warning);
  append_workbench_detail_meta(out, "objective", request.objective_name);
  append_workbench_detail_meta(out, "marshal_session_id",
                               request.marshal_session_id);
  append_workbench_detail_meta(out, "checkpoint",
                               std::to_string(request.checkpoint_count));
  append_workbench_detail_meta(out, "started_at",
                               format_time_marker_ms(request.started_at_ms));
  append_workbench_detail_meta(out, "updated_at",
                               format_time_marker_ms(request.updated_at_ms));
  append_workbench_detail_meta(out, "active_campaign",
                               request.campaign_cursor.empty()
                                   ? std::string("<none>")
                                   : request.campaign_cursor);
  push_workbench_line(out, {});
  append_workbench_detail_section(
      out, "Request Body", cuwacunu::iinuji::text_line_emphasis_t::Warning);
  if (!st.workbench.detail_loaded ||
      st.workbench.detail_kind != workbench_detail_kind_t::Request ||
      st.workbench.detail_session_id != request.marshal_session_id) {
    append_workbench_detail_text(out, "<loading request body...>",
                                 cuwacunu::iinuji::text_line_emphasis_t::Debug);
  } else if (!st.workbench.detail_error.empty()) {
    append_workbench_detail_text(
        out, "<failed to read request: " + st.workbench.detail_error + ">",
        cuwacunu::iinuji::text_line_emphasis_t::Error);
  } else {
    append_workbench_detail_text(out, st.workbench.detail_text);
  }
}

inline void append_workbench_review_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &review) {
  append_workbench_detail_section(
      out, "Workbench Review",
      cuwacunu::iinuji::text_line_emphasis_t::Success);
  append_workbench_detail_meta(out, "focus", "worklist");
  append_workbench_detail_meta(out, "operator_state",
                               workbench_state_text(st, review),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Success);
  append_workbench_detail_meta(out, "next_action",
                               workbench_next_action_text(st, review),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_workbench_detail_meta(out, "objective", review.objective_name);
  append_workbench_detail_meta(out, "marshal_session_id",
                               review.marshal_session_id);
  append_workbench_detail_meta(
      out, "effort", cuwacunu::hero::human_mcp::build_summary_effort_text(review),
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      cuwacunu::iinuji::text_line_emphasis_t::Success);
  append_workbench_detail_meta(
      out, "actions",
      (review.lifecycle == "live" && review.activity == "review")
          ? "Continue now | Acknowledge"
          : "Acknowledge",
      cuwacunu::iinuji::text_line_emphasis_t::Info,
      cuwacunu::iinuji::text_line_emphasis_t::Accent);
  push_workbench_line(out, {});
  append_workbench_detail_section(
      out, "Review Report",
      cuwacunu::iinuji::text_line_emphasis_t::Success);
  if (!st.workbench.detail_loaded ||
      st.workbench.detail_kind != workbench_detail_kind_t::Review ||
      st.workbench.detail_session_id != review.marshal_session_id) {
    append_workbench_detail_text(out, "<loading review report...>",
                                 cuwacunu::iinuji::text_line_emphasis_t::Debug);
  } else if (!st.workbench.detail_error.empty()) {
    append_workbench_detail_text(
        out, "<failed to read review report: " + st.workbench.detail_error +
                 ">",
        cuwacunu::iinuji::text_line_emphasis_t::Error);
  } else {
    append_workbench_detail_text(out, st.workbench.detail_text);
  }
}

inline void append_workbench_session_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  append_workbench_detail_section(out, "Workbench Session");
  append_workbench_detail_meta(out, "focus", "worklist");
  append_workbench_common_session_detail(out, st, session);
  push_workbench_line(out, {});
  append_workbench_detail_section(out, "Session Detail",
                                  cuwacunu::iinuji::text_line_emphasis_t::Debug);
  append_workbench_detail_text(out, session.status_detail.empty()
                                        ? std::string("<none>")
                                        : session.status_detail);
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_workbench_navigation_styled_lines(const CmdState &st) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out;
  push_workbench_line(
      out, workbench_navigation_row_text(st, kWorkbenchQueueSection),
      st.workbench.section == kWorkbenchQueueSection
          ? workbench_selected_emphasis(
                workbench_is_navigation_focus(st.workbench.focus))
          : cuwacunu::iinuji::text_line_emphasis_t::None);
  push_workbench_line(
      out, workbench_navigation_row_text(st, kWorkbenchBoosterSection),
      st.workbench.section == kWorkbenchBoosterSection
          ? workbench_selected_emphasis(
                workbench_is_navigation_focus(st.workbench.focus))
          : (workbench_booster_action_count() == 0
                 ? cuwacunu::iinuji::text_line_emphasis_t::MutedWarning
                 : cuwacunu::iinuji::text_line_emphasis_t::None));
  push_workbench_line(out, {}, cuwacunu::iinuji::text_line_emphasis_t::None);
  push_workbench_line(out,
                  "operator=" + workbench_operator_label(st) +
                      " | focus=" + workbench_focus_label(st.workbench.focus),
                  cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_workbench_line(
      out,
      "requests=" +
          std::to_string(count_pending_request_workbench_sessions(st)) +
          " | reviews=" +
          std::to_string(count_pending_review_workbench_sessions(st)) +
          " | paused=" +
          std::to_string(count_operator_paused_workbench_sessions(st)),
      cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_workbench_line(out, "keys: Enter focuses worklist | a acts on selected item",
                  cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_workbench_line(out, "      Up/Down choose inbox or booster | Esc back to header",
                  cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_workbench_line(out, "      PgUp/PgDn/Home/End scroll focused panel",
                  cuwacunu::iinuji::text_line_emphasis_t::Info);
  return out;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_workbench_worklist_styled_lines(const CmdState &st) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out;
  if (workbench_is_booster_section(st.workbench.section)) {
    const auto selected = std::min(st.workbench.selected_booster_action,
                                   workbench_booster_action_count() - 1u);
    for (std::size_t i = 0; i < workbench_booster_action_count(); ++i) {
      const auto action = workbench_booster_action_from_index(i);
      const bool active = (i == selected);
      const auto emphasis = cuwacunu::iinuji::text_line_emphasis_t::Info;
      push_workbench_line(
          out, workbench_booster_row_text(action, active),
          active ? workbench_selected_emphasis(
                       workbench_is_worklist_focus(st.workbench.focus))
                 : emphasis);
    }
    return out;
  }
  const auto rows = workbench_rows(st);
  if (rows.empty()) {
    push_workbench_line(out, "Workbench is clear.");
    return out;
  }
  const std::size_t selected =
      std::min(st.workbench.selected_session_row, rows.size() - 1);
  for (std::size_t i = 0; i < rows.size(); ++i) {
    const auto &session = rows[i];
    const bool active = (i == selected);
    const auto emphasis =
        workbench_row_emphasis(workbench_session_row_emphasis(st, session), active,
                           workbench_is_worklist_focus(st.workbench.focus));
    push_workbench_line(out, workbench_worklist_row_text(st, session, i, rows.size(),
                                                 active, emphasis));
  }
  return out;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_workbench_left_styled_lines(const CmdState &st) {
  auto out = make_workbench_navigation_styled_lines(st);
  push_workbench_line(out, {});
  const auto worklist = make_workbench_worklist_styled_lines(st);
  out.insert(out.end(), worklist.begin(), worklist.end());
  return out;
}

inline std::string workbench_styled_lines_to_text(
    const std::vector<cuwacunu::iinuji::styled_text_line_t> &lines) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i > 0)
      oss << '\n';
    oss << cuwacunu::iinuji::styled_text_line_text(lines[i]);
  }
  return oss.str();
}

inline std::string make_workbench_left(const CmdState &st) {
  return workbench_styled_lines_to_text(make_workbench_left_styled_lines(st));
}

inline void append_workbench_booster_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    workbench_booster_action_kind_t action) {
  append_workbench_detail_section(out, "Booster",
                                  cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_workbench_detail_meta(out, "action",
                               workbench_booster_action_label(action));
  append_workbench_detail_meta(out, "safety",
                               workbench_booster_action_is_danger(action)
                                   ? "danger"
                                   : "guided");
  append_workbench_detail_text(out, workbench_booster_action_summary(action),
                               cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_workbench_line(out, {});

  if (action == workbench_booster_action_kind_t::LaunchMarshal) {
    if (st.workbench.marshal_launch.active &&
        st.workbench.marshal_launch.launch_pending) {
      append_workbench_detail_section(
          out, "Starting Marshal",
          cuwacunu::iinuji::text_line_emphasis_t::Accent);
      append_workbench_detail_meta(out, "objective",
                                   st.workbench.marshal_launch.objective_name);
      append_workbench_detail_meta(
          out, "model",
          workbench_marshal_model_label(
              st, st.workbench.marshal_launch.marshal_model));
      append_workbench_detail_meta(
          out, "model_source",
          st.workbench.marshal_launch.marshal_model.empty()
              ? "objective/default"
              : "launch override");
      append_workbench_detail_meta(
          out, "reasoning",
          workbench_marshal_reasoning_label(
              st, st.workbench.marshal_launch.marshal_reasoning_effort));
      append_workbench_detail_meta(
          out, "reasoning_source",
          st.workbench.marshal_launch.marshal_reasoning_effort.empty()
              ? "objective/default"
              : "launch override");
      append_workbench_detail_meta(
          out, "objective_dsl",
          trim_to_width(st.workbench.marshal_launch.objective_dsl_path, 60));
      append_workbench_detail_text(
          out,
          st.workbench.marshal_launch.launch_status.empty()
              ? "Marshal Hero is starting this session now."
              : st.workbench.marshal_launch.launch_status,
          cuwacunu::iinuji::text_line_emphasis_t::Info);
      append_workbench_detail_text(
          out,
          "Please wait a moment. Workbench will switch to Runtime when the "
          "session becomes available.",
          cuwacunu::iinuji::text_line_emphasis_t::Debug);
      return;
    }
    const auto objective_paths = workbench_available_objective_dsl_paths(st);
    append_workbench_detail_meta(
        out, "objective_dsl_files",
        std::to_string(objective_paths.size()) + " discovered");
    append_workbench_detail_meta(
        out, "template_objective_dsl",
        trim_to_width(workbench_preferred_default_objective_dsl_path(st), 60));
    append_workbench_detail_meta(
        out, "objective_roots",
        std::to_string(workbench_objective_roots(st).size()) + " configured");
    append_workbench_detail_text(
        out,
        "Flow: choose an existing objective bundle from objective roots, or "
        "create a new objective bundle from defaults under an objective root "
        "and open it in the editor. Existing objectives are listed by name, "
        "and Booster disables any objective that is already owned by another "
        "active or resumable session. Launch prompts then offer optional model "
        "and reasoning overrides; without overrides, the Marshal objective DSL "
        "or default Marshal DSL decides.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }

  if (action == workbench_booster_action_kind_t::LaunchCampaign) {
    const auto campaign_paths = workbench_available_campaign_paths(st);
    append_workbench_detail_meta(
        out, "campaign_dsl_files",
        std::to_string(campaign_paths.size()) + " discovered");
    append_workbench_detail_meta(
        out, "default_campaign_dsl",
        trim_to_width(workbench_preferred_default_campaign_path(st), 60));
    if (const auto selected_session = selected_operator_session_record(st);
        selected_session.has_value()) {
      append_workbench_detail_meta(
          out, "selected_session",
          trim_to_width(selected_session->marshal_session_id, 40));
    }
    append_workbench_detail_text(
        out,
        "Flow: choose default, choose existing campaign DSL, or create a temp "
        "copy from default. New temp campaigns open directly in the editor "
        "before launch. Existing campaigns can parse declared BIND ids and "
        "optionally link the new campaign to the selected session.",
        cuwacunu::iinuji::text_line_emphasis_t::Debug);
    return;
  }

  append_workbench_detail_meta(
      out, "warning",
      "Type the destructive confirmation phrase exactly before reset.");
  append_workbench_detail_text(
      out,
      "This uses hero.config.dev_nuke_reset to clear runtime dump folders and "
      "reload Config Hero state. Use only when you intentionally want a clean "
      "developer reset.",
      cuwacunu::iinuji::text_line_emphasis_t::Info);
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_workbench_right_styled_lines(const CmdState &st) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out;
  if (workbench_is_booster_section(st.workbench.section)) {
    append_workbench_booster_detail(out, st, selected_workbench_booster_action(st));
    return out;
  }
  const auto selected = selected_visible_workbench_session_record(st);
  if (!selected.has_value()) {
    push_workbench_line(out, "Workbench is clear.",
                        cuwacunu::iinuji::text_line_emphasis_t::Info);
    return out;
  }
  if (const auto request = selected_workbench_request_record(st);
      request.has_value()) {
    append_workbench_request_detail(out, st, *request);
    return out;
  }
  if (const auto review = selected_workbench_review_record(st);
      review.has_value()) {
    append_workbench_review_detail(out, st, *review);
    return out;
  }
  append_workbench_session_detail(out, st, *selected);
  return out;
}

inline std::string make_workbench_right(const CmdState &st) {
  return workbench_styled_lines_to_text(make_workbench_right_styled_lines(st));
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
