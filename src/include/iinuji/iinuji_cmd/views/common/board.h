#pragma once

#include <exception>
#include <string>
#include <vector>

#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"
#include "iinuji/iinuji_cmd/views/board/contract.section.circuit.h"
#include "iinuji/iinuji_cmd/views/board/editor.highlight.h"
#include "iinuji/iinuji_cmd/views/common/base.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string board_instruction_path_from_config() {
  std::string path;
  if (!lookup_contract_config_value("DSL", "tsiemene_circuit_dsl_filename", &path)) {
    return "src/config/instructions/tsiemene_circuit.dsl";
  }
  if (path.empty()) {
    return "src/config/instructions/tsiemene_circuit.dsl";
  }
  return path;
}

inline bool decode_board_instruction_text(
    const std::string& raw_instruction,
    cuwacunu::camahjucunu::tsiemene_circuit_instruction_t* out_board,
    std::vector<std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t>>* out_resolved_hops,
    std::string* error) {
  if (error) error->clear();
  if (!out_board || !out_resolved_hops) {
    if (error) *error = "decode_board_instruction_text requires non-null outputs";
    return false;
  }

  try {
    auto parser = cuwacunu::camahjucunu::dsl::tsiemeneCircuits();
    auto board = parser.decode(raw_instruction);

    std::string validate_error;
    if (!cuwacunu::camahjucunu::validate_circuit_instruction(board, &validate_error)) {
      if (error) *error = validate_error;
      return false;
    }

    std::vector<std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t>> resolved;
    resolved.reserve(board.circuits.size());
    for (std::size_t i = 0; i < board.circuits.size(); ++i) {
      std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t> rh;
      std::string resolve_error;
      if (!cuwacunu::camahjucunu::resolve_hops(board.circuits[i], &rh, &resolve_error)) {
        if (error) *error = "circuit[" + std::to_string(i) + "] " + resolve_error;
        return false;
      }
      resolved.push_back(std::move(rh));
    }

    *out_board = std::move(board);
    *out_resolved_hops = std::move(resolved);
    return true;
  } catch (const std::exception& ex) {
    if (error) *error = std::string("board decode exception: ") + ex.what();
    return false;
  } catch (...) {
    if (error) *error = "board decode exception: unknown";
    return false;
  }
}

inline BoardState load_board_from_config() {
  cuwacunu::piaabo::dconfig::contract_runtime_t::assert_intact_or_fail_fast();
  BoardState out{};
  out.instruction_path = board_instruction_path_from_config();
  const auto sections =
      cuwacunu::piaabo::dconfig::contract_space_t::contract_instruction_sections();
  out.contract_observation_sources_dsl = sections.observation_sources_dsl;
  out.contract_observation_channels_dsl = sections.observation_channels_dsl;
  out.contract_jkimyei_specs_dsl = sections.jkimyei_specs_dsl;
  out.raw_instruction = sections.tsiemene_circuit_dsl;
  if (out.raw_instruction.empty()) {
    out.raw_instruction =
        cuwacunu::piaabo::dconfig::contract_space_t::tsiemene_circuit_dsl();
  }
  out.editor = std::make_shared<cuwacunu::iinuji::editorBox_data_t>(out.instruction_path);
  configure_board_editor_highlighting(*out.editor);
  cuwacunu::iinuji::primitives::editor_set_text(*out.editor, out.raw_instruction);
  out.editor->dirty = false;
  out.editor_focus = false;

  std::string error;
  if (!decode_board_instruction_text(out.raw_instruction, &out.board, &out.resolved_hops, &error)) {
    out.ok = false;
    out.error = error;
    out.contract_circuit_dsl_sections.clear();
    if (out.editor) out.editor->status = "invalid: " + error;
    return out;
  }

  out.contract_circuit_dsl_sections.clear();
  out.contract_circuit_dsl_sections.reserve(out.board.circuits.size());
  for (const auto& c : out.board.circuits) {
    out.contract_circuit_dsl_sections.push_back(board_contract_section_render_circuit_dsl(c));
  }
  out.ok = true;
  if (out.editor) out.editor->status = "ok";
  return out;
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
