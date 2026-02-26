#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "iinuji/iinuji_cmd/state.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline constexpr std::string_view board_contract_section_observation_channels_key() {
  return "board.contract.observation_channels@DSL:str";
}

inline constexpr std::string_view board_contract_section_observation_channels_title() {
  return "Observation Channels";
}

inline std::string board_contract_section_get_observation_channels_text(
    const CmdState& st, std::size_t /*contract_index*/) {
  return st.board.contract_observation_channels_dsl;
}

inline void board_contract_section_set_observation_channels_text(
    CmdState& st, std::size_t /*contract_index*/, std::string text) {
  st.board.contract_observation_channels_dsl = std::move(text);
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
