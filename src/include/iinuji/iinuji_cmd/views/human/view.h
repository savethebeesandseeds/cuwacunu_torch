#pragma once

#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "iinuji/iinuji_cmd/views/human/commands.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool human_has_request_for_session(const CmdState &st,
                                          std::string_view marshal_session_id) {
  return human_session_has_pending_request(st, marshal_session_id);
}

inline bool human_has_review_for_session(const CmdState &st,
                                         std::string_view marshal_session_id) {
  return human_session_has_pending_review(st, marshal_session_id);
}

inline std::string human_phase_text(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return cuwacunu::hero::human_mcp::operator_session_state_detail(session);
}

inline std::string human_current_lane_label(const CmdState &st) {
  return human_view_label(st.human.view);
}

inline std::string human_view_count_text(const CmdState &st,
                                         human_console_view_t view) {
  (void)view;
  std::ostringstream oss;
  oss << count_inbox_human_sessions(st) << " waiting";
  return oss.str();
}

inline cuwacunu::iinuji::text_line_emphasis_t
human_selected_emphasis(bool focused) {
  return focused ? cuwacunu::iinuji::text_line_emphasis_t::Accent
                 : cuwacunu::iinuji::text_line_emphasis_t::Debug;
}

inline void
push_human_line(std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
                std::string text,
                cuwacunu::iinuji::text_line_emphasis_t emphasis =
                    cuwacunu::iinuji::text_line_emphasis_t::None) {
  out.push_back(cuwacunu::iinuji::styled_text_line_t{
      .text = std::move(text),
      .emphasis = emphasis,
  });
}

inline std::string human_menu_row_text(const CmdState &st,
                                       human_console_view_t view) {
  std::ostringstream oss;
  oss << (st.human.view == view ? "> " : "  ");
  oss << "[" << human_view_label(view) << "] "
      << human_view_count_text(st, view);
  return oss.str();
}

inline std::string human_inbox_section_title(const CmdState &st) {
  return "Worklist [" + human_current_lane_label(st) + "]";
}

inline std::string human_operator_label(const CmdState &st) {
  return st.human.app.defaults.operator_id.empty()
             ? std::string("<unset>")
             : st.human.app.defaults.operator_id;
}

