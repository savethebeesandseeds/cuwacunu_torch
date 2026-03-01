#pragma once

#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "iinuji/iinuji_cmd/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline constexpr std::string_view board_contract_section_circuit_key() {
  return "board.contract.circuit@DSL:str";
}

inline constexpr std::string_view board_contract_section_circuit_title() {
  return "Circuit";
}

inline std::string board_contract_section_render_circuit_dsl(
    const cuwacunu::camahjucunu::tsiemene_circuit_decl_t& c) {
  std::ostringstream oss;
  oss << c.name << " = {\n";
  for (const auto& inst : c.instances) {
    oss << "  " << inst.alias << " = " << inst.tsi_type << "\n";
  }
  for (const auto& h : c.hops) {
    oss << "  " << h.from.instance << "@" << h.from.directive;
    if (!h.from.kind.empty()) oss << ":" << h.from.kind;
    oss << " -> " << h.to.instance << "@" << h.to.directive;
    if (!h.to.kind.empty()) oss << ":" << h.to.kind;
    oss << "\n";
  }
  oss << "}\n";
  oss << c.invoke_name << "(" << c.invoke_payload << ");\n";
  return oss.str();
}

inline void board_contract_section_sync_circuit_cache(CmdState& st) {
  st.board.contract_circuit_dsl_sections.clear();
  st.board.contract_circuit_dsl_sections.reserve(st.board.board.contracts.size());
  for (const auto& c : st.board.board.contracts) {
    st.board.contract_circuit_dsl_sections.push_back(board_contract_section_render_circuit_dsl(c));
  }
}

inline std::string board_contract_section_get_circuit_text(const CmdState& st, std::size_t contract_index) {
  if (contract_index < st.board.contract_circuit_dsl_sections.size()) {
    return st.board.contract_circuit_dsl_sections[contract_index];
  }
  if (contract_index < st.board.board.contracts.size()) {
    return board_contract_section_render_circuit_dsl(st.board.board.contracts[contract_index]);
  }
  return {};
}

inline void board_contract_section_set_circuit_text(CmdState& st,
                                                    std::size_t contract_index,
                                                    std::string text) {
  if (st.board.contract_circuit_dsl_sections.size() <= contract_index) {
    st.board.contract_circuit_dsl_sections.resize(contract_index + 1);
  }
  st.board.contract_circuit_dsl_sections[contract_index] = std::move(text);
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
