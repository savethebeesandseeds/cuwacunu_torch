#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"
#include "iinuji/iinuji_types.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

using cuwacunu::camahjucunu::tsiemene_circuit_instruction_t;
using cuwacunu::camahjucunu::tsiemene_resolved_hop_t;

enum class BoardPanelFocus : std::uint8_t {
  Context = 0,
  ViewOptions = 1,
  ContractSections = 2,
};

enum class BoardDisplayMode : std::uint8_t {
  Diagram = 0,
  ContractTextEdit = 1,
};

enum class BoardContractSection : std::uint8_t {
  Circuit = 0,
  ObservationSources = 1,
  ObservationChannels = 2,
  JkimyeiSpecs = 3,
};

enum class BoardEditorScope : std::uint8_t {
  None = 0,
  ContractVirtual = 1,
  FullInstruction = 2,
  ContractSection = 3,
};

struct BoardState {
  enum class ExitPrompt {
    None,
    SaveDiscardCancel,
  };

  bool ok{false};
  std::string error{};
  std::string raw_instruction{};
  std::string instruction_path{};
  // Shared board.contract DSL sections (non-circuit segments loaded from config).
  std::string contract_observation_sources_dsl{};
  std::string contract_observation_channels_dsl{};
  std::string contract_jkimyei_specs_dsl{};
  tsiemene_circuit_instruction_t board{};
  std::vector<std::vector<tsiemene_resolved_hop_t>> resolved_hops{};
  std::size_t selected_circuit{0};
  BoardPanelFocus panel_focus{BoardPanelFocus::Context};
  BoardDisplayMode display_mode{BoardDisplayMode::Diagram};
  std::size_t selected_view_option{0};
  std::size_t selected_contract_section{0};
  std::shared_ptr<cuwacunu::iinuji::editorBox_data_t> editor{};
  bool editor_focus{false};
  BoardEditorScope editor_scope{BoardEditorScope::None};
  std::size_t editing_contract_index{0};
  BoardContractSection editing_contract_section{BoardContractSection::Circuit};
  std::vector<std::string> contract_circuit_dsl_sections{};
  ExitPrompt exit_prompt{ExitPrompt::None};
  int exit_prompt_index{0};
  bool diagnostic_active{false};
  int diagnostic_line{-1};
  int diagnostic_col{-1};
  std::string diagnostic_message{};
  bool completion_active{false};
  std::vector<std::string> completion_items{};
  std::size_t completion_index{0};
  int completion_line{-1};
  int completion_start_col{-1};
};

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
