#pragma once

#include <exception>
#include <filesystem>
#include <string>
#include <vector>

#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "camahjucunu/dsl/tsiemene_board/tsiemene_board.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"
#include "iinuji/iinuji_cmd/views/board/contract.section.circuit.h"
#include "iinuji/iinuji_cmd/views/board/editor.highlight.h"
#include "iinuji/iinuji_cmd/views/common/base.h"
#include "iitepi/observation_contract_wave_paths.h"
#include "piaabo/dfiles.h"

namespace cuwacunu {
namespace iinuji {
namespace iinuji_cmd {

inline std::string board_instruction_path_from_config(
    const cuwacunu::iitepi::contract_hash_t& contract_hash) {
  std::string path;
  if (!lookup_contract_config_value(
          "CONTRACT", "circuit_dsl_filename", contract_hash, &path)) {
    return "src/config/instructions/defaults/default.iitepi.circuit.dsl";
  }
  if (path.empty()) {
    return "src/config/instructions/defaults/default.iitepi.circuit.dsl";
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
    const auto contract_itself =
        cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
    auto parser = cuwacunu::camahjucunu::dsl::tsiemeneCircuits(
        contract_itself->circuit.grammar);
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

inline BoardState load_board_from_contract_hash(
    const cuwacunu::iitepi::contract_hash_t& contract_hash) {
  BoardState out{};
  if (contract_hash.empty()) {
    out.ok = false;
    out.error = "missing board contract hash";
    return out;
  }
  out.contract_hash = contract_hash;
  const auto contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(out.contract_hash);
  out.contract_path = contract_itself->config_file_path;
  cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(
      out.contract_hash);

  out.instruction_path = board_instruction_path_from_config(out.contract_hash);
  out.contract_observation_sources_dsl.clear();
  out.contract_observation_channels_dsl.clear();
  out.contract_jkimyei_specs_dsl.clear();
  {
    const auto sec_it = contract_itself->module_sections.find("VICReg");
    if (sec_it != contract_itself->module_sections.end()) {
      const auto key_it = sec_it->second.find("jkimyei_dsl_file");
      if (key_it != sec_it->second.end()) {
        std::filesystem::path module_path{};
        const auto path_it = contract_itself->module_section_paths.find("VICReg");
        if (path_it != contract_itself->module_section_paths.end()) {
          module_path = path_it->second;
        }
        std::filesystem::path dsl_path = key_it->second;
        if (dsl_path.is_relative() && !module_path.empty()) {
          dsl_path = module_path.parent_path() / dsl_path;
        }
        std::error_code ec;
        if (std::filesystem::exists(dsl_path, ec) &&
            std::filesystem::is_regular_file(dsl_path, ec)) {
          out.contract_jkimyei_specs_dsl =
              cuwacunu::piaabo::dfiles::readFileToString(dsl_path);
        }
      }
    }
  }
  try {
    const std::string board_hash =
        cuwacunu::iitepi::config_space_t::locked_board_hash();
    const std::string binding_id =
        cuwacunu::iitepi::config_space_t::locked_board_binding_id();
    const std::string bound_contract_hash =
        cuwacunu::iitepi::board_space_t::contract_hash_for_binding(
            board_hash, binding_id);
    if (bound_contract_hash == out.contract_hash) {
      const auto board_itself =
          cuwacunu::iitepi::board_space_t::board_itself(board_hash);
      const auto& board_instruction = board_itself->board.decoded();
      const cuwacunu::camahjucunu::tsiemene_board_bind_decl_t* bind = nullptr;
      for (const auto& b : board_instruction.binds) {
        if (b.id == binding_id) {
          bind = &b;
          break;
        }
      }
      if (bind) {
        const std::string wave_hash =
            cuwacunu::iitepi::board_space_t::wave_hash_for_binding(
                board_hash, binding_id);
        const auto wave_itself =
            cuwacunu::iitepi::wave_space_t::wave_itself(wave_hash);
        const auto& wave_set = wave_itself->wave.decoded();
        const cuwacunu::camahjucunu::iitepi_wave_t* selected_wave = nullptr;
        for (const auto& wave : wave_set.waves) {
          if (wave.name == bind->wave_ref) {
            selected_wave = &wave;
            break;
          }
        }
        if (selected_wave && !selected_wave->sources.empty()) {
          cuwacunu::iitepi::observation_dsl_path_resolution_t observation_paths{};
          std::string path_error{};
          if (cuwacunu::iitepi::resolve_observation_dsl_paths(
                  contract_itself, wave_itself, *selected_wave,
                  &observation_paths, &path_error)) {
            if (std::filesystem::exists(observation_paths.sources_path)) {
              out.contract_observation_sources_dsl =
                  cuwacunu::piaabo::dfiles::readFileToString(
                      observation_paths.sources_path);
            }
            if (std::filesystem::exists(observation_paths.channels_path)) {
              out.contract_observation_channels_dsl =
                  cuwacunu::piaabo::dfiles::readFileToString(
                      observation_paths.channels_path);
            }
          }
        }
      }
    }
  } catch (...) {
    // keep editor state best-effort; missing runtime binding context is not fatal.
  }
  out.raw_instruction = contract_itself->circuit.dsl;
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
  out.contract_circuit_dsl_sections.reserve(out.board.circuits.size());
  for (const auto& c : out.board.circuits) {
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
