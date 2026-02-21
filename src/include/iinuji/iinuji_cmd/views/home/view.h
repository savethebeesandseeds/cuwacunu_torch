#pragma once

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {
inline std::string make_home_left(const CmdState& st) {
  std::ostringstream oss;
  oss << "CUWACUNU command terminal\n\n";
  oss << "focus: command-first workflow\n";
  oss << "screens:\n";
  oss << "  F1 home\n";
  oss << "  F2 tsi board\n";
  oss << "  F3 logs\n";
  oss << "  F4 tsiemene\n";
  oss << "  F5 data\n";
  oss << "  F9 config\n\n";

  oss << "board status:\n";
  if (!st.board.ok) {
    oss << "  invalid instruction: " << st.board.error << "\n";
  } else {
    oss << "  circuits=" << st.board.board.circuits.size() << "\n";
    for (std::size_t i = 0; i < st.board.board.circuits.size(); ++i) {
      oss << "  [" << (i + 1) << "] " << st.board.board.circuits[i].name << "\n";
    }
  }
  oss << "\nconfig tabs: " << st.config.tabs.size() << "\n";
  oss << "dlogs buffered: " << cuwacunu::piaabo::dlog_buffer_size()
      << "/" << cuwacunu::piaabo::dlog_buffer_capacity() << "\n";
  return oss.str();
}

inline std::string make_home_right() {
  std::ostringstream oss;
  oss << "commands\n";
  oss << "  help\n";
  oss << "  home | board | logs | tsi | data | config\n";
  oss << "  screen home | screen board | screen logs | screen tsi | screen data | screen config\n";
  oss << "  reload | reload board | reload data | reload config\n";
  oss << "  next | prev | circuit N\n";
  oss << "  tsi | tsi tab next|prev|N|<id> | tsi tabs\n";
  oss << "  data\n";
  oss << "  data reload\n";
  oss << "  plot [on|off|toggle]\n";
  oss << "  data plotview on|off|toggle\n";
  oss << "  data plot [on|off|toggle|seq|future|weight|norm|bytes]\n";
  oss << "  data x idx|key|toggle\n";
  oss << "  data ch next|prev|N\n";
  oss << "  data sample next|prev|random|N\n";
  oss << "  data dim next|prev|N|<name>\n";
  oss << "  data mask on|off|toggle\n";
  oss << "  logs clear\n";
  oss << "  tab next | tab prev | tab N | tab <id>\n";
  oss << "  tabs\n";
  oss << "  list\n";
  oss << "  show\n";
  oss << "  quit\n";
  oss << "\nmouse\n";
  oss << "  wheel        : vertical scroll (both panels)\n";
  oss << "  Shift/Ctrl/Alt+wheel : horizontal scroll (both panels)\n";
  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
