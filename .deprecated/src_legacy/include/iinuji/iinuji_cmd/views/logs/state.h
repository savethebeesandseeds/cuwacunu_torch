#pragma once

#include <cstddef>
#include <cstdint>

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class LogsLevelFilter : std::uint8_t {
  DebugOrHigher = 0,
  InfoOrHigher = 1,
  WarningOrHigher = 2,
  ErrorOrHigher = 3,
  FatalOnly = 4,
};

enum class LogsMetadataFilter : std::uint8_t {
  Any = 0,
  WithAnyMetadata = 1,
  WithFunction = 2,
  WithPath = 3,
  WithCallsite = 4,
};

inline constexpr std::size_t logs_settings_count() {
  return 8;
}

struct ShellLogsState {
  bool auto_follow{true};
  bool mouse_capture{true};
  LogsLevelFilter level_filter{LogsLevelFilter::InfoOrHigher};
  LogsMetadataFilter metadata_filter{LogsMetadataFilter::Any};
  bool show_date{false};
  bool show_thread{false};
  bool show_metadata{true};
  bool show_color{true};
  std::size_t selected_setting{0};
  int pending_scroll_y{0};
  int pending_scroll_x{0};
  bool pending_jump_home{false};
  bool pending_jump_end{false};
};

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
