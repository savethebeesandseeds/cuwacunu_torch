#pragma once

#include <cctype>
#include <cstdint>
#include <map>

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {
inline std::string to_upper_copy(std::string s) {
  for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  return s;
}

inline int logs_level_rank(const std::string& level) {
  const std::string u = to_upper_copy(level);
  if (u.find("FATAL") != std::string::npos || u.find("TERMINATION") != std::string::npos) return 50;
  if (u.find("ERROR") != std::string::npos || u.find("ERRNO") != std::string::npos) return 40;
  if (u.find("WARN") != std::string::npos) return 30;
  if (u.find("INFO") != std::string::npos) return 20;
  if (u.find("DEBUG") != std::string::npos) return 10;
  return 20;
}

inline int logs_filter_min_rank(LogsLevelFilter f) {
  switch (f) {
    case LogsLevelFilter::DebugOrHigher:
      return 10;
    case LogsLevelFilter::InfoOrHigher:
      return 20;
    case LogsLevelFilter::WarningOrHigher:
      return 30;
    case LogsLevelFilter::ErrorOrHigher:
      return 40;
    case LogsLevelFilter::FatalOnly:
      return 50;
  }
  return 10;
}

inline std::string logs_filter_label(LogsLevelFilter f) {
  switch (f) {
    case LogsLevelFilter::DebugOrHigher:
      return "DEBUG+";
    case LogsLevelFilter::InfoOrHigher:
      return "INFO+";
    case LogsLevelFilter::WarningOrHigher:
      return "WARNING+";
    case LogsLevelFilter::ErrorOrHigher:
      return "ERROR+";
    case LogsLevelFilter::FatalOnly:
      return "FATAL";
  }
  return "DEBUG+";
}

inline bool logs_accept_entry(const LogsState& settings,
                              const cuwacunu::piaabo::dlog_entry_t& e) {
  return logs_level_rank(e.level) >= logs_filter_min_rank(settings.level_filter);
}

inline char logs_line_marker(const LogsState& settings,
                             const cuwacunu::piaabo::dlog_entry_t& e) {
  if (!settings.show_color) return '\0';
  const int rank = logs_level_rank(e.level);
  if (rank >= 50) return '\x19';
  if (rank >= 40) return '\x1e';
  if (rank >= 30) return '\x1d';
  if (rank >= 20) return '\x1a';
  return '\x1c';
}

inline std::string format_logs_entry(const LogsState& settings,
                                     const cuwacunu::piaabo::dlog_entry_t& e) {
  std::ostringstream oss;
  const bool color = settings.show_color;

  if (settings.show_date) {
    if (color) oss << '\x11';
    oss << "[" << e.timestamp << "]";
    if (color) oss << '\x12';
    oss << " ";
  }
  oss << "[" << e.level << "] ";
  if (settings.show_thread) {
    if (color) oss << '\x13';
    oss << "[0x" << e.thread << "]";
    if (color) oss << '\x14';
    oss << " ";
  }
  oss << e.message;
  std::string line = oss.str();
  const char marker = logs_line_marker(settings, e);
  if (marker != '\0') {
    line.insert(line.begin(), marker);
  }
  return line;
}

inline std::string make_logs_left(const LogsState& settings,
                                  const std::vector<cuwacunu::piaabo::dlog_entry_t>& snap) {
  std::ostringstream oss;
  std::size_t shown = 0;
  for (const auto& e : snap) {
    if (logs_accept_entry(settings, e)) ++shown;
  }

  oss << "# dlogs buffer\n";
  oss << "# entries=" << snap.size()
      << " shown=" << shown
      << " capacity=" << cuwacunu::piaabo::dlog_buffer_capacity() << "\n";
  oss << "# level=" << logs_filter_label(settings.level_filter)
      << " date=" << (settings.show_date ? "on" : "off")
      << " thread=" << (settings.show_thread ? "on" : "off")
      << " color=" << (settings.show_color ? "on" : "off")
      << " follow=" << (settings.auto_follow ? "on" : "off") << "\n";
  oss << "# newest entries at bottom\n\n";
  if (shown == 0) {
    oss << "(no logs)\n";
    return oss.str();
  }
  for (const auto& e : snap) {
    if (!logs_accept_entry(settings, e)) continue;
    oss << format_logs_entry(settings, e) << "\n";
  }
  return oss.str();
}

inline std::string make_logs_right(const LogsState& settings,
                                   const std::vector<cuwacunu::piaabo::dlog_entry_t>& snap) {
  std::map<std::string, std::size_t> level_counts;
  std::size_t shown = 0;
  for (const auto& e : snap) {
    ++level_counts[e.level];
    if (logs_accept_entry(settings, e)) ++shown;
  }

  auto setting_line = [&](std::size_t idx, const std::string& text) {
    const std::string line = "  " + text;
    if (idx == settings.selected_setting) return mark_selected_line(line);
    return line;
  };

  std::ostringstream oss;
  oss << "Logs view\n";
  oss << "  source: piaabo/dlogs.h buffer\n";
  oss << "  entries: " << shown << " shown / " << snap.size() << " total"
      << " / " << cuwacunu::piaabo::dlog_buffer_capacity() << "\n";
  if (shown > 0) {
    std::uint64_t first_seq = 0;
    std::uint64_t last_seq = 0;
    std::string first_ts;
    std::string last_ts;
    bool have = false;
    for (const auto& e : snap) {
      if (!logs_accept_entry(settings, e)) continue;
      if (!have) {
        have = true;
        first_seq = e.seq;
        first_ts = e.timestamp;
      }
      last_seq = e.seq;
      last_ts = e.timestamp;
    }
    if (have) {
      oss << "  seq: " << first_seq << " .. " << last_seq << "\n";
      oss << "  first: " << first_ts << "\n";
      oss << "  last : " << last_ts << "\n";
    }
  }
  oss << "\nSettings (Up/Down select, Left/Right change)\n";
  oss << setting_line(0, "log level : " + logs_filter_label(settings.level_filter)) << "\n";
  oss << setting_line(1, std::string("show date : ") + (settings.show_date ? "on" : "off")) << "\n";
  oss << setting_line(2, std::string("show thread id : ") + (settings.show_thread ? "on" : "off")) << "\n";
  oss << setting_line(3, std::string("show color : ") + (settings.show_color ? "on" : "off")) << "\n";
  oss << setting_line(4, std::string("auto follow : ") + (settings.auto_follow ? "on" : "off")) << "\n";
  oss << setting_line(5, std::string("mouse capture : ") + (settings.mouse_capture ? "on" : "off (select/copy)"))
      << "\n";
  oss << "\nLevels\n";
  if (level_counts.empty()) {
    oss << "  (none)\n";
  } else {
    for (const auto& kv : level_counts) {
      oss << "  " << kv.first << " : " << kv.second << "\n";
    }
  }
  oss << "\nCommands\n";
  oss << "  iinuji.screen.logs()\n";
  oss << "  iinuji.show.logs()\n";
  oss << "  iinuji.logs.clear()\n";
  oss << "  aliases: logs, f8, logs clear\n";
  oss << "  primitive translation: disabled\n";
  oss << "\nKeys\n";
  oss << "  F8 : open logs screen\n";
  oss << "  [^] top-right click or Home : jump to oldest\n";
  oss << "  [v] bottom-right click or End : jump to newest\n";
  oss << "  wheel : vertical scroll both panels\n";
  oss << "  Shift/Ctrl/Alt+wheel : horizontal scroll both panels\n";
  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
