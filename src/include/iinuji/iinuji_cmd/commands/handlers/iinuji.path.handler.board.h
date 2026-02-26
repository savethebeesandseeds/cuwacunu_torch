#pragma once

// Included inside struct IinujiPathHandlers.

template <class PushInfo, class PushWarn, class PushErr, class AppendLog>
bool dispatch_board_call(CallHandlerId call_id,
                         PushInfo&& push_info,
                         PushWarn&& push_warn,
                         PushErr&& push_err,
                         AppendLog&& append_log) const {
  switch (call_id) {
    case CallHandlerId::BoardList:
      if (!state.board.ok) {
        push_err("board invalid: " + state.board.error);
        return true;
      }
      if (state.board.board.circuits.empty()) {
        push_warn("no contracts");
        return true;
      }
      for (std::size_t i = 0; i < state.board.board.circuits.size(); ++i) {
        const auto& c = state.board.board.circuits[i];
        append_log("[" + std::to_string(i + 1) + "] " + c.name, "list", "#d0d0d0");
      }
      return true;
    case CallHandlerId::BoardSelectNext:
      if (!select_next_board_circuit(state)) {
        push_warn("no contracts");
        return true;
      }
      screen.board();
      push_info("selected contract=" + std::to_string(state.board.selected_circuit + 1));
      return true;
    case CallHandlerId::BoardSelectPrev:
      if (!select_prev_board_circuit(state)) {
        push_warn("no contracts");
        return true;
      }
      screen.board();
      push_info("selected contract=" + std::to_string(state.board.selected_circuit + 1));
      return true;
    default:
      return false;
  }
}

template <class PushInfo, class PushWarn, class PushErr>
bool dispatch_board_select_index(const cuwacunu::camahjucunu::canonical_path_t& path,
                                 PushInfo&& push_info,
                                 PushWarn&& push_warn,
                                 PushErr&& push_err) const {
  std::size_t idx1 = 0;
  if (!parse_positive_arg_or_tail(path, 4, &idx1)) {
    push_err("usage: " + canonical_paths::build_board_select_index(1));
    return true;
  }
  if (!board_has_circuits(state)) {
    push_warn("no contracts");
    return true;
  }
  if (idx1 > state.board.board.circuits.size()) {
    push_err("contract out of range");
    return true;
  }
  state.board.selected_circuit = idx1 - 1;
  screen.board();
  push_info("selected contract=" + std::to_string(state.board.selected_circuit + 1));
  return true;
}
