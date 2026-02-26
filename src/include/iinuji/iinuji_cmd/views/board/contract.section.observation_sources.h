#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "iinuji/iinuji_cmd/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline constexpr std::string_view board_contract_section_observation_sources_key() {
  return "board.contract.observation_sources@DSL:str";
}

inline constexpr std::string_view board_contract_section_observation_sources_title() {
  return "Observation Sources";
}

inline std::string board_contract_section_get_observation_sources_text(
    const CmdState& st, std::size_t /*contract_index*/) {
  return st.board.contract_observation_sources_dsl;
}

inline void board_contract_section_set_observation_sources_text(
    CmdState& st, std::size_t /*contract_index*/, std::string text) {
  st.board.contract_observation_sources_dsl = std::move(text);
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
