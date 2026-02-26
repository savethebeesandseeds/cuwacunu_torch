#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <sstream>
#include <string>
#include <string_view>

#include "iinuji/iinuji_cmd/views/board/contract.section.circuit.h"
#include "iinuji/iinuji_cmd/views/board/contract.section.observation_sources.h"
#include "iinuji/iinuji_cmd/views/board/contract.section.observation_channels.h"
#include "iinuji/iinuji_cmd/views/board/contract.section.jkimyei_specs.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline constexpr std::array<BoardContractSection, 4> kBoardContractSectionsInOrder = {
    BoardContractSection::Circuit,
    BoardContractSection::ObservationSources,
    BoardContractSection::ObservationChannels,
    BoardContractSection::JkimyeiSpecs,
};

inline constexpr std::size_t board_contract_section_index(BoardContractSection section) {
  switch (section) {
    case BoardContractSection::Circuit:
      return 0;
    case BoardContractSection::ObservationSources:
      return 1;
    case BoardContractSection::ObservationChannels:
      return 2;
    case BoardContractSection::JkimyeiSpecs:
      return 3;
  }
  return 0;
}

inline constexpr BoardContractSection board_contract_section_from_index(std::size_t idx) {
  switch (idx) {
    case 0:
      return BoardContractSection::Circuit;
    case 1:
      return BoardContractSection::ObservationSources;
    case 2:
      return BoardContractSection::ObservationChannels;
    case 3:
      return BoardContractSection::JkimyeiSpecs;
    default:
      return BoardContractSection::Circuit;
  }
}

inline constexpr std::string_view board_contract_section_key(BoardContractSection section) {
  switch (section) {
    case BoardContractSection::Circuit:
      return board_contract_section_circuit_key();
    case BoardContractSection::ObservationSources:
      return board_contract_section_observation_sources_key();
    case BoardContractSection::ObservationChannels:
      return board_contract_section_observation_channels_key();
    case BoardContractSection::JkimyeiSpecs:
      return board_contract_section_jkimyei_specs_key();
  }
  return board_contract_section_circuit_key();
}

inline constexpr std::string_view board_contract_section_title(BoardContractSection section) {
  switch (section) {
    case BoardContractSection::Circuit:
      return board_contract_section_circuit_title();
    case BoardContractSection::ObservationSources:
      return board_contract_section_observation_sources_title();
    case BoardContractSection::ObservationChannels:
      return board_contract_section_observation_channels_title();
    case BoardContractSection::JkimyeiSpecs:
      return board_contract_section_jkimyei_specs_title();
  }
  return board_contract_section_circuit_title();
}

inline std::string board_contract_section_get_text(const CmdState& st,
                                                   std::size_t contract_index,
                                                   BoardContractSection section) {
  switch (section) {
    case BoardContractSection::Circuit:
      return board_contract_section_get_circuit_text(st, contract_index);
    case BoardContractSection::ObservationSources:
      return board_contract_section_get_observation_sources_text(st, contract_index);
    case BoardContractSection::ObservationChannels:
      return board_contract_section_get_observation_channels_text(st, contract_index);
    case BoardContractSection::JkimyeiSpecs:
      return board_contract_section_get_jkimyei_specs_text(st, contract_index);
  }
  return {};
}

inline void board_contract_section_set_text(CmdState& st,
                                            std::size_t contract_index,
                                            BoardContractSection section,
                                            std::string text) {
  switch (section) {
    case BoardContractSection::Circuit:
      board_contract_section_set_circuit_text(st, contract_index, std::move(text));
      return;
    case BoardContractSection::ObservationSources:
      board_contract_section_set_observation_sources_text(st, contract_index, std::move(text));
      return;
    case BoardContractSection::ObservationChannels:
      board_contract_section_set_observation_channels_text(st, contract_index, std::move(text));
      return;
    case BoardContractSection::JkimyeiSpecs:
      board_contract_section_set_jkimyei_specs_text(st, contract_index, std::move(text));
      return;
  }
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
