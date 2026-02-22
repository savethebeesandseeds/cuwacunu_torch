#pragma once

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
