#pragma once

#include <cstdint>
#include <string>

#include "iinuji/iinuji_cmd/views/common.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

enum class BoardViewOptionRow : std::uint8_t {
  CircuitDiagram = 0,
  ContractTextEdit = 1,
};

inline constexpr std::size_t board_view_option_row_count() {
  return board_view_option_count();
}

inline constexpr std::size_t board_contract_section_row_count() {
  return board_contract_section_count();
}

inline void clamp_board_view_option(CmdState& st) {
  const std::size_t n = board_view_option_row_count();
  if (n == 0) {
    st.board.selected_view_option = 0;
    return;
  }
  if (st.board.selected_view_option >= n) st.board.selected_view_option = 0;
}

inline bool select_next_board_view_option(CmdState& st) {
  const std::size_t n = board_view_option_row_count();
  if (n == 0) {
    st.board.selected_view_option = 0;
    return false;
  }
  st.board.selected_view_option = (st.board.selected_view_option + 1) % n;
  return true;
}

inline bool select_prev_board_view_option(CmdState& st) {
  const std::size_t n = board_view_option_row_count();
  if (n == 0) {
    st.board.selected_view_option = 0;
    return false;
  }
  st.board.selected_view_option = (st.board.selected_view_option + n - 1) % n;
  return true;
}

inline void clamp_board_contract_section(CmdState& st) {
  const std::size_t n = board_contract_section_row_count();
  if (n == 0) {
    st.board.selected_contract_section = 0;
    return;
  }
  if (st.board.selected_contract_section >= n) st.board.selected_contract_section = 0;
}

inline bool select_next_board_contract_section(CmdState& st) {
  const std::size_t n = board_contract_section_row_count();
  if (n == 0) {
    st.board.selected_contract_section = 0;
    return false;
  }
  st.board.selected_contract_section = (st.board.selected_contract_section + 1) % n;
  return true;
}

inline bool select_prev_board_contract_section(CmdState& st) {
  const std::size_t n = board_contract_section_row_count();
  if (n == 0) {
    st.board.selected_contract_section = 0;
    return false;
  }
  st.board.selected_contract_section =
      (st.board.selected_contract_section + n - 1) % n;
  return true;
}

inline BoardDisplayMode board_display_mode_for_option(std::size_t option_row) {
  switch (static_cast<BoardViewOptionRow>(option_row)) {
    case BoardViewOptionRow::CircuitDiagram:
      return BoardDisplayMode::Diagram;
    case BoardViewOptionRow::ContractTextEdit:
      return BoardDisplayMode::ContractTextEdit;
  }
  return BoardDisplayMode::Diagram;
}

inline std::string board_view_option_label(std::size_t option_row) {
  switch (static_cast<BoardViewOptionRow>(option_row)) {
    case BoardViewOptionRow::CircuitDiagram:
      return "Contract Circuit Diagram";
    case BoardViewOptionRow::ContractTextEdit:
      return "Contract Text (edit)";
  }
  return "Contract Circuit Diagram";
}

inline bool select_next_board_circuit(CmdState& st) {
  if (!board_has_circuits(st)) {
    st.board.selected_circuit = 0;
    return false;
  }
  st.board.selected_circuit = (st.board.selected_circuit + 1) % st.board.board.circuits.size();
  st.board.editing_contract_index = st.board.selected_circuit;
  return true;
}

inline bool select_prev_board_circuit(CmdState& st) {
  if (!board_has_circuits(st)) {
    st.board.selected_circuit = 0;
    return false;
  }
  st.board.selected_circuit =
      (st.board.selected_circuit + st.board.board.circuits.size() - 1) % st.board.board.circuits.size();
  st.board.editing_contract_index = st.board.selected_circuit;
  return true;
}

template <class PushWarn, class PushErr, class AppendLog>
inline bool handle_board_show(CmdState& st,
                              PushWarn&& push_warn,
                              PushErr&& push_err,
                              AppendLog&& append_log) {
  if (!board_has_circuits(st)) {
    if (!st.board.ok) push_err("board invalid: " + st.board.error);
    else push_warn("no contracts");
    return true;
  }
  const auto& c = st.board.board.circuits[st.board.selected_circuit];
  append_log("contract=" + c.name, "show", "#d8d8ff");
  append_log("circuit.invoke=" + c.invoke_name + "(\"" + c.invoke_payload + "\")", "show", "#d8d8ff");
  append_log(
      "circuit.instances=" + std::to_string(c.instances.size()) +
          " circuit.hops=" + std::to_string(c.hops.size()),
      "show",
      "#d8d8ff");
  return true;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
