// ./include/tsiemene/board.contract.init.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cctype>
#include <exception>
#include <string>

#include <torch/torch.h>

#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "tsiemene/board.builder.h"

namespace tsiemene {

namespace board_action_id {
#define BOARD_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define BOARD_PATH_METHOD(ID, TOKEN, SUMMARY)
#define BOARD_PATH_DSL_SEGMENT(ID, KEY, SUMMARY)
#define BOARD_PATH_ACTION(ID, TOKEN, SUMMARY) inline constexpr const char ID[] = TOKEN;
#include "tsiemene/board.paths.def"
#undef BOARD_PATH_ACTION
#undef BOARD_PATH_DSL_SEGMENT
#undef BOARD_PATH_METHOD
#undef BOARD_PATH_DIRECTIVE
} // namespace board_action_id

inline constexpr const char* kBoardContractInitCanonicalAction = board_action_id::ContractInit;

struct board_contract_init_record_t {
  bool ok{false};
  std::string error{};
  std::string canonical_action{kBoardContractInitCanonicalAction};
  std::string contract_hash{};
  std::string source_config_path{};
  Board board{};
};

[[nodiscard]] inline bool has_non_ws_text(std::string_view s) {
  for (const unsigned char c : s) {
    if (!std::isspace(c)) return true;
  }
  return false;
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline board_contract_init_record_t invoke_board_contract_init_from_snapshot(
    const cuwacunu::piaabo::dconfig::contract_hash_t& contract_hash,
    const cuwacunu::piaabo::dconfig::contract_snapshot_t& snapshot,
    torch::Device device = torch::kCPU) {
  board_contract_init_record_t out{};
  out.contract_hash = contract_hash;
  out.source_config_path = snapshot.config_file_path;

  try {
    const auto& sections = snapshot.contract_instruction_sections;
    if (!has_non_ws_text(sections.observation_sources_dsl)) {
      out.error = "missing observation sources DSL text in config";
      return out;
    }
    if (!has_non_ws_text(sections.observation_channels_dsl)) {
      out.error = "missing observation channels DSL text in config";
      return out;
    }
    if (!has_non_ws_text(sections.jkimyei_specs_dsl)) {
      out.error = "missing jkimyei specs DSL text in config";
      return out;
    }
    if (!has_non_ws_text(sections.tsiemene_circuit_dsl)) {
      out.error = "missing tsiemene circuit DSL text in config";
      return out;
    }
    if (!has_non_ws_text(sections.tsiemene_wave_dsl)) {
      out.error = "missing tsiemene wave DSL text in config";
      return out;
    }

    const auto grammar_it =
        snapshot.dsl_asset_text_by_key.find("tsiemene_circuit_grammar_filename");
    if (grammar_it == snapshot.dsl_asset_text_by_key.end() ||
        !has_non_ws_text(grammar_it->second)) {
      out.error = "missing tsiemene circuit grammar text in contract snapshot";
      return out;
    }

    auto parser =
        cuwacunu::camahjucunu::dsl::tsiemeneCircuits(grammar_it->second);
    auto parsed = parser.decode(sections.tsiemene_circuit_dsl);
    std::string semantic_error;
    if (!cuwacunu::camahjucunu::validate_circuit_instruction(parsed, &semantic_error)) {
      out.error = "invalid tsiemene circuit instruction: " + semantic_error;
      return out;
    }

    std::string build_error;
    if (!board_builder::build_runtime_board_from_instruction<Datatype_t, Sampler_t>(
            parsed, device, contract_hash, snapshot, &out.board, &build_error)) {
      out.error = "failed to build runtime board: " + build_error;
      return out;
    }

    BoardIssue issue{};
    if (!validate_board(out.board, &issue)) {
      out.error = "invalid runtime board: " + std::string(issue.circuit_issue.what);
      return out;
    }

    out.ok = true;
    return out;
  } catch (const std::exception& e) {
    out.error = std::string(kBoardContractInitCanonicalAction) + " exception: " + e.what();
    return out;
  } catch (...) {
    out.error = std::string(kBoardContractInitCanonicalAction) + " exception: unknown";
    return out;
  }
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline board_contract_init_record_t invoke_board_contract_init_from_file(
    const std::string& contract_file_path,
    torch::Device device = torch::kCPU) {
  board_contract_init_record_t out{};

  try {
    const auto contract_hash =
        cuwacunu::piaabo::dconfig::contract_space_t::register_contract_file(
            contract_file_path);
    cuwacunu::piaabo::dconfig::contract_space_t::assert_intact_or_fail_fast(
        contract_hash);
    const auto& snapshot =
        cuwacunu::piaabo::dconfig::contract_space_t::snapshot(contract_hash);
    return invoke_board_contract_init_from_snapshot<Datatype_t, Sampler_t>(
        contract_hash, snapshot, device);
  } catch (const std::exception& e) {
    out.error = std::string(kBoardContractInitCanonicalAction) + " exception: " + e.what();
    return out;
  } catch (...) {
    out.error = std::string(kBoardContractInitCanonicalAction) + " exception: unknown";
    return out;
  }
}

} // namespace tsiemene
