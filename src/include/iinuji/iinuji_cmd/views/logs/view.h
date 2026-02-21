#pragma once

#include <map>

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {
inline std::string make_logs_left(const std::vector<cuwacunu::piaabo::dlog_entry_t>& snap) {
  std::ostringstream oss;
  oss << "# dlogs buffer\n";
  oss << "# entries=" << snap.size()
      << " capacity=" << cuwacunu::piaabo::dlog_buffer_capacity() << "\n";
  oss << "# newest entries at bottom\n\n";
  if (snap.empty()) {
    oss << "(no logs)\n";
    return oss.str();
  }
  for (const auto& e : snap) {
    oss << cuwacunu::piaabo::dlog_format_entry(e) << "\n";
  }
  return oss.str();
}

inline std::string make_logs_right(const std::vector<cuwacunu::piaabo::dlog_entry_t>& snap) {
  std::map<std::string, std::size_t> level_counts;
  for (const auto& e : snap) ++level_counts[e.level];

  std::ostringstream oss;
  oss << "Logs view\n";
  oss << "  source: piaabo/dlogs.h buffer\n";
  oss << "  entries: " << snap.size()
      << " / " << cuwacunu::piaabo::dlog_buffer_capacity() << "\n";
  if (!snap.empty()) {
    oss << "  seq: " << snap.front().seq << " .. " << snap.back().seq << "\n";
    oss << "  first: " << snap.front().timestamp << "\n";
    oss << "  last : " << snap.back().timestamp << "\n";
  }
  oss << "\nLevels\n";
  if (level_counts.empty()) {
    oss << "  (none)\n";
  } else {
    for (const auto& kv : level_counts) {
      oss << "  " << kv.first << " : " << kv.second << "\n";
    }
  }
  oss << "\nCommands\n";
  oss << "  logs\n";
  oss << "  logs clear\n";
  oss << "  screen logs\n";
  oss << "\nKeys\n";
  oss << "  F3 : open logs screen\n";
  oss << "  wheel : vertical scroll both panels\n";
  oss << "  Shift/Ctrl/Alt+wheel : horizontal scroll both panels\n";
  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
