#pragma once

#include <exception>
#include <filesystem>
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

inline std::string board_instruction_path_from_config(
    const cuwacunu::piaabo::dconfig::contract_hash_t& contract_hash) {
  std::string path;
  if (!lookup_contract_config_value(
          "DSL", "tsiemene_circuit_dsl_filename", contract_hash, &path)) {
    return "src/config/instructions/tsiemene_circuit.dsl";
  }
  if (path.empty()) {
    return "src/config/instructions/tsiemene_circuit.dsl";
  }
  return path;
}

inline bool decode_board_instruction_text(
    const std::string& raw_instruction,
    const std::string& contract_hash,
    cuwacunu::camahjucunu::tsiemene_circuit_instruction_t* out_board,
    std::vector<std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t>>* out_resolved_hops,
    std::string* error) {
  if (error) error->clear();
  if (!out_board || !out_resolved_hops) {
    if (error) *error = "decode_board_instruction_text requires non-null outputs";
    return false;
  }
  if (contract_hash.empty()) {
    if (error) *error = "decode_board_instruction_text requires a contract hash";
    return false;
  }

  try {
    auto parser = cuwacunu::camahjucunu::dsl::tsiemeneCircuits(
        cuwacunu::piaabo::dconfig::contract_space_t::tsiemene_circuit_grammar(
            contract_hash));
    auto board = parser.decode(raw_instruction);

    std::string validate_error;
    if (!cuwacunu::camahjucunu::validate_circuit_instruction(board, &validate_error)) {
      if (error) *error = validate_error;
      return false;
    }

    std::vector<std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t>> resolved;
    resolved.reserve(board.contracts.size());
    for (std::size_t i = 0; i < board.contracts.size(); ++i) {
      std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t> rh;
      std::string resolve_error;
      if (!cuwacunu::camahjucunu::resolve_hops(board.contracts[i], &rh, &resolve_error)) {
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

inline BoardState load_board_from_contract_hash(
    const cuwacunu::piaabo::dconfig::contract_hash_t& contract_hash) {
  BoardState out{};
  if (contract_hash.empty()) {
    out.ok = false;
    out.error = "missing board contract hash";
    return out;
  }
  out.contract_hash = contract_hash;
  out.contract_path =
      cuwacunu::piaabo::dconfig::contract_space_t::snapshot(out.contract_hash)
          .config_file_path;
  cuwacunu::piaabo::dconfig::contract_space_t::assert_intact_or_fail_fast(
      out.contract_hash);

  out.instruction_path = board_instruction_path_from_config(out.contract_hash);
  const auto sections =
      cuwacunu::piaabo::dconfig::contract_space_t::contract_instruction_sections(
          out.contract_hash);
  out.contract_observation_sources_dsl = sections.observation_sources_dsl;
  out.contract_observation_channels_dsl = sections.observation_channels_dsl;
  out.contract_jkimyei_specs_dsl = sections.jkimyei_specs_dsl;
  out.raw_instruction = sections.tsiemene_circuit_dsl;
  if (out.raw_instruction.empty()) {
    out.raw_instruction = cuwacunu::piaabo::dconfig::contract_space_t::
        tsiemene_circuit_dsl(out.contract_hash);
  }
  out.editor = std::make_shared<cuwacunu::iinuji::editorBox_data_t>(out.instruction_path);
  configure_board_editor_highlighting(*out.editor);
  cuwacunu::iinuji::primitives::editor_set_text(*out.editor, out.raw_instruction);
  out.editor->dirty = false;
  out.editor_focus = false;

  std::string error;
  if (!decode_board_instruction_text(
          out.raw_instruction, out.contract_hash, &out.board, &out.resolved_hops,
          &error)) {
    out.ok = false;
    out.error = error;
    out.contract_circuit_dsl_sections.clear();
    if (out.editor) out.editor->status = "invalid: " + error;
    return out;
  }

  out.contract_circuit_dsl_sections.clear();
  out.contract_circuit_dsl_sections.reserve(out.board.contracts.size());
  for (const auto& c : out.board.contracts) {
    out.contract_circuit_dsl_sections.push_back(board_contract_section_render_circuit_dsl(c));
  }
  out.ok = true;
  if (out.editor) out.editor->status = "ok";
  return out;
}

inline BoardState load_board_from_config() {
  return load_board_from_contract_hash(resolve_configured_board_contract_hash());
}

}  // namespace iinuji_cmd
}  // namespace iinuji
}  // namespace cuwacunu
