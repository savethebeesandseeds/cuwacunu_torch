#pragma once

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {
inline std::string make_config_left(const CmdState& st) {
  if (!st.config.ok) {
    return "Config tabs not loaded.\n\nerror: " + st.config.error;
  }
  if (st.config.tabs.empty()) return "No config tabs.";
  const auto& tab = st.config.tabs[st.config.selected_tab];

  std::ostringstream oss;
  oss << "# tab: " << tab.title << "\n";
  if (!tab.path.empty()) oss << "# path: " << tab.path << "\n";
  if (!tab.ok) {
    oss << "# load error: " << tab.error << "\n";
  }
  oss << "\n";
  if (!tab.content.empty()) oss << tab.content;
  return oss.str();
}

inline std::string make_config_right(const CmdState& st) {
  std::ostringstream oss;
  oss << "Config tabs\n";
  if (st.config.tabs.empty()) {
    oss << "  (none)\n";
  } else {
    for (std::size_t i = 0; i < st.config.tabs.size(); ++i) {
      const auto& t = st.config.tabs[i];
      const bool active = (i == st.config.selected_tab);
      std::ostringstream row;
      row << "  " << (active ? ">" : " ") << "[" << (i + 1) << "] " << t.id;
      if (!t.ok) row << " (err)";
      if (active) oss << mark_selected_line(row.str()) << "\n";
      else oss << row.str() << "\n";
    }
  }
  oss << "\nCommands\n";
  static const auto kConfigCallCommands = canonical_paths::call_texts_by_prefix({"iinuji.config."});
  static const auto kConfigPatternCommands = canonical_paths::pattern_texts_by_prefix({"iinuji.config."});
  for (const auto cmd : kConfigCallCommands) oss << "  " << cmd << "\n";
  for (const auto cmd : kConfigPatternCommands) oss << "  " << cmd << "\n";
  oss << "\nCanonical\n";
  oss << "  aliases: tabs, config, f9\n";
  oss << "  primitive translation: disabled\n";
  oss << "\nKeys\n";
  oss << "  F9 : open config screen\n";
  oss << "  Up/Down : previous/next tab\n";
  oss << "  wheel : vertical scroll both panels\n";
  oss << "  Shift/Ctrl/Alt+wheel : horizontal scroll both panels\n";
  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
