#pragma once

#include <string>

#include "iinuji/iinuji_cmd/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

template <class PushInfo>
inline bool handle_home_command(CmdState& st, const std::string& command, PushInfo&& push_info) {
  if (!(command == "home" || command == "f1")) return false;
  st.screen = ScreenMode::Home;
  push_info("screen=home");
  return true;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
