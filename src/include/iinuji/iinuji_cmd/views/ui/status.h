#pragma once

#include <sstream>
#include <string>
#include <vector>

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool ui_workbench_has_items(const CmdState &st) {
  return count_pending_operator_requests(st) > 0 ||
         count_pending_operator_reviews(st) > 0 ||
         count_operator_paused_sessions(st) > 0;
}

inline std::string ui_status_screen_badge(const CmdState &st,
                                          ScreenMode screen) {
  std::string badge = ui_screen_tab_text(screen);
  if (screen == ScreenMode::Workbench && ui_workbench_has_items(st)) {
    badge += "*";
  }
  return st.screen == screen ? "[" + badge + "]" : " " + badge + " ";
}

inline std::vector<cuwacunu::iinuji::styled_text_segment_t>
ui_status_segments(const CmdState &st) {
  using cuwacunu::iinuji::styled_text_segment_t;
  using cuwacunu::iinuji::text_line_emphasis_t;

  std::vector<styled_text_segment_t> segments{};
  segments.reserve(6);
  for (const auto screen : {ScreenMode::Home, ScreenMode::Workbench,
                            ScreenMode::Runtime, ScreenMode::Lattice,
                            ScreenMode::ShellLogs, ScreenMode::Config}) {
    if (!segments.empty()) {
      segments.push_back(styled_text_segment_t{
          .text = " ",
          .emphasis = text_line_emphasis_t::None,
      });
    }
    segments.push_back(styled_text_segment_t{
        .text = ui_status_screen_badge(st, screen),
        .emphasis = text_line_emphasis_t::None,
    });
  }
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
