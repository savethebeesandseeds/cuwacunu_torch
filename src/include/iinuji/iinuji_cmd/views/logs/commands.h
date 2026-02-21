#pragma once

#include <sstream>
#include <string>

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

template <class PushInfo>
inline bool handle_logs_command(CmdState& st,
                                const std::string& command,
                                std::istringstream& iss,
                                PushInfo&& push_info) {
  if (!(command == "logs" || command == "f3")) return false;
  std::string arg;
  iss >> arg;
  arg = to_lower_copy(arg);
  if (arg == "clear") {
    cuwacunu::piaabo::dlog_clear_buffer();
    push_info("logs cleared");
    st.screen = ScreenMode::Logs;
    return true;
  }
  st.screen = ScreenMode::Logs;
  push_info("screen=logs");
  return true;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
