// ./include/iitepi/board.contract.init.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cctype>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include <torch/torch.h>

#include "hero/hero_runtime_lock.h"
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

[[nodiscard]] inline const char* board_log_value_or_empty(
    const std::string& value) {
  return value.empty() ? "<empty>" : value.c_str();
}

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

[[nodiscard]] inline const cuwacunu::camahjucunu::iitepi_wave_t*
find_wave_by_id(
    const cuwacunu::camahjucunu::iitepi_wave_set_t& wave_set,
    const std::string& wave_id) {
  for (const auto& wave : wave_set.waves) {
    if (wave.name == wave_id) return &wave;
  }
  return nullptr;
}

[[nodiscard]] inline std::mutex& board_binding_runtime_mutex() {
  static std::mutex mutex{};
  return mutex;
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

[[nodiscard]] inline std::uint64_t board_now_ms_utc() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

[[nodiscard]] inline std::string make_board_contract_run_id(
    const board_binding_run_record_t& run_record,
    std::size_t contract_index) {
  std::ostringstream out;
  out << "run."
      << board_init_trim_ascii_copy(run_record.board_hash) << "."
      << board_init_trim_ascii_copy(run_record.board_binding_id) << "."
      << contract_index << "."
      << board_now_ms_utc();
  return out.str();
}

[[nodiscard]] inline bool resolve_active_record_type_from_observation(
    const cuwacunu::camahjucunu::observation_spec_t& observation,
    std::string* out_record_type,
    std::string* error) {
  if (!out_record_type) {
    if (error) {
      *error = "missing output pointer while resolving observation record_type";
    }
    return false;
  }
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
  // Finalize the init snapshot into one immutable run record.
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
    log_err(
        "[board.binding.run] init failed board_hash=%s binding=%s error=%s\n",
        board_log_value_or_empty(out.board_hash),
        board_log_value_or_empty(out.board_binding_id),
        board_log_value_or_empty(out.error));
    return out;
  }

  out.contract_hash = std::move(init.contract_hash);
  out.wave_hash = std::move(init.wave_hash);
  out.resolved_record_type = std::move(init.resolved_record_type);
  out.resolved_sampler = std::move(init.resolved_sampler);
  out.board = std::move(init.board);
  out.contract_steps.reserve(out.board.contracts.size());
  log_info(
      "[board.binding.run] start board_hash=%s binding=%s contracts=%zu record_type=%s sampler=%s\n",
      board_log_value_or_empty(out.board_hash),
      board_log_value_or_empty(out.board_binding_id),
      out.board.contracts.size(),
      board_log_value_or_empty(out.resolved_record_type),
      board_log_value_or_empty(out.resolved_sampler));

  // Execution phase: contracts are evaluated in board declaration order.
  for (std::size_t i = 0; i < out.board.contracts.size(); ++i) {
    auto& contract = out.board.contracts[i];
    log_dbg(
        "[board.binding.run] contract[%zu] begin name=%s epochs=%llu\n",
        i,
        board_log_value_or_empty(contract.name),
        static_cast<unsigned long long>(
            std::max<std::uint64_t>(1, contract.execution.epochs)));
    BoardContext ctx{};
    ctx.wave_mode_flags = contract.execution.wave_mode_flags;
    ctx.debug_enabled = contract.execution.debug_enabled;
    ctx.run_id = make_board_contract_run_id(out, i);
    std::string run_error;
    const std::uint64_t steps = run_contract(contract, ctx, &run_error);
    if (!run_error.empty()) {
      out.error = "run_contract failed for contract[" + std::to_string(i) +
                  "]: " + run_error;
      log_err(
          "[board.binding.run] contract[%zu] failed name=%s error=%s\n",
          i,
          board_log_value_or_empty(contract.name),
          board_log_value_or_empty(out.error));
      return out;
    }
    out.contract_steps.push_back(steps);
    out.total_steps += steps;
    log_info(
        "[board.binding.run] contract[%zu] done steps=%llu cumulative_steps=%llu\n",
        i,
        static_cast<unsigned long long>(steps),
        static_cast<unsigned long long>(out.total_steps));
  }

  // Finalization: publish completed run counters.
  out.ok = true;
  log_info(
      "[board.binding.run] done board_hash=%s binding=%s total_steps=%llu contracts=%zu\n",
      board_log_value_or_empty(out.board_hash),
      board_log_value_or_empty(out.board_binding_id),
      static_cast<unsigned long long>(out.total_steps),
      out.contract_steps.size());
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
    log_err(
        "[board.contract.init] missing board record board_hash=%s binding=%s\n",
        board_log_value_or_empty(board_hash),
        board_log_value_or_empty(board_binding_id));
    return out;
  }
  out.board_hash = board_hash;
  out.board_binding_id = board_binding_id;
  out.source_config_path = board_itself->config_file_path;
  log_info(
      "[board.contract.init] start board_hash=%s binding=%s device=%s\n",
      board_log_value_or_empty(out.board_hash),
      board_log_value_or_empty(out.board_binding_id),
      device.str().c_str());

  try {
    // Initialization preflight: bind contract/wave identity from board snapshot.
    const auto& board_instruction = board_itself->board.decoded();
    const auto* bind = find_bind_by_id(board_instruction, board_binding_id);
    if (!bind) {
      out.error = "unknown board binding id: " + board_binding_id;
      log_err(
          "[board.contract.init] binding lookup failed board_hash=%s binding=%s\n",
          board_log_value_or_empty(out.board_hash),
          board_log_value_or_empty(out.board_binding_id));
      return out;
    }
    const auto* contract_decl =
        find_contract_decl_by_id(board_instruction, bind->contract_ref);
    if (!contract_decl) {
      out.error = "binding references unknown CONTRACT id: " + bind->contract_ref;
      log_err(
          "[board.contract.init] missing CONTRACT ref=%s board_hash=%s binding=%s\n",
          board_log_value_or_empty(bind->contract_ref),
          board_log_value_or_empty(out.board_hash),
          board_log_value_or_empty(out.board_binding_id));
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
    log_info(
        "[board.contract.init] resolved contract_hash=%s wave_hash=%s\n",
        board_log_value_or_empty(out.contract_hash),
        board_log_value_or_empty(out.wave_hash));
    cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(
        out.contract_hash);
    cuwacunu::iitepi::wave_space_t::assert_intact_or_fail_fast(
        out.wave_hash);

    const auto contract_itself =
        cuwacunu::iitepi::contract_space_t::contract_itself(out.contract_hash);
    const auto wave_itself =
        cuwacunu::iitepi::wave_space_t::wave_itself(out.wave_hash);

    if (!has_non_ws_text(contract_itself->circuit.dsl)) {
      out.error = "missing tsiemene circuit DSL text in contract";
      return out;
    }
    if (!has_non_ws_text(wave_itself->wave.dsl)) {
      out.error = "missing iitepi wave DSL text in bound wave file";
      return out;
    }
    if (!has_non_ws_text(contract_itself->circuit.grammar)) {
      out.error = "missing tsiemene circuit grammar text in contract";
      return out;
    }

    // Build runtime board payload from validated contract + wave DSLs.
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

    // Final validation before exposing initialized runtime board.
    BoardIssue issue{};
    if (!validate_board(out.board, &issue)) {
      out.error = "invalid runtime board: " + std::string(issue.circuit_issue.what);
      log_err(
          "[board.contract.init] board validation failed board_hash=%s binding=%s error=%s\n",
          board_log_value_or_empty(out.board_hash),
          board_log_value_or_empty(out.board_binding_id),
          board_log_value_or_empty(out.error));
      return out;
    }

    out.ok = true;
    log_info(
        "[board.contract.init] done board_hash=%s binding=%s contracts=%zu\n",
        board_log_value_or_empty(out.board_hash),
        board_log_value_or_empty(out.board_binding_id),
        out.board.contracts.size());
    return out;
  } catch (const std::exception& e) {
    out.error = std::string(kBoardContractInitCanonicalAction) + " exception: " + e.what();
    log_err(
        "[board.contract.init] exception board_hash=%s binding=%s error=%s\n",
        board_log_value_or_empty(out.board_hash),
        board_log_value_or_empty(out.board_binding_id),
        board_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    out.error = std::string(kBoardContractInitCanonicalAction) + " exception: unknown";
    log_err(
        "[board.contract.init] exception board_hash=%s binding=%s error=%s\n",
        board_log_value_or_empty(out.board_hash),
        board_log_value_or_empty(out.board_binding_id),
        board_log_value_or_empty(out.error));
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
  log_info(
      "[board.contract.init] infer runtime board_hash=%s binding=%s device=%s\n",
      board_log_value_or_empty(out.board_hash),
      board_log_value_or_empty(out.board_binding_id),
      device.str().c_str());

  try {
    if (!board_itself) {
      out.error = "missing board record";
      log_err(
          "[board.contract.init] missing board record board_hash=%s binding=%s\n",
          board_log_value_or_empty(out.board_hash),
          board_log_value_or_empty(out.board_binding_id));
      return out;
    }
    const auto& board_instruction = board_itself->board.decoded();
    const auto* bind = find_bind_by_id(board_instruction, board_binding_id);
    if (!bind) {
      out.error = "unknown board binding id: " + board_binding_id;
      log_err(
          "[board.contract.init] binding lookup failed board_hash=%s binding=%s\n",
          board_log_value_or_empty(out.board_hash),
          board_log_value_or_empty(out.board_binding_id));
      return out;
    }
    const auto* contract_decl =
        find_contract_decl_by_id(board_instruction, bind->contract_ref);
    if (!contract_decl) {
      out.error = "binding references unknown CONTRACT id: " + bind->contract_ref;
      log_err(
          "[board.contract.init] missing CONTRACT ref=%s board_hash=%s binding=%s\n",
          board_log_value_or_empty(bind->contract_ref),
          board_log_value_or_empty(out.board_hash),
          board_log_value_or_empty(out.board_binding_id));
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
    const auto wave_itself =
        cuwacunu::iitepi::wave_space_t::wave_itself(wave_hash);

    const auto& wave_set = wave_itself->wave.decoded();
    const auto* wave = find_wave_by_id(wave_set, bind->wave_ref);
    if (!wave) {
      out.error = "binding references unknown WAVE id: " + bind->wave_ref;
      log_err(
          "[board.contract.init] missing WAVE ref=%s board_hash=%s binding=%s\n",
          board_log_value_or_empty(bind->wave_ref),
          board_log_value_or_empty(out.board_hash),
          board_log_value_or_empty(out.board_binding_id));
      return out;
    }

    cuwacunu::camahjucunu::observation_spec_t effective_observation{};
    if (!board_builder::load_wave_dataloader_observation_payloads(
            wave_itself,
            *wave,
            nullptr,
            nullptr,
            &effective_observation,
            &out.error)) {
      return out;
    }

    std::string inferred_record_type;
    if (!resolve_active_record_type_from_observation(
            effective_observation, &inferred_record_type, &out.error)) {
      return out;
    }

    std::string inferred_sampler;
    if (!resolve_binding_wave_sampler(
            board_itself, *bind, wave_hash, &inferred_sampler, &out.error)) {
      return out;
    }

    out.resolved_record_type = inferred_record_type;
    out.resolved_sampler = inferred_sampler;
    log_info(
        "[board.contract.init] resolved runtime board_hash=%s binding=%s record_type=%s sampler=%s\n",
        board_log_value_or_empty(out.board_hash),
        board_log_value_or_empty(out.board_binding_id),
        board_log_value_or_empty(out.resolved_record_type),
        board_log_value_or_empty(out.resolved_sampler));

    if (inferred_record_type == "kline") {
      if (inferred_sampler == "sequential") {
        auto typed = invoke_board_contract_init_from_snapshot<
            cuwacunu::camahjucunu::exchange::kline_t,
            torch::data::samplers::SequentialSampler>(
            board_hash, board_binding_id, board_itself, device);
        typed.resolved_record_type = inferred_record_type;
        typed.resolved_sampler = inferred_sampler;
        if (!typed.ok) {
          log_err(
              "[board.contract.init] typed init failed board_hash=%s binding=%s error=%s\n",
              board_log_value_or_empty(typed.board_hash),
              board_log_value_or_empty(typed.board_binding_id),
              board_log_value_or_empty(typed.error));
        }
        return typed;
      }
      if (inferred_sampler == "random") {
        auto typed = invoke_board_contract_init_from_snapshot<
            cuwacunu::camahjucunu::exchange::kline_t,
            torch::data::samplers::RandomSampler>(
            board_hash, board_binding_id, board_itself, device);
        typed.resolved_record_type = inferred_record_type;
        typed.resolved_sampler = inferred_sampler;
        if (!typed.ok) {
          log_err(
              "[board.contract.init] typed init failed board_hash=%s binding=%s error=%s\n",
              board_log_value_or_empty(typed.board_hash),
              board_log_value_or_empty(typed.board_binding_id),
              board_log_value_or_empty(typed.error));
        }
        return typed;
      }
    }

    out.error = "unsupported runtime combination record_type='" +
                inferred_record_type + "' sampler='" + inferred_sampler +
                "' (supported now: kline + sequential|random)";
    log_err(
        "[board.contract.init] unsupported runtime board_hash=%s binding=%s error=%s\n",
        board_log_value_or_empty(out.board_hash),
        board_log_value_or_empty(out.board_binding_id),
        board_log_value_or_empty(out.error));
    return out;
  } catch (const std::exception& e) {
    out.error = std::string(kBoardContractInitCanonicalAction) + " exception: " + e.what();
    log_err(
        "[board.contract.init] exception board_hash=%s binding=%s error=%s\n",
        board_log_value_or_empty(out.board_hash),
        board_log_value_or_empty(out.board_binding_id),
        board_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    out.error = std::string(kBoardContractInitCanonicalAction) + " exception: unknown";
    log_err(
        "[board.contract.init] exception board_hash=%s binding=%s error=%s\n",
        board_log_value_or_empty(out.board_hash),
        board_log_value_or_empty(out.board_binding_id),
        board_log_value_or_empty(out.error));
    return out;
  }
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline board_binding_run_record_t run_binding_snapshot(
    const cuwacunu::iitepi::board_hash_t& board_hash,
    const std::string& board_binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::board_record_t>& board_itself,
    torch::Device device = torch::kCPU) {
  log_info(
      "[board.binding.run] snapshot start board_hash=%s binding=%s\n",
      board_log_value_or_empty(board_hash),
      board_log_value_or_empty(board_binding_id));
  try {
    auto out = run_initialized_board_binding(
        invoke_board_contract_init_from_snapshot<Datatype_t, Sampler_t>(
            board_hash, board_binding_id, board_itself, device));
    if (!out.ok) {
      log_err(
          "[board.binding.run] snapshot failed board_hash=%s binding=%s error=%s\n",
          board_log_value_or_empty(out.board_hash),
          board_log_value_or_empty(out.board_binding_id),
          board_log_value_or_empty(out.error));
    }
    return out;
  } catch (const std::exception& e) {
    board_binding_run_record_t out{};
    out.board_hash = board_hash;
    out.board_binding_id = board_binding_id;
    if (board_itself) out.source_config_path = board_itself->config_file_path;
    out.error = std::string(kBoardBindingRunCanonicalAction) + " exception: " +
                e.what();
    log_err(
        "[board.binding.run] snapshot exception board_hash=%s binding=%s error=%s\n",
        board_log_value_or_empty(out.board_hash),
        board_log_value_or_empty(out.board_binding_id),
        board_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    board_binding_run_record_t out{};
    out.board_hash = board_hash;
    out.board_binding_id = board_binding_id;
    if (board_itself) out.source_config_path = board_itself->config_file_path;
    out.error =
        std::string(kBoardBindingRunCanonicalAction) + " exception: unknown";
    log_err(
        "[board.binding.run] snapshot exception board_hash=%s binding=%s error=%s\n",
        board_log_value_or_empty(out.board_hash),
        board_log_value_or_empty(out.board_binding_id),
        board_log_value_or_empty(out.error));
    return out;
  }
}

[[nodiscard]] inline board_binding_run_record_t run_binding_snapshot(
    const cuwacunu::iitepi::board_hash_t& board_hash,
    const std::string& board_binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::board_record_t>& board_itself,
    torch::Device device = torch::kCPU) {
  log_info(
      "[board.binding.run] snapshot start board_hash=%s binding=%s\n",
      board_log_value_or_empty(board_hash),
      board_log_value_or_empty(board_binding_id));
  try {
    auto out = run_initialized_board_binding(invoke_board_contract_init_from_snapshot(
        board_hash, board_binding_id, board_itself, device));
    if (!out.ok) {
      log_err(
          "[board.binding.run] snapshot failed board_hash=%s binding=%s error=%s\n",
          board_log_value_or_empty(out.board_hash),
          board_log_value_or_empty(out.board_binding_id),
          board_log_value_or_empty(out.error));
    }
    return out;
  } catch (const std::exception& e) {
    board_binding_run_record_t out{};
    out.board_hash = board_hash;
    out.board_binding_id = board_binding_id;
    if (board_itself) out.source_config_path = board_itself->config_file_path;
    out.error = std::string(kBoardBindingRunCanonicalAction) + " exception: " +
                e.what();
    log_err(
        "[board.binding.run] snapshot exception board_hash=%s binding=%s error=%s\n",
        board_log_value_or_empty(out.board_hash),
        board_log_value_or_empty(out.board_binding_id),
        board_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    board_binding_run_record_t out{};
    out.board_hash = board_hash;
    out.board_binding_id = board_binding_id;
    if (board_itself) out.source_config_path = board_itself->config_file_path;
    out.error =
        std::string(kBoardBindingRunCanonicalAction) + " exception: unknown";
    log_err(
        "[board.binding.run] snapshot exception board_hash=%s binding=%s error=%s\n",
        board_log_value_or_empty(out.board_hash),
        board_log_value_or_empty(out.board_binding_id),
        board_log_value_or_empty(out.error));
    return out;
  }
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline board_binding_run_record_t run_binding(
    const std::string& board_binding_id,
    torch::Device device = torch::kCPU) {
  board_binding_run_record_t out{};
  try {
    // Input normalization before hitting runtime lock and snapshots.
    const std::string binding_id = board_init_trim_ascii_copy(board_binding_id);
    if (!has_non_ws_text(binding_id)) {
      out.error = "run_binding requires non-empty board_binding_id";
      log_err("[board.binding.run] invalid request error=%s\n",
              board_log_value_or_empty(out.error));
      return out;
    }
    const auto board_hash = cuwacunu::iitepi::board_space_t::locked_board_hash();
    log_info(
        "[board.binding.run] dispatch board_hash=%s binding=%s\n",
        board_log_value_or_empty(board_hash),
        board_log_value_or_empty(binding_id));
    const auto board_itself =
        cuwacunu::iitepi::board_space_t::board_itself(board_hash);
    std::lock_guard<std::mutex> run_guard(board_binding_runtime_mutex());
    cuwacunu::hero::runtime_lock::scoped_lock_t global_runtime_lock{};
    std::string global_lock_error;
    if (!cuwacunu::hero::runtime_lock::acquire(&global_runtime_lock,
                                               &global_lock_error)) {
      out.error =
          "failed to acquire global runtime lock: " + global_lock_error;
      log_err(
          "[board.binding.run] global lock failed board_hash=%s binding=%s error=%s\n",
          board_log_value_or_empty(board_hash),
          board_log_value_or_empty(binding_id),
          board_log_value_or_empty(out.error));
      return out;
    }
    log_info(
        "[board.binding.run] runtime lock acquired board_hash=%s binding=%s lock_path=%s\n",
        board_log_value_or_empty(board_hash),
        board_log_value_or_empty(binding_id),
        board_log_value_or_empty(global_runtime_lock.path));
    return run_binding_snapshot<Datatype_t, Sampler_t>(
        board_hash, binding_id, board_itself, device);
  } catch (const std::exception& e) {
    out.error = std::string(kBoardBindingRunCanonicalAction) + " exception: " +
                e.what();
    log_err("[board.binding.run] dispatch exception error=%s\n",
            board_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    out.error =
        std::string(kBoardBindingRunCanonicalAction) + " exception: unknown";
    log_err("[board.binding.run] dispatch exception error=%s\n",
            board_log_value_or_empty(out.error));
    return out;
  }
}

[[nodiscard]] inline board_binding_run_record_t run_binding(
    const std::string& board_binding_id,
    torch::Device device = torch::kCPU) {
  board_binding_run_record_t out{};
  try {
    // Input normalization before hitting runtime lock and snapshots.
    const std::string binding_id = board_init_trim_ascii_copy(board_binding_id);
    if (!has_non_ws_text(binding_id)) {
      out.error = "run_binding requires non-empty board_binding_id";
      log_err("[board.binding.run] invalid request error=%s\n",
              board_log_value_or_empty(out.error));
      return out;
    }
    const auto board_hash = cuwacunu::iitepi::board_space_t::locked_board_hash();
    log_info(
        "[board.binding.run] dispatch board_hash=%s binding=%s\n",
        board_log_value_or_empty(board_hash),
        board_log_value_or_empty(binding_id));
    const auto board_itself =
        cuwacunu::iitepi::board_space_t::board_itself(board_hash);
    std::lock_guard<std::mutex> run_guard(board_binding_runtime_mutex());
    cuwacunu::hero::runtime_lock::scoped_lock_t global_runtime_lock{};
    std::string global_lock_error;
    if (!cuwacunu::hero::runtime_lock::acquire(&global_runtime_lock,
                                               &global_lock_error)) {
      out.error =
          "failed to acquire global runtime lock: " + global_lock_error;
      log_err(
          "[board.binding.run] global lock failed board_hash=%s binding=%s error=%s\n",
          board_log_value_or_empty(board_hash),
          board_log_value_or_empty(binding_id),
          board_log_value_or_empty(out.error));
      return out;
    }
    log_info(
        "[board.binding.run] runtime lock acquired board_hash=%s binding=%s lock_path=%s\n",
        board_log_value_or_empty(board_hash),
        board_log_value_or_empty(binding_id),
        board_log_value_or_empty(global_runtime_lock.path));
    return run_binding_snapshot(
        board_hash, binding_id, board_itself, device);
  } catch (const std::exception& e) {
    out.error = std::string(kBoardBindingRunCanonicalAction) + " exception: " +
                e.what();
    log_err("[board.binding.run] dispatch exception error=%s\n",
            board_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    out.error =
        std::string(kBoardBindingRunCanonicalAction) + " exception: unknown";
    log_err("[board.binding.run] dispatch exception error=%s\n",
            board_log_value_or_empty(out.error));
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

namespace cuwacunu::iitepi {

using board_contract_init_record_t = ::tsiemene::board_contract_init_record_t;
using board_binding_run_record_t = ::tsiemene::board_binding_run_record_t;

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline board_binding_run_record_t run_binding(
    const std::string& board_binding_id,
    torch::Device device = torch::kCPU) {
  return ::tsiemene::run_binding<Datatype_t, Sampler_t>(
      board_binding_id, device);
}

[[nodiscard]] inline board_binding_run_record_t run_binding(
    const std::string& board_binding_id,
    torch::Device device = torch::kCPU) {
  return ::tsiemene::run_binding(board_binding_id, device);
}

}  // namespace cuwacunu::iitepi
