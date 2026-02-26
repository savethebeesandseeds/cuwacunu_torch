#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "iinuji/iinuji_cmd/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline constexpr std::string_view board_contract_section_jkimyei_specs_key() {
  return "board.contract.jkimyei_specs@DSL:str";
}

inline constexpr std::string_view board_contract_section_jkimyei_specs_title() {
  return "Jkimyei Specs";
}

inline std::string board_contract_section_get_jkimyei_specs_text(
    const CmdState& st, std::size_t /*contract_index*/) {
  return st.board.contract_jkimyei_specs_dsl;
}

inline void board_contract_section_set_jkimyei_specs_text(
    CmdState& st, std::size_t /*contract_index*/, std::string text) {
  st.board.contract_jkimyei_specs_dsl = std::move(text);
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
