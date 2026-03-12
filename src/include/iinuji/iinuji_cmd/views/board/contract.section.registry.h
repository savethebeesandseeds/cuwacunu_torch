#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>

#include "iinuji/iinuji_cmd/views/board/contract.section.circuit.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline constexpr std::array<BoardContractSection, 1> kBoardContractSectionsInOrder = {
    BoardContractSection::Circuit};

inline constexpr std::size_t board_contract_section_index(BoardContractSection section) {
  (void)section;
  return 0;
}

inline constexpr BoardContractSection board_contract_section_from_index(std::size_t idx) {
  (void)idx;
  return BoardContractSection::Circuit;
}

inline constexpr std::string_view board_contract_section_key(BoardContractSection section) {
  (void)section;
  return board_contract_section_circuit_key();
}

inline constexpr std::string_view board_contract_section_title(BoardContractSection section) {
  (void)section;
  return board_contract_section_circuit_title();
}

inline std::string board_contract_section_get_text(const CmdState& st,
                                                   std::size_t contract_index,
                                                   BoardContractSection section) {
  (void)section;
  return board_contract_section_get_circuit_text(st, contract_index);
}

inline void board_contract_section_set_text(CmdState& st,
                                            std::size_t contract_index,
                                            BoardContractSection section,
                                            std::string text) {
  (void)section;
  board_contract_section_set_circuit_text(st, contract_index, std::move(text));
}

inline void board_contract_sections_sync_from_runtime_board(CmdState& st) {
  board_contract_section_sync_circuit_cache(st);
}

inline std::string render_board_contract_text_by_sections(const CmdState& st,
                                                          std::size_t contract_index) {
  std::ostringstream oss;
  bool first = true;
  for (const BoardContractSection section : kBoardContractSectionsInOrder) {
    if (!first) oss << "\n";
    first = false;
    const std::string_view key = board_contract_section_key(section);
    oss << "BEGIN " << key << "\n";
    const std::string section_text = board_contract_section_get_text(st, contract_index, section);
    if (section_text.empty()) {
      oss << "# missing DSL text\n";
    } else {
      oss << section_text;
      if (section_text.back() != '\n') oss << "\n";
    }
    oss << "END " << key << "\n";
  }
  return oss.str();
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