inline std::string human_lifecycle_badge(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (session.phase == "idle")
    return "[IDLE]";
  if (session.phase == "finished") {
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

inline cuwacunu::iinuji::text_line_emphasis_t human_inbox_row_emphasis(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (human_has_request_for_session(st, session.marshal_session_id)) {
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (human_session_is_operator_paused(session)) {
    return cuwacunu::iinuji::text_line_emphasis_t::Info;
  }
  if (human_has_review_for_session(st, session.marshal_session_id)) {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::None;
}

inline cuwacunu::iinuji::text_line_emphasis_t
human_row_emphasis(cuwacunu::iinuji::text_line_emphasis_t base, bool active,
                   bool focused) {
  if (active) {
    (void)base;
    (void)focused;
    return cuwacunu::iinuji::text_line_emphasis_t::Accent;
  }
  return base;
}

inline std::string human_worklist_objective(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return session.objective_name.empty() ? std::string("<unnamed objective>")
                                        : session.objective_name;
}

inline std::string human_middle_compact(std::string value,
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

inline std::string human_worklist_session_id(
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  std::string marshal_session_id = session.marshal_session_id.empty()
                                       ? std::string("<none>")
                                       : session.marshal_session_id;
  if (marshal_session_id.rfind("marshal.", 0) == 0) {
    marshal_session_id.erase(0, std::string("marshal.").size());
  }
  return human_middle_compact(std::move(marshal_session_id), 16);
}

inline std::string human_worklist_extra_info(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (human_has_request_for_session(st, session.marshal_session_id)) {
    return "request=" +
           trim_to_width(
               std::string(cuwacunu::hero::human_mcp::human_request_kind_label(
                   session.pause_kind)),
               14);
  }
  if (human_session_is_operator_paused(session)) {
    return "pause=operator";
  }
  if (human_has_review_for_session(st, session.marshal_session_id)) {
    if (!session.finish_reason.empty() && session.finish_reason != "none") {
      return "review=" + trim_to_width(session.finish_reason, 14);
    }
    return "review=pending";
  }
  return "state=" + trim_to_width(session.phase.empty() ? std::string("inbox")
                                                        : session.phase,
                                  14);
}

inline std::string human_next_action_text(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  return cuwacunu::hero::human_mcp::operator_session_action_hint(
      session, human_has_review_for_session(st, session.marshal_session_id));
}

inline std::string
human_emphasis_ansi_prefix(cuwacunu::iinuji::text_line_emphasis_t emphasis,
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
human_ansi_segment(std::string_view text,
                   cuwacunu::iinuji::text_line_emphasis_t emphasis,
                   bool dim = false) {
  if (text.empty())
    return {};
  return human_emphasis_ansi_prefix(emphasis, dim) + std::string(text) +
         "\x1b[0m";
}

inline std::string human_pad_trim_to_width(const std::string &value,
                                           std::size_t width) {
  std::string out = trim_to_width(value, static_cast<int>(width));
  if (out.size() < width)
    out.append(width - out.size(), ' ');
  return out;
}

inline std::size_t human_worklist_index_width(std::size_t total_rows) {
  std::size_t width = 1;
  while (total_rows >= 10) {
    total_rows /= 10;
    ++width;
  }
  return std::max<std::size_t>(2, width);
}

inline std::string human_worklist_row_text(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session,
    std::size_t row_index, std::size_t total_rows, bool active,
    cuwacunu::iinuji::text_line_emphasis_t emphasis) {
  constexpr std::size_t kHumanObjectiveWidth = 28;
  constexpr std::size_t kHumanSessionWidth = 16;
  constexpr std::size_t kHumanExtraWidth = 22;

  std::ostringstream lead;
  lead << (active ? "> " : "  ") << "["
       << std::setw(static_cast<int>(human_worklist_index_width(total_rows)))
       << (row_index + 1) << "] "
       << human_pad_trim_to_width(human_worklist_objective(session),
                                  kHumanObjectiveWidth);

  std::string meta = human_pad_trim_to_width(human_worklist_session_id(session),
                                             kHumanSessionWidth);
  const std::string extra = human_worklist_extra_info(st, session);
  if (!extra.empty()) {
    meta += " | " + human_pad_trim_to_width(extra, kHumanExtraWidth);
  }

  return human_ansi_segment(lead.str(), emphasis, false) +
         human_ansi_segment(" | " + meta, emphasis, true);
}

inline std::string human_detail_rule() { return panel_rule('-', 56); }

inline void append_human_detail_section(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
    std::string_view title,
    cuwacunu::iinuji::text_line_emphasis_t emphasis =
        cuwacunu::iinuji::text_line_emphasis_t::Accent) {
  push_human_line(out, human_detail_rule(), emphasis);
  push_human_line(out, std::string(title), emphasis);
  push_human_line(out, human_detail_rule(), emphasis);
}

inline void
append_human_detail_text(std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
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
    push_human_line(out, std::string(prefix) + std::string(line), emphasis);
    if (end == std::string::npos)
      break;
    start = end + 1;
    if (start == text.size())
      break;
  }
}

inline std::string human_detail_meta_prefix(std::string_view key) {
  constexpr std::size_t kHumanDetailKeyWidth = 18;
  std::string prefix = std::string(key) + ":";
  if (prefix.size() < kHumanDetailKeyWidth) {
    prefix.append(kHumanDetailKeyWidth - prefix.size(), ' ');
  } else {
    prefix.push_back(' ');
  }
  return prefix;
}

inline void
append_human_detail_meta(std::vector<cuwacunu::iinuji::styled_text_line_t> &out,
                         std::string_view key, std::string value,
                         cuwacunu::iinuji::text_line_emphasis_t key_emphasis =
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                         cuwacunu::iinuji::text_line_emphasis_t value_emphasis =
                             cuwacunu::iinuji::text_line_emphasis_t::Debug) {
  if (value.empty())
    value = "<none>";
  const std::string prefix = human_detail_meta_prefix(key);
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
    push_human_line(out,
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
human_detail_session_state_emphasis(
    const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  if (session.finish_reason == "failed" ||
      session.finish_reason == "terminated") {
    return cuwacunu::iinuji::text_line_emphasis_t::Error;
  }
  if (human_has_request_for_session(st, session.marshal_session_id)) {
    return cuwacunu::iinuji::text_line_emphasis_t::Warning;
  }
  if (human_has_review_for_session(st, session.marshal_session_id) ||
      session.finish_reason == "success") {
    return cuwacunu::iinuji::text_line_emphasis_t::Success;
  }
  if (human_session_is_operator_paused(session) || session.phase == "idle" ||
      session.phase == "finished") {
    return cuwacunu::iinuji::text_line_emphasis_t::Info;
  }
  return cuwacunu::iinuji::text_line_emphasis_t::Debug;
}

inline void append_human_common_session_detail(
    std::vector<cuwacunu::iinuji::styled_text_line_t> &out, const CmdState &st,
    const cuwacunu::hero::marshal::marshal_session_record_t &session) {
  append_human_detail_meta(out, "operator_state",
                           human_lifecycle_badge(session) + " " +
                               human_phase_text(session),
                           cuwacunu::iinuji::text_line_emphasis_t::Info,
                           human_detail_session_state_emphasis(st, session));
  append_human_detail_meta(out, "next_action",
                           human_next_action_text(st, session),
                           cuwacunu::iinuji::text_line_emphasis_t::Info,
                           cuwacunu::iinuji::text_line_emphasis_t::Accent);
  append_human_detail_meta(out, "objective", session.objective_name);
  append_human_detail_meta(out, "marshal_session_id",
                           session.marshal_session_id);
  append_human_detail_meta(out, "authority_scope", session.authority_scope);
  append_human_detail_meta(out, "active_campaign",
                           session.active_campaign_cursor.empty()
                               ? std::string("<none>")
                               : session.active_campaign_cursor);
  append_human_detail_meta(out, "campaign_history",
                           std::to_string(session.campaign_cursors.size()) +
                               " linked campaigns");
  append_human_detail_meta(out, "started_at",
                           format_time_marker_ms(session.started_at_ms));
  append_human_detail_meta(out, "updated_at",
                           format_time_marker_ms(session.updated_at_ms));
  append_human_detail_meta(
      out, "finished_at",
      format_optional_time_marker_ms(session.finished_at_ms));
  if (!session.last_intent_kind.empty()) {
    append_human_detail_meta(out, "last_intent", session.last_intent_kind);
  }
  if (!session.last_warning.empty()) {
    append_human_detail_meta(out, "last_warning", session.last_warning,
                             cuwacunu::iinuji::text_line_emphasis_t::Info,
                             cuwacunu::iinuji::text_line_emphasis_t::Warning);
  }
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_human_navigation_styled_lines(const CmdState &st) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out;
  push_human_line(
      out, human_menu_row_text(st, kHumanInboxView),
      st.human.view == kHumanInboxView
          ? human_selected_emphasis(human_is_menu_focus(st.human.focus))
          : cuwacunu::iinuji::text_line_emphasis_t::None);

  push_human_line(out, {}, cuwacunu::iinuji::text_line_emphasis_t::None);
  push_human_line(out,
                  "operator=" + human_operator_label(st) +
                      " | focus=" + human_focus_label(st.human.focus),
                  cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_human_line(out, "keys: Enter opens inbox | a acts on selected item",
                  cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_human_line(out, "      Up/Down move | Esc back to header",
                  cuwacunu::iinuji::text_line_emphasis_t::Info);
  push_human_line(out, "      PgUp/PgDn/Home/End detail",
                  cuwacunu::iinuji::text_line_emphasis_t::Info);
  return out;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_human_worklist_styled_lines(const CmdState &st) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out;
  const auto rows = human_inbox_session_rows(st);
  if (rows.empty()) {
    push_human_line(out, "Inbox is clear.");
    return out;
  }
  const std::size_t selected =
      std::min(st.human.selected_inbox_session, rows.size() - 1);
  const std::size_t start = selected > 7 ? (selected - 7) : 0;
  const std::size_t end = std::min(rows.size(), start + 18);
  for (std::size_t i = start; i < end; ++i) {
    const auto &session = rows[i];
    const bool active = (i == selected);
    const auto emphasis =
        human_row_emphasis(human_inbox_row_emphasis(st, session), active,
                           human_is_inbox_focus(st.human.focus));
    push_human_line(out, human_worklist_row_text(st, session, i, rows.size(),
                                                 active, emphasis));
  }
  return out;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_human_left_styled_lines(const CmdState &st) {
  auto out = make_human_navigation_styled_lines(st);
  push_human_line(out, {});
  const auto worklist = make_human_worklist_styled_lines(st);
  out.insert(out.end(), worklist.begin(), worklist.end());
  return out;
}

inline std::string human_styled_lines_to_text(
    const std::vector<cuwacunu::iinuji::styled_text_line_t> &lines) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i > 0)
      oss << '\n';
    oss << cuwacunu::iinuji::styled_text_line_text(lines[i]);
  }
  return oss.str();
}

inline std::string make_human_left(const CmdState &st) {
  return human_styled_lines_to_text(make_human_left_styled_lines(st));
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
make_human_right_styled_lines(const CmdState &st) {
  std::vector<cuwacunu::iinuji::styled_text_line_t> out;
  if (human_is_inbox_view(st.human.view)) {
    const auto selected = selected_visible_inbox_human_session_record(st);
    if (!selected.has_value()) {
      push_human_line(out, "Inbox is clear.",
                      cuwacunu::iinuji::text_line_emphasis_t::Info);
      return out;
    }
    if (const auto request = selected_human_request_record(st);
        request.has_value()) {
      append_human_detail_section(
          out, "Inbox Request",
          cuwacunu::iinuji::text_line_emphasis_t::Warning);
      append_human_detail_meta(out, "focus", "worklist");
      append_human_detail_meta(out, "operator_state",
                               human_phase_text(*request),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Warning);
      append_human_detail_meta(out, "next_action",
                               human_next_action_text(st, *request),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Accent);
      append_human_detail_meta(
          out, "request_kind",
          std::string(cuwacunu::hero::human_mcp::human_request_kind_label(
              request->pause_kind)),
          cuwacunu::iinuji::text_line_emphasis_t::Info,
          cuwacunu::iinuji::text_line_emphasis_t::Warning);
      append_human_detail_meta(out, "objective", request->objective_name);
      append_human_detail_meta(out, "marshal_session_id",
                               request->marshal_session_id);
      append_human_detail_meta(out, "checkpoint",
                               std::to_string(request->checkpoint_count));
      append_human_detail_meta(out, "started_at",
                               format_time_marker_ms(request->started_at_ms));
      append_human_detail_meta(out, "updated_at",
                               format_time_marker_ms(request->updated_at_ms));
      append_human_detail_meta(out, "active_campaign",
                               request->active_campaign_cursor.empty()
                                   ? std::string("<none>")
                                   : request->active_campaign_cursor);
      push_human_line(out, {});
      append_human_detail_section(
          out, "Request Body", cuwacunu::iinuji::text_line_emphasis_t::Warning);
      if (!st.human.detail_loaded ||
          st.human.detail_kind != human_detail_kind_t::InboxRequest ||
          st.human.detail_session_id != request->marshal_session_id) {
        append_human_detail_text(out, "<loading request body...>",
                                 cuwacunu::iinuji::text_line_emphasis_t::Debug);
      } else if (!st.human.detail_error.empty()) {
        append_human_detail_text(
            out, "<failed to read request: " + st.human.detail_error + ">",
            cuwacunu::iinuji::text_line_emphasis_t::Error);
      } else {
        append_human_detail_text(out, st.human.detail_text);
      }
      return out;
    }
    if (const auto review = selected_human_review_record(st);
        review.has_value()) {
      append_human_detail_section(
          out, "Inbox Review", cuwacunu::iinuji::text_line_emphasis_t::Success);
      append_human_detail_meta(out, "focus", "worklist");
      append_human_detail_meta(out, "operator_state", human_phase_text(*review),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Success);
      append_human_detail_meta(out, "next_action",
                               human_next_action_text(st, *review),
                               cuwacunu::iinuji::text_line_emphasis_t::Info,
                               cuwacunu::iinuji::text_line_emphasis_t::Accent);
      append_human_detail_meta(out, "objective", review->objective_name);
      append_human_detail_meta(out, "marshal_session_id",
                               review->marshal_session_id);
      append_human_detail_meta(
          out, "effort",
          cuwacunu::hero::human_mcp::build_summary_effort_text(*review),
          cuwacunu::iinuji::text_line_emphasis_t::Info,
          cuwacunu::iinuji::text_line_emphasis_t::Success);
      append_human_detail_meta(
          out, "actions",
          review->phase == "idle" ? "Continue | Acknowledge" : "Acknowledge",
          cuwacunu::iinuji::text_line_emphasis_t::Info,
          cuwacunu::iinuji::text_line_emphasis_t::Accent);
      push_human_line(out, {});
      append_human_detail_section(
          out, "Review Report",
          cuwacunu::iinuji::text_line_emphasis_t::Success);
      if (!st.human.detail_loaded ||
          st.human.detail_kind != human_detail_kind_t::InboxReview ||
          st.human.detail_session_id != review->marshal_session_id) {
        append_human_detail_text(out, "<loading review report...>",
                                 cuwacunu::iinuji::text_line_emphasis_t::Debug);
      } else if (!st.human.detail_error.empty()) {
        append_human_detail_text(
            out,
            "<failed to read review report: " + st.human.detail_error + ">",
            cuwacunu::iinuji::text_line_emphasis_t::Error);
      } else {
        append_human_detail_text(out, st.human.detail_text);
      }
      return out;
    }

    append_human_detail_section(out, "Inbox Session");
    append_human_detail_meta(out, "focus", "worklist");
    append_human_common_session_detail(out, st, *selected);
    push_human_line(out, {});
    append_human_detail_section(out, "Session Detail",
                                cuwacunu::iinuji::text_line_emphasis_t::Debug);
    append_human_detail_text(out, selected->phase_detail.empty()
                                      ? std::string("<none>")
                                      : selected->phase_detail);
    return out;
  }

  return out;
}

inline std::string make_human_right(const CmdState &st) {
  return human_styled_lines_to_text(make_human_right_styled_lines(st));
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
