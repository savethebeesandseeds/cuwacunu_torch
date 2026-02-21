#pragma once

#include <sstream>
#include <string>

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline bool select_next_board_circuit(CmdState& st) {
  if (!board_has_circuits(st)) {
    st.board.selected_circuit = 0;
    return false;
  }
  st.board.selected_circuit = (st.board.selected_circuit + 1) % st.board.board.circuits.size();
  return true;
}

inline bool select_prev_board_circuit(CmdState& st) {
  if (!board_has_circuits(st)) {
    st.board.selected_circuit = 0;
    return false;
  }
  st.board.selected_circuit =
      (st.board.selected_circuit + st.board.board.circuits.size() - 1) % st.board.board.circuits.size();
  return true;
}

inline bool select_board_circuit_by_token(CmdState& st, const std::string& token) {
  std::size_t idx1 = 0;
  if (!parse_positive_index(token, &idx1)) return false;
  if (!board_has_circuits(st)) return false;
  if (idx1 > st.board.board.circuits.size()) return false;
  st.board.selected_circuit = idx1 - 1;
  return true;
}

template <class PushInfo, class PushWarn, class PushErr>
inline bool handle_board_nav_command(CmdState& st,
                                     const std::string& command,
                                     std::istringstream& iss,
                                     PushInfo&& push_info,
                                     PushWarn&& push_warn,
                                     PushErr&& push_err) {
  if (command == "next") {
    if (!select_next_board_circuit(st)) {
      push_warn("no circuits");
      return true;
    }
    st.screen = ScreenMode::Board;
    push_info("selected circuit=" + std::to_string(st.board.selected_circuit + 1));
    return true;
  }
  if (command == "prev") {
    if (!select_prev_board_circuit(st)) {
      push_warn("no circuits");
      return true;
    }
    st.screen = ScreenMode::Board;
    push_info("selected circuit=" + std::to_string(st.board.selected_circuit + 1));
    return true;
  }
  if (command == "circuit") {
    std::string arg;
    iss >> arg;
    std::size_t idx1 = 0;
    if (!parse_positive_index(arg, &idx1)) {
      push_err("usage: circuit N");
      return true;
    }
    if (!board_has_circuits(st)) {
      push_warn("no circuits");
      return true;
    }
    if (idx1 > st.board.board.circuits.size()) {
      push_err("circuit out of range");
      return true;
    }
    st.board.selected_circuit = idx1 - 1;
    st.screen = ScreenMode::Board;
    push_info("selected circuit=" + std::to_string(st.board.selected_circuit + 1));
    return true;
  }
  return false;
}

template <class PushWarn, class PushErr, class AppendLog>
inline bool handle_board_list_command(CmdState& st,
                                      const std::string& command,
                                      PushWarn&& push_warn,
                                      PushErr&& push_err,
                                      AppendLog&& append_log) {
  if (command != "list") return false;
  if (!st.board.ok) {
    push_err("board invalid: " + st.board.error);
    return true;
  }
  if (st.board.board.circuits.empty()) {
    push_warn("no circuits");
    return true;
  }
  for (std::size_t i = 0; i < st.board.board.circuits.size(); ++i) {
    const auto& c = st.board.board.circuits[i];
    append_log("[" + std::to_string(i + 1) + "] " + c.name, "list", "#d0d0d0");
  }
  return true;
}

template <class PushWarn, class PushErr, class AppendLog>
inline bool handle_board_show(CmdState& st,
                              PushWarn&& push_warn,
                              PushErr&& push_err,
                              AppendLog&& append_log) {
  if (!board_has_circuits(st)) {
    if (!st.board.ok) push_err("board invalid: " + st.board.error);
    else push_warn("no circuits");
    return true;
  }
  const auto& c = st.board.board.circuits[st.board.selected_circuit];
  append_log("circuit=" + c.name, "show", "#d8d8ff");
  append_log("invoke=" + c.invoke_name + "(\"" + c.invoke_payload + "\")", "show", "#d8d8ff");
  append_log(
      "instances=" + std::to_string(c.instances.size()) + " hops=" + std::to_string(c.hops.size()),
      "show",
      "#d8d8ff");
  return true;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
