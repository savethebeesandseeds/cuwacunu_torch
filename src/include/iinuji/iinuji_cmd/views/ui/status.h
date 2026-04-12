#pragma once

#include <sstream>
#include <string>

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string ui_status_line(const CmdState &st) {
  std::ostringstream oss;
  oss << (st.screen == ScreenMode::Home ? "[F1 HOME]" : " F1 HOME ");
  oss << " ";
  oss << (st.screen == ScreenMode::Human ? "[F2 MARSHAL]" : " F2 MARSHAL ");
  oss << " ";
  oss << (st.screen == ScreenMode::Runtime ? "[F3 RUNTIME]" : " F3 RUNTIME ");
  oss << " ";
  oss << (st.screen == ScreenMode::Lattice ? "[F4 LATTICE]" : " F4 LATTICE ");
  oss << " ";
  oss << (st.screen == ScreenMode::ShellLogs ? "[F8 SHELL LOGS]"
                                             : " F8 SHELL LOGS ");
  oss << " ";
  oss << (st.screen == ScreenMode::Config ? "[F9 CONFIG]" : " F9 CONFIG ");
  return oss.str();
}

} // namespace iinuji_cmd
} // namespace iinuji
} // namespace cuwacunu
