// ./include/iitepi/board.contract.init.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cctype>
#include <exception>
#include <filesystem>
#include <memory>
#include <string>

#include <torch/torch.h>

#include "camahjucunu/dsl/tsiemene_board/tsiemene_board.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "piaabo/dconfig.h"
#include "iitepi/board/board.builder.h"

namespace tsiemene {

namespace board_action_id {
#define BOARD_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define BOARD_PATH_METHOD(ID, TOKEN, SUMMARY)
#define BOARD_PATH_DSL_SEGMENT(ID, KEY, SUMMARY)
#define BOARD_PATH_ACTION(ID, TOKEN, SUMMARY) inline constexpr const char ID[] = TOKEN;
#include "iitepi/board/board.paths.def"
#undef BOARD_PATH_ACTION
#undef BOARD_PATH_DSL_SEGMENT
#undef BOARD_PATH_METHOD
#undef BOARD_PATH_DIRECTIVE
}  // namespace board_action_id

inline constexpr const char* kBoardContractInitCanonicalAction =
    board_action_id::ContractInit;

struct board_contract_init_record_t {
  bool ok{false};
  std::string error{};
  std::string canonical_action{kBoardContractInitCanonicalAction};
  std::string board_hash{};
  std::string board_binding_id{};
  std::string contract_hash{};
  std::string wave_hash{};
  std::string source_config_path{};
  Board board{};
};

[[nodiscard]] inline bool has_non_ws_text(std::string_view s) {
  for (const unsigned char c : s) {
    if (!std::isspace(c)) return true;
  }
  return false;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::tsiemene_board_bind_decl_t*
find_bind_by_id(const cuwacunu::camahjucunu::tsiemene_board_instruction_t& instruction,
                const std::string& binding_id) {
  for (const auto& bind : instruction.binds) {
    if (bind.id == binding_id) return &bind;
  }
  return nullptr;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::tsiemene_board_contract_decl_t*
find_contract_decl_by_id(
    const cuwacunu::camahjucunu::tsiemene_board_instruction_t& instruction,
    const std::string& contract_id) {
  for (const auto& decl : instruction.contracts) {
    if (decl.id == contract_id) return &decl;
  }
  return nullptr;
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline board_contract_init_record_t invoke_board_contract_init_from_snapshot(
    const cuwacunu::iitepi::board_hash_t& board_hash,
    const std::string& board_binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::board_record_t>& board_itself,
    torch::Device device = torch::kCPU) {
  board_contract_init_record_t out{};
  if (!board_itself) {
    out.error = "missing board record";
    return out;
  }
  out.board_hash = board_hash;
  out.board_binding_id = board_binding_id;
  out.source_config_path = board_itself->config_file_path;

  try {
    const auto& board_instruction = board_itself->board.decoded();
    const auto* bind = find_bind_by_id(board_instruction, board_binding_id);
    if (!bind) {
      out.error = "unknown board binding id: " + board_binding_id;
      return out;
    }
    const auto* contract_decl =
        find_contract_decl_by_id(board_instruction, bind->contract_ref);
    if (!contract_decl) {
      out.error = "binding references unknown CONTRACT id: " + bind->contract_ref;
      return out;
    }

    const std::string contract_path = (std::filesystem::path(board_itself->config_folder) /
                                       contract_decl->file)
                                          .string();

    out.contract_hash =
        cuwacunu::iitepi::contract_space_t::register_contract_file(
            contract_path);
    out.wave_hash =
        cuwacunu::iitepi::board_space_t::wave_hash_for_binding(
            board_hash, board_binding_id);
    cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(
        out.contract_hash);
    cuwacunu::iitepi::wave_space_t::assert_intact_or_fail_fast(
        out.wave_hash);

    const auto contract_itself =
        cuwacunu::iitepi::contract_space_t::contract_itself(out.contract_hash);
    const auto wave_itself =
        cuwacunu::iitepi::wave_space_t::wave_itself(out.wave_hash);

    if (!has_non_ws_text(contract_itself->observation.sources.dsl)) {
      out.error = "missing observation sources DSL text in contract";
      return out;
    }
    if (!has_non_ws_text(contract_itself->observation.channels.dsl)) {
      out.error = "missing observation channels DSL text in contract";
      return out;
    }
    if (!has_non_ws_text(contract_itself->jkimyei.dsl)) {
      out.error = "missing jkimyei specs DSL text in contract";
      return out;
    }
    if (!has_non_ws_text(contract_itself->circuit.dsl)) {
      out.error = "missing tsiemene circuit DSL text in contract";
      return out;
    }
    if (!has_non_ws_text(wave_itself->wave.dsl)) {
      out.error = "missing tsiemene wave DSL text in bound wave file";
      return out;
    }
    if (!has_non_ws_text(contract_itself->circuit.grammar)) {
      out.error = "missing tsiemene circuit grammar text in contract";
      return out;
    }

    const auto& parsed = contract_itself->circuit.decoded();
    std::string build_error;
    if (!board_builder::build_runtime_board_from_instruction<Datatype_t, Sampler_t>(
            parsed,
            device,
            out.contract_hash,
            contract_itself,
            out.wave_hash,
            wave_itself,
            bind->wave_ref,
            &out.board,
            &build_error)) {
      out.error = "failed to build runtime board: " + build_error;
      return out;
    }
    out.board.board_hash = out.board_hash;
    out.board.board_path = board_itself->config_file_path;
    out.board.board_binding_id = out.board_binding_id;
    out.board.contract_hash = out.contract_hash;
    out.board.wave_hash = out.wave_hash;

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
    const std::string& board_file_path,
    const std::string& board_binding_id,
    torch::Device device = torch::kCPU) {
  board_contract_init_record_t out{};

  try {
    const auto board_hash =
        cuwacunu::iitepi::board_space_t::register_board_file(
            board_file_path);
    cuwacunu::iitepi::board_space_t::assert_intact_or_fail_fast(board_hash);
    const auto board_itself =
        cuwacunu::iitepi::board_space_t::board_itself(board_hash);
    return invoke_board_contract_init_from_snapshot<Datatype_t, Sampler_t>(
        board_hash, board_binding_id, board_itself, device);
  } catch (const std::exception& e) {
    out.error = std::string(kBoardContractInitCanonicalAction) + " exception: " + e.what();
    return out;
  } catch (...) {
    out.error = std::string(kBoardContractInitCanonicalAction) + " exception: unknown";
    return out;
  }
}

}  // namespace tsiemene
