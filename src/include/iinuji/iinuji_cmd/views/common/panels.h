#pragma once

#include <chrono>
#include <cstdint>
#include <ctime>
#include <iomanip>
#include <initializer_list>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "iinuji/iinuji_cmd/views/common/base.h"
#include "iinuji/iinuji_cmd/views/common/labels.h"
#include "iinuji/iinuji_cmd/views/common/viewport.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

struct ui_text_panel_model_t {
  std::vector<cuwacunu::iinuji::styled_text_line_t> lines{};
  std::optional<std::size_t> selected_line{};
  bool reset_horizontal_scroll{false};
  std::string title{};
  bool focused{false};
};

inline void clear_panel_selection_tracking(
    std::initializer_list<
        std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>> panels) {
  for (const auto &panel : panels)
    clear_selected_text_row_tracking(panel);
}

inline void show_single_left_panel(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left_nav_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
        &left_worklist_panel) {
  left->visible = true;
  left_nav_panel->visible = false;
  left_worklist_panel->visible = false;
}

inline void show_split_left_panels(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &left_nav_panel,
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t>
        &left_worklist_panel) {
  left->visible = true;
  for (const auto &candidate : left->children) {
    if (candidate && candidate->id == "left_main") {
      candidate->visible = false;
    }
  }
  left_nav_panel->visible = true;
  left_worklist_panel->visible = true;
}

inline void apply_text_panel_model(
    const std::shared_ptr<cuwacunu::iinuji::iinuji_object_t> &panel,
    const ui_text_panel_model_t &model) {
  set_text_box_styled_lines(panel, model.lines, false);
  panel->style.title = ui_focusable_panel_title(model.title, model.focused);
  sync_plain_text_panel_selection(panel, model.selected_line,
                                  model.lines.size(),
                                  model.reset_horizontal_scroll);
}

inline std::string panel_rule(char fill = '=', std::size_t width = 70) {
  return std::string(width, fill);
}

inline void append_panel_title(std::ostringstream &oss, std::string_view title,
                               char fill = '=') {
  const std::string rule = panel_rule(fill);
  oss << rule << "\n" << title << "\n" << rule << "\n";
}

inline void append_meta_line(std::ostringstream &oss, std::string_view key,
                             std::string value) {
  if (value.empty())
    value = "<none>";
  oss << key << ": " << value << "\n";
}

inline std::string join_trimmed_values(const std::vector<std::string> &values,
                                       std::size_t max_items = 4,
                                       std::size_t max_width = 60) {
  if (values.empty())
    return "<none>";
  std::ostringstream oss;
  const std::size_t limit = std::min(values.size(), max_items);
  for (std::size_t i = 0; i < limit; ++i) {
    if (i != 0)
      oss << ", ";
    oss << trim_to_width(values[i], static_cast<int>(max_width));
  }
  if (values.size() > limit) {
    oss << " +" << (values.size() - limit) << " more";
  }
  return oss.str();
}

inline std::string format_compact_duration_ms(std::uint64_t duration_ms) {
  const std::uint64_t total_seconds = duration_ms / 1000;
  const std::uint64_t days = total_seconds / 86400;
  const std::uint64_t hours = (total_seconds % 86400) / 3600;
  const std::uint64_t minutes = (total_seconds % 3600) / 60;
  const std::uint64_t seconds = total_seconds % 60;

  std::ostringstream oss;
  if (days > 0)
    oss << days << "d";
  if (hours > 0)
    oss << hours << "h";
  if (minutes > 0)
    oss << minutes << "m";
  if (days == 0 && seconds > 0)
    oss << seconds << "s";
  if (oss.str().empty()) {
    if (duration_ms > 0) {
      oss << duration_ms << "ms";
    } else {
      oss << "0s";
    }
  }
  return oss.str();
}

// Render operator-facing timestamps in the process local timezone so `TZ`
// or the system timezone controls display without changing stored UTC epochs.
inline std::string format_timestamp_ms_local(std::uint64_t timestamp_ms) {
  if (timestamp_ms == 0)
    return "<unknown>";
  const std::time_t timestamp_s =
      static_cast<std::time_t>(timestamp_ms / 1000);
  std::tm tm{};
#if defined(_POSIX_VERSION)
  if (::localtime_r(&timestamp_s, &tm) == nullptr)
    return "<invalid>";
#else
  const std::tm *raw_tm = std::localtime(&timestamp_s);
  if (raw_tm == nullptr)
    return "<invalid>";
  tm = *raw_tm;
#endif
  std::ostringstream oss;
  oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S %Z %z");
  return oss.str();
}

inline std::string format_age_since_ms(std::uint64_t timestamp_ms) {
  if (timestamp_ms == 0)
    return "<unknown>";
  const std::uint64_t now_ms = static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
  if (timestamp_ms >= now_ms)
    return "0s ago";
  return format_compact_duration_ms(now_ms - timestamp_ms) + " ago";
}

inline std::string format_time_marker_ms(std::uint64_t timestamp_ms) {
  if (timestamp_ms == 0)
    return "<unknown>";
  return format_timestamp_ms_local(timestamp_ms) + " | " +
         format_age_since_ms(timestamp_ms);
}

inline std::string format_optional_time_marker_ms(
    const std::optional<std::uint64_t> &timestamp_ms) {
  if (!timestamp_ms.has_value())
    return "<none>";
  return format_time_marker_ms(*timestamp_ms);
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
