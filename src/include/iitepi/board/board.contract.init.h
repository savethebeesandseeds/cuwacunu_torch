// ./include/iitepi/board.contract.init.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cctype>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <torch/torch.h>

#include "camahjucunu/dsl/tsiemene_board/tsiemene_board.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "camahjucunu/types/types_data.h"
#include "iitepi/iitepi.h"
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
  std::string resolved_record_type{};
  std::string resolved_sampler{};
  std::string source_config_path{};
  Board board{};
};

inline constexpr const char* kBoardBindingRunCanonicalAction =
    "board.binding@run";

struct board_binding_run_record_t {
  bool ok{false};
  std::string error{};
  std::string canonical_action{kBoardBindingRunCanonicalAction};
  std::string board_hash{};
  std::string board_binding_id{};
  std::string contract_hash{};
  std::string wave_hash{};
  std::string resolved_record_type{};
  std::string resolved_sampler{};
  std::string source_config_path{};
  std::uint64_t total_steps{0};
  std::vector<std::uint64_t> contract_steps{};
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

[[nodiscard]] inline const cuwacunu::camahjucunu::tsiemene_wave_t*
find_wave_by_id(
    const cuwacunu::camahjucunu::tsiemene_wave_set_t& wave_set,
    const std::string& wave_id) {
  for (const auto& wave : wave_set.waves) {
    if (wave.name == wave_id) return &wave;
  }
  return nullptr;
}

[[nodiscard]] inline std::string board_init_trim_ascii_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] inline std::string board_init_lower_ascii_copy(std::string s) {
  for (char& ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
}

[[nodiscard]] inline bool board_init_parse_bool_ascii(std::string value, bool* out) {
  if (!out) return false;
  value = board_init_lower_ascii_copy(board_init_trim_ascii_copy(std::move(value)));
  if (value == "1" || value == "true" || value == "yes" || value == "on") {
    *out = true;
    return true;
  }
  if (value == "0" || value == "false" || value == "no" || value == "off") {
    *out = false;
    return true;
  }
  return false;
}

[[nodiscard]] inline bool resolve_contract_active_record_type(
    const std::shared_ptr<const cuwacunu::iitepi::contract_record_t>& contract_itself,
    std::string* out_record_type,
    std::string* error) {
  if (!contract_itself || !out_record_type) {
    if (error) *error = "missing contract record while resolving observation record_type";
    return false;
  }
  const auto observation = contract_itself->observation.decoded();
  std::unordered_set<std::string> active_types{};
  for (const auto& ch : observation.channel_forms) {
    bool active = false;
    if (!board_init_parse_bool_ascii(ch.active, &active)) {
      if (error) {
        *error = "invalid observation channel active flag '" + ch.active +
                 "' for interval '" +
                 cuwacunu::camahjucunu::exchange::enum_to_string(ch.interval) + "'";
      }
      return false;
    }
    if (!active) continue;
    const std::string record = board_init_lower_ascii_copy(
        board_init_trim_ascii_copy(ch.record_type));
    if (record.empty()) {
      if (error) {
        *error = "active observation channel has empty record_type for interval '" +
                 cuwacunu::camahjucunu::exchange::enum_to_string(ch.interval) + "'";
      }
      return false;
    }
    active_types.insert(record);
  }

  if (active_types.empty()) {
    if (error) *error = "no active observation channels found";
    return false;
  }
  if (active_types.size() != 1) {
    std::string joined;
    for (const auto& item : active_types) {
      if (!joined.empty()) joined += ", ";
      joined += item;
    }
    if (error) {
      *error = "active observation channels must share one record_type; found: " +
               joined;
    }
    return false;
  }

  *out_record_type = *active_types.begin();
  return true;
}

[[nodiscard]] inline bool resolve_binding_wave_sampler(
    const std::shared_ptr<const cuwacunu::iitepi::board_record_t>& board_itself,
    const cuwacunu::camahjucunu::tsiemene_board_bind_decl_t& bind,
    const std::string& wave_hash,
    std::string* out_sampler,
    std::string* error) {
  if (!board_itself || !out_sampler) {
    if (error) *error = "missing board record while resolving wave sampler";
    return false;
  }
  const auto wave_itself = cuwacunu::iitepi::wave_space_t::wave_itself(wave_hash);
  const auto& wave_set = wave_itself->wave.decoded();
  const auto* wave = find_wave_by_id(wave_set, bind.wave_ref);
  if (!wave) {
    if (error) *error = "binding references unknown WAVE id: " + bind.wave_ref;
    return false;
  }
  const std::string sampler = board_init_lower_ascii_copy(
      board_init_trim_ascii_copy(wave->sampler));
  if (sampler != "sequential" && sampler != "random") {
    if (error) {
      *error = "unsupported wave sampler '" + wave->sampler +
               "' (expected sequential|random)";
    }
    return false;
  }
  *out_sampler = sampler;
  return true;
}

[[nodiscard]] inline board_binding_run_record_t run_initialized_board_binding(
    board_contract_init_record_t init) {
  board_binding_run_record_t out{};
  out.board_hash = init.board_hash;
  out.board_binding_id = init.board_binding_id;
  out.source_config_path = init.source_config_path;

  if (!init.ok) {
    out.error = std::move(init.error);
    out.contract_hash = std::move(init.contract_hash);
    out.wave_hash = std::move(init.wave_hash);
    out.resolved_record_type = std::move(init.resolved_record_type);
    out.resolved_sampler = std::move(init.resolved_sampler);
    return out;
  }

  out.contract_hash = std::move(init.contract_hash);
  out.wave_hash = std::move(init.wave_hash);
  out.resolved_record_type = std::move(init.resolved_record_type);
  out.resolved_sampler = std::move(init.resolved_sampler);
  out.board = std::move(init.board);
  out.contract_steps.reserve(out.board.contracts.size());

  for (std::size_t i = 0; i < out.board.contracts.size(); ++i) {
    auto& contract = out.board.contracts[i];
    BoardContext ctx{};
    std::string run_error;
    const std::uint64_t steps = run_contract(contract, ctx, &run_error);
    if (!run_error.empty()) {
      out.error = "run_contract failed for contract[" + std::to_string(i) +
                  "]: " + run_error;
      return out;
    }
    out.contract_steps.push_back(steps);
    out.total_steps += steps;
  }

  out.ok = true;
  return out;
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

[[nodiscard]] inline board_contract_init_record_t invoke_board_contract_init_from_snapshot(
    const cuwacunu::iitepi::board_hash_t& board_hash,
    const std::string& board_binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::board_record_t>& board_itself,
    torch::Device device = torch::kCPU) {
  board_contract_init_record_t out{};
  out.board_hash = board_hash;
  out.board_binding_id = board_binding_id;
  if (board_itself) out.source_config_path = board_itself->config_file_path;

  try {
    if (!board_itself) {
      out.error = "missing board record";
      return out;
    }
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
    const auto contract_hash =
        cuwacunu::iitepi::contract_space_t::register_contract_file(contract_path);
    const auto wave_hash =
        cuwacunu::iitepi::board_space_t::wave_hash_for_binding(board_hash, board_binding_id);
    cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(contract_hash);
    cuwacunu::iitepi::wave_space_t::assert_intact_or_fail_fast(wave_hash);
    const auto contract_itself =
        cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);

    std::string inferred_record_type;
    if (!resolve_contract_active_record_type(
            contract_itself, &inferred_record_type, &out.error)) {
      return out;
    }

    std::string inferred_sampler;
    if (!resolve_binding_wave_sampler(
            board_itself, *bind, wave_hash, &inferred_sampler, &out.error)) {
      return out;
    }

    out.resolved_record_type = inferred_record_type;
    out.resolved_sampler = inferred_sampler;

    if (inferred_record_type == "kline") {
      if (inferred_sampler == "sequential") {
        auto typed = invoke_board_contract_init_from_snapshot<
            cuwacunu::camahjucunu::exchange::kline_t,
            torch::data::samplers::SequentialSampler>(
            board_hash, board_binding_id, board_itself, device);
        typed.resolved_record_type = inferred_record_type;
        typed.resolved_sampler = inferred_sampler;
        return typed;
      }
      if (inferred_sampler == "random") {
        auto typed = invoke_board_contract_init_from_snapshot<
            cuwacunu::camahjucunu::exchange::kline_t,
            torch::data::samplers::RandomSampler>(
            board_hash, board_binding_id, board_itself, device);
        typed.resolved_record_type = inferred_record_type;
        typed.resolved_sampler = inferred_sampler;
        return typed;
      }
    }

    out.error = "unsupported runtime combination record_type='" +
                inferred_record_type + "' sampler='" + inferred_sampler +
                "' (supported now: kline + sequential|random)";
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
[[nodiscard]] inline board_binding_run_record_t invoke_board_binding_run_from_snapshot(
    const cuwacunu::iitepi::board_hash_t& board_hash,
    const std::string& board_binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::board_record_t>& board_itself,
    torch::Device device = torch::kCPU) {
  try {
    return run_initialized_board_binding(
        invoke_board_contract_init_from_snapshot<Datatype_t, Sampler_t>(
            board_hash, board_binding_id, board_itself, device));
  } catch (const std::exception& e) {
    board_binding_run_record_t out{};
    out.board_hash = board_hash;
    out.board_binding_id = board_binding_id;
    if (board_itself) out.source_config_path = board_itself->config_file_path;
    out.error = std::string(kBoardBindingRunCanonicalAction) + " exception: " +
                e.what();
    return out;
  } catch (...) {
    board_binding_run_record_t out{};
    out.board_hash = board_hash;
    out.board_binding_id = board_binding_id;
    if (board_itself) out.source_config_path = board_itself->config_file_path;
    out.error =
        std::string(kBoardBindingRunCanonicalAction) + " exception: unknown";
    return out;
  }
}

[[nodiscard]] inline board_binding_run_record_t invoke_board_binding_run_from_snapshot(
    const cuwacunu::iitepi::board_hash_t& board_hash,
    const std::string& board_binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::board_record_t>& board_itself,
    torch::Device device = torch::kCPU) {
  try {
    return run_initialized_board_binding(invoke_board_contract_init_from_snapshot(
        board_hash, board_binding_id, board_itself, device));
  } catch (const std::exception& e) {
    board_binding_run_record_t out{};
    out.board_hash = board_hash;
    out.board_binding_id = board_binding_id;
    if (board_itself) out.source_config_path = board_itself->config_file_path;
    out.error = std::string(kBoardBindingRunCanonicalAction) + " exception: " +
                e.what();
    return out;
  } catch (...) {
    board_binding_run_record_t out{};
    out.board_hash = board_hash;
    out.board_binding_id = board_binding_id;
    if (board_itself) out.source_config_path = board_itself->config_file_path;
    out.error =
        std::string(kBoardBindingRunCanonicalAction) + " exception: unknown";
    return out;
  }
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline board_binding_run_record_t invoke_board_binding_run_from_locked_runtime(
    std::optional<std::string> board_binding_id = std::nullopt,
    torch::Device device = torch::kCPU) {
  board_binding_run_record_t out{};
  try {
    const auto board_hash = cuwacunu::iitepi::board_space_t::locked_board_hash();
    const std::string binding_id = board_binding_id.has_value()
                                       ? *board_binding_id
                                       : cuwacunu::iitepi::board_space_t::
                                             locked_board_binding_id();
    const auto board_itself =
        cuwacunu::iitepi::board_space_t::board_itself(board_hash);
    return invoke_board_binding_run_from_snapshot<Datatype_t, Sampler_t>(
        board_hash, binding_id, board_itself, device);
  } catch (const std::exception& e) {
    out.error = std::string(kBoardBindingRunCanonicalAction) + " exception: " +
                e.what();
    return out;
  } catch (...) {
    out.error =
        std::string(kBoardBindingRunCanonicalAction) + " exception: unknown";
    return out;
  }
}

[[nodiscard]] inline board_binding_run_record_t invoke_board_binding_run_from_locked_runtime(
    std::optional<std::string> board_binding_id = std::nullopt,
    torch::Device device = torch::kCPU) {
  board_binding_run_record_t out{};
  try {
    const auto board_hash = cuwacunu::iitepi::board_space_t::locked_board_hash();
    const std::string binding_id = board_binding_id.has_value()
                                       ? *board_binding_id
                                       : cuwacunu::iitepi::board_space_t::
                                             locked_board_binding_id();
    const auto board_itself =
        cuwacunu::iitepi::board_space_t::board_itself(board_hash);
    return invoke_board_binding_run_from_snapshot(
        board_hash, binding_id, board_itself, device);
  } catch (const std::exception& e) {
    out.error = std::string(kBoardBindingRunCanonicalAction) + " exception: " +
                e.what();
    return out;
  } catch (...) {
    out.error =
        std::string(kBoardBindingRunCanonicalAction) + " exception: unknown";
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
    return invoke_board_contract_init_from_snapshot(
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
