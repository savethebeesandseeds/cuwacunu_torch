// ./include/iitepi/runtime_binding/runtime_binding.contract.init.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
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
#include "camahjucunu/dsl/runtime_binding_instruction/runtime_binding_instruction.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "camahjucunu/types/types_data.h"
#include "iitepi/iitepi.h"
#include "iitepi/runtime_binding/runtime_binding.builder.h"

namespace tsiemene {

namespace runtime_binding_action_id {
#define RUNTIME_BINDING_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define RUNTIME_BINDING_PATH_METHOD(ID, TOKEN, SUMMARY)
#define RUNTIME_BINDING_PATH_DSL_SEGMENT(ID, KEY, SUMMARY)
#define RUNTIME_BINDING_PATH_ACTION(ID, TOKEN, SUMMARY) inline constexpr const char ID[] = TOKEN;
#include "iitepi/runtime_binding/runtime_binding.paths.def"
#undef RUNTIME_BINDING_PATH_ACTION
#undef RUNTIME_BINDING_PATH_DSL_SEGMENT
#undef RUNTIME_BINDING_PATH_METHOD
#undef RUNTIME_BINDING_PATH_DIRECTIVE
}  // namespace runtime_binding_action_id

inline constexpr const char* kRuntimeBindingContractInitCanonicalAction =
    runtime_binding_action_id::ContractInit;

struct runtime_binding_contract_init_record_t {
  bool ok{false};
  std::string error{};
  std::string canonical_action{kRuntimeBindingContractInitCanonicalAction};
  std::string campaign_hash{};
  std::string binding_id{};
  std::string contract_hash{};
  std::string wave_hash{};
  std::string resolved_record_type{};
  std::string resolved_sampler{};
  std::string source_config_path{};
  RuntimeBinding runtime_binding{};
};

inline constexpr const char* kRuntimeBindingRunCanonicalAction =
    "runtime_binding.run";

struct runtime_binding_run_record_t {
  bool ok{false};
  std::string error{};
  std::string canonical_action{kRuntimeBindingRunCanonicalAction};
  std::string campaign_hash{};
  std::string binding_id{};
  std::string contract_hash{};
  std::string wave_hash{};
  std::string resolved_record_type{};
  std::string resolved_sampler{};
  std::string source_config_path{};
  std::uint64_t total_steps{0};
  std::vector<std::uint64_t> contract_steps{};
  std::vector<std::string> run_ids{};
  RuntimeBinding runtime_binding{};
};

struct runtime_binding_device_resolution_t {
  torch::Device device{torch::kCPU};
  std::string contract_hash{};
  std::string source_section{};
  std::string configured_device{};
  std::string error{};
  bool resolved_from_config{false};
};

[[nodiscard]] inline const char* runtime_binding_log_value_or_empty(
    const std::string& value) {
  return value.empty() ? "<empty>" : value.c_str();
}

[[nodiscard]] inline bool has_non_ws_text(std::string_view s) {
  for (const unsigned char c : s) {
    if (!std::isspace(c)) return true;
  }
  return false;
}

[[nodiscard]] inline std::string runtime_binding_init_trim_ascii_copy(
    std::string s);

[[nodiscard]] inline bool runtime_binding_requests_cuda_device_value(
    std::string_view raw_value) {
  std::string value(raw_value);
  value = runtime_binding_init_trim_ascii_copy(std::move(value));
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  if (value == "gpu" || value == "cuda") return true;
  if (value.rfind("gpu:", 0) == 0 || value.rfind("cuda:", 0) == 0) return true;
  if (value.rfind("torch::", 0) == 0) value = value.substr(7);
  if (value.rfind("at::", 0) == 0) value = value.substr(4);
  if (value == "kcuda") return true;
  return value.rfind("kcuda:", 0) == 0;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::runtime_binding_bind_decl_t*
find_bind_by_id(
    const cuwacunu::camahjucunu::runtime_binding_instruction_t& instruction,
    const std::string& binding_id);

[[nodiscard]] inline const cuwacunu::camahjucunu::
    runtime_binding_contract_decl_t*
find_contract_decl_by_id(
    const cuwacunu::camahjucunu::runtime_binding_instruction_t& instruction,
    const std::string& contract_id);

[[nodiscard]] inline std::optional<std::string>
runtime_binding_section_device_value(
    const std::shared_ptr<const cuwacunu::iitepi::contract_record_t>&
        contract_itself,
    std::string_view section) {
  if (!contract_itself) return std::nullopt;
  const auto sec_it =
      contract_itself->module_sections.find(std::string(section));
  if (sec_it == contract_itself->module_sections.end()) return std::nullopt;
  const auto value_it = sec_it->second.find("device");
  if (value_it == sec_it->second.end()) return std::nullopt;
  const std::string configured =
      runtime_binding_init_trim_ascii_copy(value_it->second);
  if (!has_non_ws_text(configured)) return std::nullopt;
  return configured;
}

[[nodiscard]] inline runtime_binding_device_resolution_t
resolve_runtime_binding_preferred_device_for_binding(
    const std::string& binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>&
        runtime_binding_itself,
    torch::Device fallback_device = torch::kCPU) {
  runtime_binding_device_resolution_t out{};
  out.device = fallback_device;
  if (!runtime_binding_itself) {
    out.error = "missing runtime binding record";
    return out;
  }

  const auto& runtime_binding_instruction =
      runtime_binding_itself->runtime_binding.decoded();
  const auto* bind = find_bind_by_id(runtime_binding_instruction, binding_id);
  if (!bind) {
    out.error = "unknown runtime binding id: " + binding_id;
    return out;
  }
  const auto* contract_decl =
      find_contract_decl_by_id(runtime_binding_instruction, bind->contract_ref);
  if (!contract_decl) {
    out.error = "binding references unknown CONTRACT id: " + bind->contract_ref;
    return out;
  }

  const std::string contract_path =
      (std::filesystem::path(runtime_binding_itself->config_file_path)
           .parent_path() /
       contract_decl->file)
          .string();
  out.contract_hash =
      cuwacunu::iitepi::contract_space_t::register_contract_file(contract_path);
  cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(
      out.contract_hash);
  const auto contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(out.contract_hash);

  static constexpr std::string_view kDevicePrioritySections[] = {
      "VICReg",
      "EXPECTED_VALUE",
      "TRANSFER_MATRIX_EVALUATION",
      "EMBEDDING_SEQUENCE_ANALYTICS",
  };

  bool selected = false;
  for (const auto section : kDevicePrioritySections) {
    const auto configured =
        runtime_binding_section_device_value(contract_itself, section);
    if (!configured.has_value()) continue;
    const torch::Device resolved =
        cuwacunu::iitepi::config_device(out.contract_hash, std::string(section));
    if (resolved.is_cpu() &&
        runtime_binding_requests_cuda_device_value(*configured)) {
      out.error =
          "CUDA explicitly requested in section '" + std::string(section) +
          "' but runtime device resolved to CPU";
    }
    if (!selected) {
      out.device = resolved;
      out.source_section = std::string(section);
      out.configured_device = *configured;
      out.resolved_from_config = true;
      selected = true;
      continue;
    }
    if (resolved.str() != out.device.str()) {
      log_warn(
          "[runtime_binding.device] conflicting configured devices for contract_hash=%s binding=%s preferred=%s(%s->%s) ignored=%s(%s->%s)\n",
          runtime_binding_log_value_or_empty(out.contract_hash),
          runtime_binding_log_value_or_empty(binding_id),
          runtime_binding_log_value_or_empty(out.source_section),
          runtime_binding_log_value_or_empty(out.configured_device),
          out.device.str().c_str(),
          std::string(section).c_str(),
          configured->c_str(),
          resolved.str().c_str());
    }
  }

  if (selected) return out;

  const auto general_section_it =
      cuwacunu::iitepi::config_space_t::config.find("GENERAL");
  if (general_section_it != cuwacunu::iitepi::config_space_t::config.end()) {
    const auto device_it = general_section_it->second.find("device");
    if (device_it != general_section_it->second.end()) {
      const std::string configured =
          runtime_binding_init_trim_ascii_copy(device_it->second);
      if (has_non_ws_text(configured)) {
        out.device =
            cuwacunu::iitepi::config_device(out.contract_hash, "GENERAL");
        out.source_section = "GENERAL";
        out.configured_device = configured;
        out.resolved_from_config = true;
        if (out.device.is_cpu() &&
            runtime_binding_requests_cuda_device_value(configured)) {
          out.error =
              "CUDA explicitly requested in section 'GENERAL' but runtime "
              "device resolved to CPU";
        }
      }
    }
  }

  return out;
}

[[nodiscard]] inline runtime_binding_device_resolution_t
resolve_runtime_binding_preferred_device_for_contract_path(
    const std::filesystem::path& contract_path,
    torch::Device fallback_device = torch::kCPU) {
  runtime_binding_device_resolution_t out{};
  out.device = fallback_device;
  if (contract_path.empty()) {
    out.error = "missing contract path";
    return out;
  }

  out.contract_hash =
      cuwacunu::iitepi::contract_space_t::register_contract_file(
          contract_path.string());
  cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(
      out.contract_hash);
  const auto contract_itself =
      cuwacunu::iitepi::contract_space_t::contract_itself(out.contract_hash);
  if (!contract_itself) {
    out.error = "missing contract record";
    return out;
  }

  static constexpr std::string_view kDevicePrioritySections[] = {
      "VICReg",
      "EXPECTED_VALUE",
      "TRANSFER_MATRIX_EVALUATION",
      "EMBEDDING_SEQUENCE_ANALYTICS",
  };

  for (const auto section : kDevicePrioritySections) {
    const auto configured =
        runtime_binding_section_device_value(contract_itself, section);
    if (!configured.has_value()) continue;
    out.device =
        cuwacunu::iitepi::config_device(out.contract_hash, std::string(section));
    out.source_section = std::string(section);
    out.configured_device = *configured;
    out.resolved_from_config = true;
    if (out.device.is_cpu() &&
        runtime_binding_requests_cuda_device_value(*configured)) {
      out.error =
          "CUDA explicitly requested in section '" + std::string(section) +
          "' but runtime device resolved to CPU";
    }
    return out;
  }

  const auto general_section_it =
      cuwacunu::iitepi::config_space_t::config.find("GENERAL");
  if (general_section_it != cuwacunu::iitepi::config_space_t::config.end()) {
    const auto device_it = general_section_it->second.find("device");
    if (device_it != general_section_it->second.end()) {
      const std::string configured =
          runtime_binding_init_trim_ascii_copy(device_it->second);
      if (has_non_ws_text(configured)) {
        out.device =
            cuwacunu::iitepi::config_device(out.contract_hash, "GENERAL");
        out.source_section = "GENERAL";
        out.configured_device = configured;
        out.resolved_from_config = true;
        if (out.device.is_cpu() &&
            runtime_binding_requests_cuda_device_value(configured)) {
          out.error =
              "CUDA explicitly requested in section 'GENERAL' but runtime "
              "device resolved to CPU";
        }
      }
    }
  }

  return out;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::runtime_binding_bind_decl_t*
find_bind_by_id(const cuwacunu::camahjucunu::runtime_binding_instruction_t& instruction,
                const std::string& binding_id) {
  for (const auto& bind : instruction.binds) {
    if (bind.id == binding_id) return &bind;
  }
  return nullptr;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::runtime_binding_contract_decl_t*
find_contract_decl_by_id(
    const cuwacunu::camahjucunu::runtime_binding_instruction_t& instruction,
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

[[nodiscard]] inline std::mutex& runtime_binding_runtime_mutex() {
  static std::mutex mutex{};
  return mutex;
}

[[nodiscard]] inline std::string runtime_binding_init_trim_ascii_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] inline std::string runtime_binding_init_lower_ascii_copy(std::string s) {
  for (char& ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
}

[[nodiscard]] inline bool runtime_binding_init_parse_bool_ascii(std::string value, bool* out) {
  if (!out) return false;
  value = runtime_binding_init_lower_ascii_copy(runtime_binding_init_trim_ascii_copy(std::move(value)));
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

[[nodiscard]] inline std::uint64_t runtime_binding_now_ms_utc() {
  return static_cast<std::uint64_t>(
      std::chrono::duration_cast<std::chrono::milliseconds>(
          std::chrono::system_clock::now().time_since_epoch())
          .count());
}

[[nodiscard]] inline std::string make_runtime_binding_contract_run_id(
    const runtime_binding_run_record_t& run_record,
    std::size_t contract_index) {
  std::ostringstream out;
  out << "run."
      << runtime_binding_init_trim_ascii_copy(run_record.campaign_hash) << "."
      << runtime_binding_init_trim_ascii_copy(run_record.binding_id) << "."
      << contract_index << "."
      << runtime_binding_now_ms_utc();
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
    if (!runtime_binding_init_parse_bool_ascii(ch.active, &active)) {
      if (error) {
        *error = "invalid observation channel active flag '" + ch.active +
                 "' for interval '" +
                 cuwacunu::camahjucunu::exchange::enum_to_string(ch.interval) + "'";
      }
      return false;
    }
    if (!active) continue;
    const std::string record = runtime_binding_init_lower_ascii_copy(
        runtime_binding_init_trim_ascii_copy(ch.record_type));
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
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>& runtime_binding_itself,
    const cuwacunu::camahjucunu::runtime_binding_bind_decl_t& bind,
    const std::string& wave_hash,
    std::string* out_sampler,
    std::string* error) {
  if (!runtime_binding_itself || !out_sampler) {
    if (error) *error = "missing runtime binding record while resolving wave sampler";
    return false;
  }
  const auto wave_itself = cuwacunu::iitepi::wave_space_t::wave_itself(wave_hash);
  const auto& wave_set = wave_itself->wave.decoded();
  const auto* wave = find_wave_by_id(wave_set, bind.wave_ref);
  if (!wave) {
    if (error) *error = "binding references unknown WAVE id: " + bind.wave_ref;
    return false;
  }
  const std::string sampler = runtime_binding_init_lower_ascii_copy(
      runtime_binding_init_trim_ascii_copy(wave->sampler));
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

[[nodiscard]] inline runtime_binding_run_record_t run_initialized_runtime_binding(
    runtime_binding_contract_init_record_t init) {
  // Finalize the init snapshot into one immutable run record.
  runtime_binding_run_record_t out{};
  out.campaign_hash = init.campaign_hash;
  out.binding_id = init.binding_id;
  out.source_config_path = init.source_config_path;

  if (!init.ok) {
    out.error = std::move(init.error);
    out.contract_hash = std::move(init.contract_hash);
    out.wave_hash = std::move(init.wave_hash);
    out.resolved_record_type = std::move(init.resolved_record_type);
    out.resolved_sampler = std::move(init.resolved_sampler);
    emit_runtime_binding_trace_event(
        {.phase = "run_end",
         .campaign_hash = out.campaign_hash,
         .binding_id = out.binding_id,
         .contract_hash = out.contract_hash,
         .wave_hash = out.wave_hash,
         .resolved_record_type = out.resolved_record_type,
         .resolved_sampler = out.resolved_sampler,
         .contract_count = 0,
         .ok = false,
         .error = out.error});
    log_err(
        "[runtime_binding.run] init failed campaign_hash=%s binding=%s error=%s\n",
        runtime_binding_log_value_or_empty(out.campaign_hash),
        runtime_binding_log_value_or_empty(out.binding_id),
        runtime_binding_log_value_or_empty(out.error));
    return out;
  }

  out.contract_hash = std::move(init.contract_hash);
  out.wave_hash = std::move(init.wave_hash);
  out.resolved_record_type = std::move(init.resolved_record_type);
  out.resolved_sampler = std::move(init.resolved_sampler);
  out.runtime_binding = std::move(init.runtime_binding);
  out.contract_steps.reserve(out.runtime_binding.contracts.size());
  out.run_ids.reserve(out.runtime_binding.contracts.size());
  emit_runtime_binding_trace_event(
      {.phase = "run_start",
       .campaign_hash = out.campaign_hash,
       .binding_id = out.binding_id,
       .contract_hash = out.contract_hash,
       .wave_hash = out.wave_hash,
       .resolved_record_type = out.resolved_record_type,
       .resolved_sampler = out.resolved_sampler,
       .contract_count = static_cast<std::uint64_t>(
           out.runtime_binding.contracts.size()),
       .ok = true});
  log_info(
      "[runtime_binding.run] start campaign_hash=%s binding=%s contracts=%zu record_type=%s sampler=%s\n",
      runtime_binding_log_value_or_empty(out.campaign_hash),
      runtime_binding_log_value_or_empty(out.binding_id),
      out.runtime_binding.contracts.size(),
      runtime_binding_log_value_or_empty(out.resolved_record_type),
      runtime_binding_log_value_or_empty(out.resolved_sampler));

  // Execution phase: contracts are evaluated in runtime-binding declaration order.
  for (std::size_t i = 0; i < out.runtime_binding.contracts.size(); ++i) {
    auto& contract = out.runtime_binding.contracts[i];
    log_dbg(
        "[runtime_binding.run] contract[%zu] begin name=%s epochs=%llu\n",
        i,
        runtime_binding_log_value_or_empty(contract.name),
        static_cast<unsigned long long>(
            std::max<std::uint64_t>(1, contract.execution.epochs)));
    RuntimeContext ctx{};
    ctx.wave_mode_flags = contract.execution.wave_mode_flags;
    ctx.debug_enabled = contract.execution.debug_enabled;
    ctx.binding_id = out.binding_id;
    ctx.run_id = make_runtime_binding_contract_run_id(out, i);
    ctx.source_runtime_cursor = contract.spec.source_runtime_cursor;
    runtime_binding_trace_context_t trace_ctx{
        .campaign_hash = out.campaign_hash,
        .binding_id = out.binding_id,
        .contract_hash = contract.spec.contract_hash,
        .wave_hash = out.wave_hash,
        .contract_name = contract.name,
        .contract_index = static_cast<std::uint64_t>(i),
        .contract_count = static_cast<std::uint64_t>(
            out.runtime_binding.contracts.size()),
        .epochs = std::max<std::uint64_t>(1, contract.execution.epochs)};
    ctx.user = &trace_ctx;
    out.run_ids.push_back(ctx.run_id);
    emit_runtime_binding_trace_event(
        {.phase = "contract_start",
         .campaign_hash = trace_ctx.campaign_hash,
         .binding_id = trace_ctx.binding_id,
         .contract_hash = trace_ctx.contract_hash,
         .wave_hash = trace_ctx.wave_hash,
         .contract_name = trace_ctx.contract_name,
         .run_id = ctx.run_id,
         .source_runtime_cursor = ctx.source_runtime_cursor,
         .contract_index = trace_ctx.contract_index,
         .contract_count = trace_ctx.contract_count,
         .epochs = trace_ctx.epochs,
         .ok = true});
    std::string run_error;
    const std::uint64_t steps = run_runtime_binding_contract(contract, ctx, &run_error);
    if (!run_error.empty()) {
      out.error = "run_contract failed for contract[" + std::to_string(i) +
                  "]: " + run_error;
      emit_runtime_binding_trace_event(
          {.phase = "contract_end",
           .campaign_hash = trace_ctx.campaign_hash,
           .binding_id = trace_ctx.binding_id,
           .contract_hash = trace_ctx.contract_hash,
           .wave_hash = trace_ctx.wave_hash,
           .contract_name = trace_ctx.contract_name,
           .run_id = ctx.run_id,
           .source_runtime_cursor = ctx.source_runtime_cursor,
           .contract_index = trace_ctx.contract_index,
           .contract_count = trace_ctx.contract_count,
           .epochs = trace_ctx.epochs,
           .total_steps = steps,
           .ok = false,
           .error = out.error});
      emit_runtime_binding_trace_event(
          {.phase = "run_end",
           .campaign_hash = out.campaign_hash,
           .binding_id = out.binding_id,
           .contract_hash = out.contract_hash,
           .wave_hash = out.wave_hash,
           .resolved_record_type = out.resolved_record_type,
           .resolved_sampler = out.resolved_sampler,
           .contract_index = trace_ctx.contract_index,
           .contract_count = trace_ctx.contract_count,
           .total_steps = out.total_steps,
           .ok = false,
           .error = out.error});
      log_err(
          "[runtime_binding.run] contract[%zu] failed name=%s error=%s\n",
          i,
          runtime_binding_log_value_or_empty(contract.name),
          runtime_binding_log_value_or_empty(out.error));
      return out;
    }
    out.contract_steps.push_back(steps);
    out.total_steps += steps;
    emit_runtime_binding_trace_event(
        {.phase = "contract_end",
         .campaign_hash = trace_ctx.campaign_hash,
         .binding_id = trace_ctx.binding_id,
         .contract_hash = trace_ctx.contract_hash,
         .wave_hash = trace_ctx.wave_hash,
         .contract_name = trace_ctx.contract_name,
         .run_id = ctx.run_id,
         .source_runtime_cursor = ctx.source_runtime_cursor,
         .contract_index = trace_ctx.contract_index,
         .contract_count = trace_ctx.contract_count,
         .epochs = trace_ctx.epochs,
         .total_steps = out.total_steps,
         .ok = true});
    log_info(
        "[runtime_binding.run] contract[%zu] done steps=%llu cumulative_steps=%llu\n",
        i,
        static_cast<unsigned long long>(steps),
        static_cast<unsigned long long>(out.total_steps));
  }

  // Finalization: publish completed run counters.
  out.ok = true;
  emit_runtime_binding_trace_event(
      {.phase = "run_end",
       .campaign_hash = out.campaign_hash,
       .binding_id = out.binding_id,
       .contract_hash = out.contract_hash,
       .wave_hash = out.wave_hash,
       .resolved_record_type = out.resolved_record_type,
       .resolved_sampler = out.resolved_sampler,
       .contract_count = static_cast<std::uint64_t>(out.contract_steps.size()),
       .total_steps = out.total_steps,
       .ok = true});
  log_info(
      "[runtime_binding.run] done campaign_hash=%s binding=%s total_steps=%llu contracts=%zu\n",
      runtime_binding_log_value_or_empty(out.campaign_hash),
      runtime_binding_log_value_or_empty(out.binding_id),
      static_cast<unsigned long long>(out.total_steps),
      out.contract_steps.size());
  return out;
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline runtime_binding_contract_init_record_t invoke_runtime_binding_contract_init_from_snapshot(
    const cuwacunu::iitepi::runtime_binding_hash_t& campaign_hash,
    const std::string& binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>& runtime_binding_itself,
    torch::Device device = torch::kCPU) {
  runtime_binding_contract_init_record_t out{};
  if (!runtime_binding_itself) {
    out.error = "missing runtime binding record";
    log_err(
        "[runtime_binding.contract.init] missing runtime binding record campaign_hash=%s binding=%s\n",
        runtime_binding_log_value_or_empty(campaign_hash),
        runtime_binding_log_value_or_empty(binding_id));
    return out;
  }
  out.campaign_hash = campaign_hash;
  out.binding_id = binding_id;
  out.source_config_path = runtime_binding_itself->config_file_path;
  log_info(
      "[runtime_binding.contract.init] start campaign_hash=%s binding=%s device=%s\n",
      runtime_binding_log_value_or_empty(out.campaign_hash),
      runtime_binding_log_value_or_empty(out.binding_id),
      device.str().c_str());

  try {
    // Initialization preflight: bind contract/wave identity from runtime-binding snapshot.
    const auto& runtime_binding_instruction =
        runtime_binding_itself->runtime_binding.decoded();
    const auto* bind = find_bind_by_id(runtime_binding_instruction, binding_id);
    if (!bind) {
      out.error = "unknown runtime binding id: " + binding_id;
      log_err(
          "[runtime_binding.contract.init] binding lookup failed campaign_hash=%s binding=%s\n",
          runtime_binding_log_value_or_empty(out.campaign_hash),
          runtime_binding_log_value_or_empty(out.binding_id));
      return out;
    }
    const auto* contract_decl =
        find_contract_decl_by_id(runtime_binding_instruction, bind->contract_ref);
    if (!contract_decl) {
      out.error = "binding references unknown CONTRACT id: " + bind->contract_ref;
      log_err(
          "[runtime_binding.contract.init] missing CONTRACT ref=%s campaign_hash=%s binding=%s\n",
          runtime_binding_log_value_or_empty(bind->contract_ref),
          runtime_binding_log_value_or_empty(out.campaign_hash),
          runtime_binding_log_value_or_empty(out.binding_id));
      return out;
    }

    const std::string contract_path =
        (std::filesystem::path(runtime_binding_itself->config_file_path)
             .parent_path() /
         contract_decl->file)
            .string();

    out.contract_hash =
        cuwacunu::iitepi::contract_space_t::register_contract_file(
            contract_path);
    out.wave_hash =
        cuwacunu::iitepi::runtime_binding_space_t::wave_hash_for_binding(
            campaign_hash, binding_id);
    log_info(
        "[runtime_binding.contract.init] resolved contract_hash=%s wave_hash=%s\n",
        runtime_binding_log_value_or_empty(out.contract_hash),
        runtime_binding_log_value_or_empty(out.wave_hash));
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

    // Build runtime binding payload from validated contract + wave DSLs.
    const auto& parsed = contract_itself->circuit.decoded();
    std::string build_error;
    if (!runtime_binding_builder::build_runtime_binding_from_instruction<Datatype_t, Sampler_t>(
            parsed,
            device,
            out.contract_hash,
            contract_itself,
            out.wave_hash,
            wave_itself,
            bind->wave_ref,
            &out.runtime_binding,
            &build_error)) {
      out.error = "failed to build runtime binding: " + build_error;
      return out;
    }
    out.runtime_binding.campaign_hash = out.campaign_hash;
    out.runtime_binding.runtime_binding_path = runtime_binding_itself->config_file_path;
    out.runtime_binding.binding_id = out.binding_id;
    out.runtime_binding.contract_hash = out.contract_hash;
    out.runtime_binding.wave_hash = out.wave_hash;

    // Final validation before exposing initialized runtime binding.
    RuntimeBindingIssue issue{};
    if (!validate_runtime_binding(out.runtime_binding, &issue)) {
      out.error = "invalid runtime binding: " + std::string(issue.circuit_issue.what);
      log_err(
          "[runtime_binding.contract.init] validation failed campaign_hash=%s binding=%s error=%s\n",
          runtime_binding_log_value_or_empty(out.campaign_hash),
          runtime_binding_log_value_or_empty(out.binding_id),
          runtime_binding_log_value_or_empty(out.error));
      return out;
    }

    out.ok = true;
    log_info(
        "[runtime_binding.contract.init] done campaign_hash=%s binding=%s contracts=%zu\n",
        runtime_binding_log_value_or_empty(out.campaign_hash),
        runtime_binding_log_value_or_empty(out.binding_id),
        out.runtime_binding.contracts.size());
    return out;
  } catch (const std::exception& e) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) + " exception: " + e.what();
    log_err(
        "[runtime_binding.contract.init] exception campaign_hash=%s binding=%s error=%s\n",
        runtime_binding_log_value_or_empty(out.campaign_hash),
        runtime_binding_log_value_or_empty(out.binding_id),
        runtime_binding_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) + " exception: unknown";
    log_err(
        "[runtime_binding.contract.init] exception campaign_hash=%s binding=%s error=%s\n",
        runtime_binding_log_value_or_empty(out.campaign_hash),
        runtime_binding_log_value_or_empty(out.binding_id),
        runtime_binding_log_value_or_empty(out.error));
    return out;
  }
}

[[nodiscard]] inline runtime_binding_contract_init_record_t invoke_runtime_binding_contract_init_from_snapshot(
    const cuwacunu::iitepi::runtime_binding_hash_t& campaign_hash,
    const std::string& binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>& runtime_binding_itself,
    torch::Device device = torch::kCPU) {
  runtime_binding_contract_init_record_t out{};
  out.campaign_hash = campaign_hash;
  out.binding_id = binding_id;
  if (runtime_binding_itself) out.source_config_path = runtime_binding_itself->config_file_path;
  log_info(
      "[runtime_binding.contract.init] infer runtime campaign_hash=%s binding=%s device=%s\n",
      runtime_binding_log_value_or_empty(out.campaign_hash),
      runtime_binding_log_value_or_empty(out.binding_id),
      device.str().c_str());

  try {
    if (!runtime_binding_itself) {
      out.error = "missing runtime binding record";
      log_err(
          "[runtime_binding.contract.init] missing runtime binding record campaign_hash=%s binding=%s\n",
          runtime_binding_log_value_or_empty(out.campaign_hash),
          runtime_binding_log_value_or_empty(out.binding_id));
      return out;
    }
    const auto& runtime_binding_instruction =
        runtime_binding_itself->runtime_binding.decoded();
    const auto* bind = find_bind_by_id(runtime_binding_instruction, binding_id);
    if (!bind) {
      out.error = "unknown runtime binding id: " + binding_id;
      log_err(
          "[runtime_binding.contract.init] binding lookup failed campaign_hash=%s binding=%s\n",
          runtime_binding_log_value_or_empty(out.campaign_hash),
          runtime_binding_log_value_or_empty(out.binding_id));
      return out;
    }
    const auto* contract_decl =
        find_contract_decl_by_id(runtime_binding_instruction, bind->contract_ref);
    if (!contract_decl) {
      out.error = "binding references unknown CONTRACT id: " + bind->contract_ref;
      log_err(
          "[runtime_binding.contract.init] missing CONTRACT ref=%s campaign_hash=%s binding=%s\n",
          runtime_binding_log_value_or_empty(bind->contract_ref),
          runtime_binding_log_value_or_empty(out.campaign_hash),
          runtime_binding_log_value_or_empty(out.binding_id));
      return out;
    }

    const std::string contract_path =
        (std::filesystem::path(runtime_binding_itself->config_file_path)
             .parent_path() /
         contract_decl->file)
            .string();
    const auto contract_hash =
        cuwacunu::iitepi::contract_space_t::register_contract_file(contract_path);
    const auto wave_hash =
        cuwacunu::iitepi::runtime_binding_space_t::wave_hash_for_binding(campaign_hash, binding_id);
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
          "[runtime_binding.contract.init] missing WAVE ref=%s campaign_hash=%s binding=%s\n",
          runtime_binding_log_value_or_empty(bind->wave_ref),
          runtime_binding_log_value_or_empty(out.campaign_hash),
          runtime_binding_log_value_or_empty(out.binding_id));
      return out;
    }

    cuwacunu::camahjucunu::observation_spec_t effective_observation{};
    if (!runtime_binding_builder::load_wave_dataloader_observation_payloads(
            contract_itself,
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
            runtime_binding_itself, *bind, wave_hash, &inferred_sampler, &out.error)) {
      return out;
    }

    out.resolved_record_type = inferred_record_type;
    out.resolved_sampler = inferred_sampler;
    log_info(
        "[runtime_binding.contract.init] resolved runtime campaign_hash=%s binding=%s record_type=%s sampler=%s\n",
        runtime_binding_log_value_or_empty(out.campaign_hash),
        runtime_binding_log_value_or_empty(out.binding_id),
        runtime_binding_log_value_or_empty(out.resolved_record_type),
        runtime_binding_log_value_or_empty(out.resolved_sampler));

    if (inferred_record_type == "kline") {
      if (inferred_sampler == "sequential") {
        auto typed = invoke_runtime_binding_contract_init_from_snapshot<
            cuwacunu::camahjucunu::exchange::kline_t,
            torch::data::samplers::SequentialSampler>(
            campaign_hash, binding_id, runtime_binding_itself, device);
        typed.resolved_record_type = inferred_record_type;
        typed.resolved_sampler = inferred_sampler;
        if (!typed.ok) {
          log_err(
              "[runtime_binding.contract.init] typed init failed campaign_hash=%s binding=%s error=%s\n",
              runtime_binding_log_value_or_empty(typed.campaign_hash),
              runtime_binding_log_value_or_empty(typed.binding_id),
              runtime_binding_log_value_or_empty(typed.error));
        }
        return typed;
      }
      if (inferred_sampler == "random") {
        auto typed = invoke_runtime_binding_contract_init_from_snapshot<
            cuwacunu::camahjucunu::exchange::kline_t,
            torch::data::samplers::RandomSampler>(
            campaign_hash, binding_id, runtime_binding_itself, device);
        typed.resolved_record_type = inferred_record_type;
        typed.resolved_sampler = inferred_sampler;
        if (!typed.ok) {
          log_err(
              "[runtime_binding.contract.init] typed init failed campaign_hash=%s binding=%s error=%s\n",
              runtime_binding_log_value_or_empty(typed.campaign_hash),
              runtime_binding_log_value_or_empty(typed.binding_id),
              runtime_binding_log_value_or_empty(typed.error));
        }
        return typed;
      }
    }

    out.error = "unsupported runtime combination record_type='" +
                inferred_record_type + "' sampler='" + inferred_sampler +
                "' (supported now: kline + sequential|random)";
    log_err(
        "[runtime_binding.contract.init] unsupported runtime campaign_hash=%s binding=%s error=%s\n",
        runtime_binding_log_value_or_empty(out.campaign_hash),
        runtime_binding_log_value_or_empty(out.binding_id),
        runtime_binding_log_value_or_empty(out.error));
    return out;
  } catch (const std::exception& e) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) + " exception: " + e.what();
    log_err(
        "[runtime_binding.contract.init] exception campaign_hash=%s binding=%s error=%s\n",
        runtime_binding_log_value_or_empty(out.campaign_hash),
        runtime_binding_log_value_or_empty(out.binding_id),
        runtime_binding_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) + " exception: unknown";
    log_err(
        "[runtime_binding.contract.init] exception campaign_hash=%s binding=%s error=%s\n",
        runtime_binding_log_value_or_empty(out.campaign_hash),
        runtime_binding_log_value_or_empty(out.binding_id),
        runtime_binding_log_value_or_empty(out.error));
    return out;
  }
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline runtime_binding_run_record_t run_runtime_binding_snapshot(
    const cuwacunu::iitepi::runtime_binding_hash_t& campaign_hash,
    const std::string& binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>& runtime_binding_itself,
    torch::Device device = torch::kCPU) {
  log_info(
      "[runtime_binding.run] snapshot start campaign_hash=%s binding=%s\n",
      runtime_binding_log_value_or_empty(campaign_hash),
      runtime_binding_log_value_or_empty(binding_id));
  try {
    auto out = run_initialized_runtime_binding(
        invoke_runtime_binding_contract_init_from_snapshot<Datatype_t, Sampler_t>(
            campaign_hash, binding_id, runtime_binding_itself, device));
    if (!out.ok) {
      log_err(
          "[runtime_binding.run] snapshot failed campaign_hash=%s binding=%s error=%s\n",
          runtime_binding_log_value_or_empty(out.campaign_hash),
          runtime_binding_log_value_or_empty(out.binding_id),
          runtime_binding_log_value_or_empty(out.error));
    }
    return out;
  } catch (const std::exception& e) {
    runtime_binding_run_record_t out{};
    out.campaign_hash = campaign_hash;
    out.binding_id = binding_id;
    if (runtime_binding_itself) out.source_config_path = runtime_binding_itself->config_file_path;
    out.error = std::string(kRuntimeBindingRunCanonicalAction) + " exception: " +
                e.what();
    log_err(
        "[runtime_binding.run] snapshot exception campaign_hash=%s binding=%s error=%s\n",
        runtime_binding_log_value_or_empty(out.campaign_hash),
        runtime_binding_log_value_or_empty(out.binding_id),
        runtime_binding_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    runtime_binding_run_record_t out{};
    out.campaign_hash = campaign_hash;
    out.binding_id = binding_id;
    if (runtime_binding_itself) out.source_config_path = runtime_binding_itself->config_file_path;
    out.error =
        std::string(kRuntimeBindingRunCanonicalAction) + " exception: unknown";
    log_err(
        "[runtime_binding.run] snapshot exception campaign_hash=%s binding=%s error=%s\n",
        runtime_binding_log_value_or_empty(out.campaign_hash),
        runtime_binding_log_value_or_empty(out.binding_id),
        runtime_binding_log_value_or_empty(out.error));
    return out;
  }
}

[[nodiscard]] inline runtime_binding_run_record_t run_runtime_binding_snapshot(
    const cuwacunu::iitepi::runtime_binding_hash_t& campaign_hash,
    const std::string& binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>& runtime_binding_itself,
    torch::Device device = torch::kCPU) {
  log_info(
      "[runtime_binding.run] snapshot start campaign_hash=%s binding=%s\n",
      runtime_binding_log_value_or_empty(campaign_hash),
      runtime_binding_log_value_or_empty(binding_id));
  try {
    auto out = run_initialized_runtime_binding(invoke_runtime_binding_contract_init_from_snapshot(
        campaign_hash, binding_id, runtime_binding_itself, device));
    if (!out.ok) {
      log_err(
          "[runtime_binding.run] snapshot failed campaign_hash=%s binding=%s error=%s\n",
          runtime_binding_log_value_or_empty(out.campaign_hash),
          runtime_binding_log_value_or_empty(out.binding_id),
          runtime_binding_log_value_or_empty(out.error));
    }
    return out;
  } catch (const std::exception& e) {
    runtime_binding_run_record_t out{};
    out.campaign_hash = campaign_hash;
    out.binding_id = binding_id;
    if (runtime_binding_itself) out.source_config_path = runtime_binding_itself->config_file_path;
    out.error = std::string(kRuntimeBindingRunCanonicalAction) + " exception: " +
                e.what();
    log_err(
        "[runtime_binding.run] snapshot exception campaign_hash=%s binding=%s error=%s\n",
        runtime_binding_log_value_or_empty(out.campaign_hash),
        runtime_binding_log_value_or_empty(out.binding_id),
        runtime_binding_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    runtime_binding_run_record_t out{};
    out.campaign_hash = campaign_hash;
    out.binding_id = binding_id;
    if (runtime_binding_itself) out.source_config_path = runtime_binding_itself->config_file_path;
    out.error =
        std::string(kRuntimeBindingRunCanonicalAction) + " exception: unknown";
    log_err(
        "[runtime_binding.run] snapshot exception campaign_hash=%s binding=%s error=%s\n",
        runtime_binding_log_value_or_empty(out.campaign_hash),
        runtime_binding_log_value_or_empty(out.binding_id),
        runtime_binding_log_value_or_empty(out.error));
    return out;
  }
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline runtime_binding_run_record_t run_runtime_binding(
    const std::string& binding_id,
    torch::Device device = torch::kCPU) {
  runtime_binding_run_record_t out{};
  try {
    // Input normalization before hitting runtime lock and snapshots.
    const std::string normalized_binding_id =
        runtime_binding_init_trim_ascii_copy(binding_id);
    if (!has_non_ws_text(normalized_binding_id)) {
      out.error = "run_runtime_binding requires non-empty binding_id";
      log_err("[runtime_binding.run] invalid request error=%s\n",
              runtime_binding_log_value_or_empty(out.error));
      return out;
    }
    const auto campaign_hash = cuwacunu::iitepi::runtime_binding_space_t::locked_runtime_binding_hash();
    log_info(
        "[runtime_binding.run] dispatch campaign_hash=%s binding=%s\n",
        runtime_binding_log_value_or_empty(campaign_hash),
        runtime_binding_log_value_or_empty(normalized_binding_id));
    const auto runtime_binding_itself =
        cuwacunu::iitepi::runtime_binding_space_t::runtime_binding_itself(campaign_hash);
    std::lock_guard<std::mutex> run_guard(runtime_binding_runtime_mutex());
    cuwacunu::hero::runtime_lock::scoped_lock_t global_runtime_lock{};
    std::string global_lock_error;
    if (!cuwacunu::hero::runtime_lock::acquire(&global_runtime_lock,
                                               &global_lock_error)) {
      out.error =
          "failed to acquire global runtime lock: " + global_lock_error;
      log_err(
          "[runtime_binding.run] global lock failed campaign_hash=%s binding=%s error=%s\n",
          runtime_binding_log_value_or_empty(campaign_hash),
          runtime_binding_log_value_or_empty(normalized_binding_id),
          runtime_binding_log_value_or_empty(out.error));
      return out;
    }
    log_info(
        "[runtime_binding.run] runtime lock acquired campaign_hash=%s binding=%s lock_path=%s\n",
        runtime_binding_log_value_or_empty(campaign_hash),
        runtime_binding_log_value_or_empty(normalized_binding_id),
        runtime_binding_log_value_or_empty(global_runtime_lock.path));
    return run_runtime_binding_snapshot<Datatype_t, Sampler_t>(
        campaign_hash, normalized_binding_id, runtime_binding_itself, device);
  } catch (const std::exception& e) {
    out.error = std::string(kRuntimeBindingRunCanonicalAction) + " exception: " +
                e.what();
    log_err("[runtime_binding.run] dispatch exception error=%s\n",
            runtime_binding_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    out.error =
        std::string(kRuntimeBindingRunCanonicalAction) + " exception: unknown";
    log_err("[runtime_binding.run] dispatch exception error=%s\n",
            runtime_binding_log_value_or_empty(out.error));
    return out;
  }
}

[[nodiscard]] inline runtime_binding_run_record_t run_runtime_binding(
    const std::string& binding_id,
    torch::Device device = torch::kCPU) {
  runtime_binding_run_record_t out{};
  try {
    // Input normalization before hitting runtime lock and snapshots.
    const std::string normalized_binding_id =
        runtime_binding_init_trim_ascii_copy(binding_id);
    if (!has_non_ws_text(normalized_binding_id)) {
      out.error = "run_runtime_binding requires non-empty binding_id";
      log_err("[runtime_binding.run] invalid request error=%s\n",
              runtime_binding_log_value_or_empty(out.error));
      return out;
    }
    const auto campaign_hash = cuwacunu::iitepi::runtime_binding_space_t::locked_runtime_binding_hash();
    log_info(
        "[runtime_binding.run] dispatch campaign_hash=%s binding=%s\n",
        runtime_binding_log_value_or_empty(campaign_hash),
        runtime_binding_log_value_or_empty(normalized_binding_id));
    const auto runtime_binding_itself =
        cuwacunu::iitepi::runtime_binding_space_t::runtime_binding_itself(campaign_hash);
    std::lock_guard<std::mutex> run_guard(runtime_binding_runtime_mutex());
    cuwacunu::hero::runtime_lock::scoped_lock_t global_runtime_lock{};
    std::string global_lock_error;
    if (!cuwacunu::hero::runtime_lock::acquire(&global_runtime_lock,
                                               &global_lock_error)) {
      out.error =
          "failed to acquire global runtime lock: " + global_lock_error;
      log_err(
          "[runtime_binding.run] global lock failed campaign_hash=%s binding=%s error=%s\n",
          runtime_binding_log_value_or_empty(campaign_hash),
          runtime_binding_log_value_or_empty(normalized_binding_id),
          runtime_binding_log_value_or_empty(out.error));
      return out;
    }
    log_info(
        "[runtime_binding.run] runtime lock acquired campaign_hash=%s binding=%s lock_path=%s\n",
        runtime_binding_log_value_or_empty(campaign_hash),
        runtime_binding_log_value_or_empty(normalized_binding_id),
        runtime_binding_log_value_or_empty(global_runtime_lock.path));
    return run_runtime_binding_snapshot(
        campaign_hash, normalized_binding_id, runtime_binding_itself, device);
  } catch (const std::exception& e) {
    out.error = std::string(kRuntimeBindingRunCanonicalAction) + " exception: " +
                e.what();
    log_err("[runtime_binding.run] dispatch exception error=%s\n",
            runtime_binding_log_value_or_empty(out.error));
    return out;
  } catch (...) {
    out.error =
        std::string(kRuntimeBindingRunCanonicalAction) + " exception: unknown";
    log_err("[runtime_binding.run] dispatch exception error=%s\n",
            runtime_binding_log_value_or_empty(out.error));
    return out;
  }
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline runtime_binding_contract_init_record_t invoke_runtime_binding_contract_init_from_file(
    const std::string& runtime_binding_file_path,
    const std::string& binding_id,
    torch::Device device = torch::kCPU) {
  runtime_binding_contract_init_record_t out{};

  try {
    const auto campaign_hash =
        cuwacunu::iitepi::runtime_binding_space_t::register_runtime_binding_file(
            runtime_binding_file_path);
    cuwacunu::iitepi::runtime_binding_space_t::assert_intact_or_fail_fast(campaign_hash);
    const auto runtime_binding_itself =
        cuwacunu::iitepi::runtime_binding_space_t::runtime_binding_itself(campaign_hash);
    return invoke_runtime_binding_contract_init_from_snapshot<Datatype_t, Sampler_t>(
        campaign_hash, binding_id, runtime_binding_itself, device);
  } catch (const std::exception& e) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) + " exception: " + e.what();
    return out;
  } catch (...) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) + " exception: unknown";
    return out;
  }
}

[[nodiscard]] inline runtime_binding_contract_init_record_t invoke_runtime_binding_contract_init_from_file(
    const std::string& runtime_binding_file_path,
    const std::string& binding_id,
    torch::Device device = torch::kCPU) {
  runtime_binding_contract_init_record_t out{};

  try {
    const auto campaign_hash =
        cuwacunu::iitepi::runtime_binding_space_t::register_runtime_binding_file(
            runtime_binding_file_path);
    cuwacunu::iitepi::runtime_binding_space_t::assert_intact_or_fail_fast(campaign_hash);
    const auto runtime_binding_itself =
        cuwacunu::iitepi::runtime_binding_space_t::runtime_binding_itself(campaign_hash);
    return invoke_runtime_binding_contract_init_from_snapshot(
        campaign_hash, binding_id, runtime_binding_itself, device);
  } catch (const std::exception& e) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) + " exception: " + e.what();
    return out;
  } catch (...) {
    out.error = std::string(kRuntimeBindingContractInitCanonicalAction) + " exception: unknown";
    return out;
  }
}

}  // namespace tsiemene

namespace cuwacunu::iitepi {

using runtime_binding_contract_init_record_t = ::tsiemene::runtime_binding_contract_init_record_t;
using runtime_binding_run_record_t = ::tsiemene::runtime_binding_run_record_t;

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline runtime_binding_run_record_t run_runtime_binding(
    const std::string& binding_id,
    torch::Device device = torch::kCPU) {
  return ::tsiemene::run_runtime_binding<Datatype_t, Sampler_t>(
      binding_id, device);
}

[[nodiscard]] inline runtime_binding_run_record_t run_runtime_binding(
    const std::string& binding_id,
    torch::Device device = torch::kCPU) {
  return ::tsiemene::run_runtime_binding(binding_id, device);
}

}  // namespace cuwacunu::iitepi
