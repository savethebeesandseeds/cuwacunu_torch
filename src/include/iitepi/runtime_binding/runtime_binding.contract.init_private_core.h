// ./include/iitepi/runtime_binding/runtime_binding.contract.init.h
// SPDX-License-Identifier: MIT
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdint>
#include <exception>
#include <filesystem>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

#include <torch/torch.h>

#include "camahjucunu/dsl/runtime_binding_instruction/runtime_binding_instruction.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit.h"
#include "camahjucunu/types/types_data.h"
#include "hero/hero_runtime_lock.h"
#include "iitepi/iitepi.h"
#include "iitepi/runtime_binding/runtime_binding.builder.h"

namespace tsiemene {

namespace runtime_binding_action_id {
#define RUNTIME_BINDING_PATH_DIRECTIVE(ID, TOKEN, SUMMARY)
#define RUNTIME_BINDING_PATH_METHOD(ID, TOKEN, SUMMARY)
#define RUNTIME_BINDING_PATH_DSL_SEGMENT(ID, KEY, SUMMARY)
#define RUNTIME_BINDING_PATH_ACTION(ID, TOKEN, SUMMARY)                        \
  inline constexpr const char ID[] = TOKEN;
#include "iitepi/runtime_binding/runtime_binding.paths.def"
#undef RUNTIME_BINDING_PATH_ACTION
#undef RUNTIME_BINDING_PATH_DSL_SEGMENT
#undef RUNTIME_BINDING_PATH_METHOD
#undef RUNTIME_BINDING_PATH_DIRECTIVE
} // namespace runtime_binding_action_id

inline constexpr const char *kRuntimeBindingContractInitCanonicalAction =
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

inline constexpr const char *kRuntimeBindingRunCanonicalAction =
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

struct runtime_binding_snapshot_context_t {
  bool ok{false};
  std::string error{};
  std::string campaign_hash{};
  std::string binding_id{};
  std::string contract_hash{};
  std::string wave_hash{};
  std::string wave_id{};
  std::string source_config_path{};
  std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>
      runtime_binding_itself{};
  std::shared_ptr<const cuwacunu::iitepi::contract_record_t> contract_itself{};
  std::shared_ptr<const cuwacunu::iitepi::wave_record_t> wave_itself{};
};

[[nodiscard]] inline const char *
runtime_binding_log_value_or_empty(const std::string &value) {
  return value.empty() ? "<empty>" : value.c_str();
}

[[nodiscard]] inline bool has_non_ws_text(std::string_view s) {
  for (const unsigned char c : s) {
    if (!std::isspace(c))
      return true;
  }
  return false;
}

[[nodiscard]] inline std::string
runtime_binding_init_trim_ascii_copy(std::string s);

[[nodiscard]] inline bool
runtime_binding_requests_cuda_device_value(std::string_view raw_value) {
  std::string value(raw_value);
  value = runtime_binding_init_trim_ascii_copy(std::move(value));
  std::transform(
      value.begin(), value.end(), value.begin(),
      [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
  if (value == "gpu" || value == "cuda")
    return true;
  if (value.rfind("gpu:", 0) == 0 || value.rfind("cuda:", 0) == 0)
    return true;
  if (value.rfind("torch::", 0) == 0)
    value = value.substr(7);
  if (value.rfind("at::", 0) == 0)
    value = value.substr(4);
  if (value == "kcuda")
    return true;
  return value.rfind("kcuda:", 0) == 0;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::runtime_binding_bind_decl_t *
find_bind_by_id(
    const cuwacunu::camahjucunu::runtime_binding_instruction_t &instruction,
    const std::string &binding_id);

[[nodiscard]] inline const cuwacunu::camahjucunu::
    runtime_binding_contract_decl_t *
    find_contract_decl_by_id(
        const cuwacunu::camahjucunu::runtime_binding_instruction_t &instruction,
        const std::string &contract_id);

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
[[nodiscard]] inline runtime_binding_contract_init_record_t
invoke_runtime_binding_contract_init_from_snapshot(
    const cuwacunu::iitepi::runtime_binding_hash_t &campaign_hash,
    const std::string &binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>
        &runtime_binding_itself,
    torch::Device device = torch::kCPU);

[[nodiscard]] inline runtime_binding_contract_init_record_t
invoke_runtime_binding_contract_init_from_snapshot(
    const cuwacunu::iitepi::runtime_binding_hash_t &campaign_hash,
    const std::string &binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>
        &runtime_binding_itself,
    torch::Device device = torch::kCPU);

[[nodiscard]] inline std::optional<std::string>
runtime_binding_section_device_value(
    const std::shared_ptr<const cuwacunu::iitepi::contract_record_t>
        &contract_itself,
    std::string_view section) {
  if (!contract_itself)
    return std::nullopt;
  const auto sec_it =
      contract_itself->module_sections.find(std::string(section));
  if (sec_it == contract_itself->module_sections.end())
    return std::nullopt;
  const auto value_it = sec_it->second.find("device");
  if (value_it == sec_it->second.end())
    return std::nullopt;
  const std::string configured =
      runtime_binding_init_trim_ascii_copy(value_it->second);
  if (!has_non_ws_text(configured))
    return std::nullopt;
  return configured;
}

[[nodiscard]] inline runtime_binding_device_resolution_t
resolve_runtime_binding_preferred_device_for_binding(
    const std::string &binding_id,
    const std::shared_ptr<const cuwacunu::iitepi::runtime_binding_record_t>
        &runtime_binding_itself,
    torch::Device fallback_device = torch::kCPU) {
  runtime_binding_device_resolution_t out{};
  out.device = fallback_device;
  if (!runtime_binding_itself) {
    out.error = "missing runtime binding record";
    return out;
  }

  const auto &runtime_binding_instruction =
      runtime_binding_itself->runtime_binding.decoded();
  const auto *bind = find_bind_by_id(runtime_binding_instruction, binding_id);
  if (!bind) {
    out.error = "unknown runtime binding id: " + binding_id;
    return out;
  }
  const auto *contract_decl =
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
    if (!configured.has_value())
      continue;
    const torch::Device resolved = cuwacunu::iitepi::config_device(
        out.contract_hash, std::string(section));
    if (resolved.is_cpu() &&
        runtime_binding_requests_cuda_device_value(*configured)) {
      out.error = "CUDA explicitly requested in section '" +
                  std::string(section) + "' but runtime device resolved to CPU";
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
      log_warn("[runtime_binding.device] conflicting configured devices for "
               "contract_hash=%s binding=%s preferred=%s(%s->%s) "
               "ignored=%s(%s->%s)\n",
               runtime_binding_log_value_or_empty(out.contract_hash),
               runtime_binding_log_value_or_empty(binding_id),
               runtime_binding_log_value_or_empty(out.source_section),
               runtime_binding_log_value_or_empty(out.configured_device),
               out.device.str().c_str(), std::string(section).c_str(),
               configured->c_str(), resolved.str().c_str());
    }
  }

  if (selected)
    return out;

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
    const std::filesystem::path &contract_path,
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
    if (!configured.has_value())
      continue;
    out.device = cuwacunu::iitepi::config_device(out.contract_hash,
                                                 std::string(section));
    out.source_section = std::string(section);
    out.configured_device = *configured;
    out.resolved_from_config = true;
    if (out.device.is_cpu() &&
        runtime_binding_requests_cuda_device_value(*configured)) {
      out.error = "CUDA explicitly requested in section '" +
                  std::string(section) + "' but runtime device resolved to CPU";
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

[[nodiscard]] inline const cuwacunu::camahjucunu::runtime_binding_bind_decl_t *
find_bind_by_id(
    const cuwacunu::camahjucunu::runtime_binding_instruction_t &instruction,
    const std::string &binding_id) {
  for (const auto &bind : instruction.binds) {
    if (bind.id == binding_id)
      return &bind;
  }
  return nullptr;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::
    runtime_binding_contract_decl_t *
    find_contract_decl_by_id(
        const cuwacunu::camahjucunu::runtime_binding_instruction_t &instruction,
        const std::string &contract_id) {
  for (const auto &decl : instruction.contracts) {
    if (decl.id == contract_id)
      return &decl;
  }
  return nullptr;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::iitepi_wave_t *
find_wave_by_id(const cuwacunu::camahjucunu::iitepi_wave_set_t &wave_set,
                const std::string &wave_id) {
  for (const auto &wave : wave_set.waves) {
    if (wave.name == wave_id)
      return &wave;
  }
  return nullptr;
}

[[nodiscard]] inline std::string
runtime_binding_init_trim_ascii_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
    ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
    --e;
  return s.substr(b, e - b);
}

[[nodiscard]] inline std::string
runtime_binding_init_lower_ascii_copy(std::string s) {
  for (char &ch : s) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return s;
}
