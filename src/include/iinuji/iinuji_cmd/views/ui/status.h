#pragma once

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool ui_inbox_has_items(const CmdState &st) {
  if (!st.inbox.operator_inbox.actionable_requests.empty() ||
      !st.inbox.operator_inbox.unacknowledged_summaries.empty()) {
    return true;
  }
  return std::any_of(
      st.inbox.operator_inbox.all_sessions.begin(),
      st.inbox.operator_inbox.all_sessions.end(), [](const auto &session) {
        return session.phase == "paused" && session.pause_kind == "operator";
      });
}

inline std::string ui_status_inbox_label(const CmdState &st) {
  const bool has_inbox_items = ui_inbox_has_items(st);
  if (st.screen == ScreenMode::Inbox) {
    return has_inbox_items ? "[F2 INBOX*]" : "[F2 INBOX]";
  }
  return has_inbox_items ? " F2 INBOX* " : " F2 INBOX ";
}

inline std::string ui_status_screen_label(const CmdState &st, ScreenMode screen,
                                          const char *label) {
  return st.screen == screen ? "[" + std::string(label) + "]"
                             : " " + std::string(label) + " ";
}

inline std::vector<cuwacunu::iinuji::styled_text_segment_t>
ui_status_segments(const CmdState &st) {
  using cuwacunu::iinuji::styled_text_segment_t;
  using cuwacunu::iinuji::text_line_emphasis_t;

  const std::string prefix =
      ui_status_screen_label(st, ScreenMode::Home, "F1 HOME") + " ";
  const std::string suffix =
      " " + ui_status_screen_label(st, ScreenMode::Runtime, "F3 RUNTIME") +
      " " + ui_status_screen_label(st, ScreenMode::Lattice, "F4 LATTICE") +
      " " + ui_status_screen_label(st, ScreenMode::ShellLogs, "F8 SHELL LOGS") +
      " " + ui_status_screen_label(st, ScreenMode::Config, "F9 CONFIG");
  std::vector<styled_text_segment_t> segments{};
  segments.reserve(3);
  segments.push_back(styled_text_segment_t{
      .text = prefix,
      .emphasis = text_line_emphasis_t::None,
  });
  segments.push_back(styled_text_segment_t{
      .text = ui_status_inbox_label(st),
      .emphasis = text_line_emphasis_t::None,
  });
  segments.push_back(styled_text_segment_t{
      .text = suffix,
      .emphasis = text_line_emphasis_t::None,
  });
  return segments;
}

inline std::vector<cuwacunu::iinuji::styled_text_line_t>
ui_status_styled_lines(const CmdState &st) {
  using cuwacunu::iinuji::text_line_emphasis_t;
  return {cuwacunu::iinuji::make_segmented_styled_text_line(
      ui_status_segments(st), text_line_emphasis_t::None)};
}

inline std::string ui_status_line(const CmdState &st) {
  const auto lines = ui_status_styled_lines(st);
  std::ostringstream oss;
  for (std::size_t i = 0; i < lines.size(); ++i) {
    if (i > 0) {
      oss << '\n';
    }
    oss << cuwacunu::iinuji::styled_text_line_text(lines[i]);
  }
  return oss.str();
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
