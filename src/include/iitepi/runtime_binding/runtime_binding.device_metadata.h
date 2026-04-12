// ./include/iitepi/runtime_binding/runtime_binding.device_metadata.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "iitepi/iitepi.h"

namespace tsiemene {

struct runtime_binding_device_metadata_t {
  std::string resolved_device{"cpu"};
  std::string contract_hash{};
  std::string source_section{};
  std::string configured_device{};
  std::string error{};
  bool resolved_from_config{false};
  bool cuda_required{false};
};

[[nodiscard]] inline std::string runtime_binding_device_metadata_trim_ascii_copy(
    std::string s) {
  std::size_t begin = 0;
  while (begin < s.size() &&
         std::isspace(static_cast<unsigned char>(s[begin])) != 0) {
    ++begin;
  }
  std::size_t end = s.size();
  while (end > begin &&
         std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
    --end;
  }
  return s.substr(begin, end - begin);
}

[[nodiscard]] inline bool runtime_binding_device_metadata_has_non_ws_text(
    std::string_view s) {
  for (const unsigned char c : s) {
    if (std::isspace(c) == 0) return true;
  }
  return false;
}

[[nodiscard]] inline bool runtime_binding_device_metadata_requests_cuda_value(
    std::string_view raw_value) {
  std::string value(raw_value);
  value = runtime_binding_device_metadata_trim_ascii_copy(std::move(value));
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

[[nodiscard]] inline std::string
runtime_binding_device_metadata_normalize_label(std::string value) {
  value = runtime_binding_device_metadata_trim_ascii_copy(std::move(value));
  if (value.empty()) return {};
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::tolower(c));
  });
  if (value.rfind("torch::", 0) == 0) value = value.substr(7);
  if (value.rfind("at::", 0) == 0) value = value.substr(4);
  if (value == "gpu" || value == "cuda" || value == "kcuda") return "cuda";
  if (value.rfind("gpu:", 0) == 0) return "cuda:" + value.substr(4);
  if (value.rfind("cuda:", 0) == 0) return value;
  if (value.rfind("kcuda:", 0) == 0) return "cuda:" + value.substr(6);
  if (value == "kcpu") return "cpu";
  return value;
}

[[nodiscard]] inline std::optional<std::string>
runtime_binding_device_metadata_section_value(
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
      runtime_binding_device_metadata_trim_ascii_copy(value_it->second);
  if (!runtime_binding_device_metadata_has_non_ws_text(configured)) {
    return std::nullopt;
  }
  return configured;
}

[[nodiscard]] inline runtime_binding_device_metadata_t
resolve_runtime_binding_preferred_device_metadata_for_contract_path(
    const std::filesystem::path& contract_path,
    std::string fallback_device = "cpu") {
  runtime_binding_device_metadata_t out{};
  const std::string normalized_fallback =
      runtime_binding_device_metadata_normalize_label(
          runtime_binding_device_metadata_trim_ascii_copy(
              std::move(fallback_device)));
  if (!normalized_fallback.empty()) out.resolved_device = normalized_fallback;
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
        runtime_binding_device_metadata_section_value(contract_itself, section);
    if (!configured.has_value()) continue;
    out.source_section = std::string(section);
    out.configured_device = *configured;
    out.resolved_from_config = true;
    out.cuda_required =
        runtime_binding_device_metadata_requests_cuda_value(*configured);
    const std::string normalized =
        runtime_binding_device_metadata_normalize_label(*configured);
    if (!normalized.empty()) out.resolved_device = normalized;
    return out;
  }

  const auto general_section_it =
      cuwacunu::iitepi::config_space_t::config.find("GENERAL");
  if (general_section_it != cuwacunu::iitepi::config_space_t::config.end()) {
    const auto device_it = general_section_it->second.find("device");
    if (device_it != general_section_it->second.end()) {
      const std::string configured =
          runtime_binding_device_metadata_trim_ascii_copy(device_it->second);
      if (runtime_binding_device_metadata_has_non_ws_text(configured)) {
        out.source_section = "GENERAL";
        out.configured_device = configured;
        out.resolved_from_config = true;
        out.cuda_required =
            runtime_binding_device_metadata_requests_cuda_value(configured);
        const std::string normalized =
            runtime_binding_device_metadata_normalize_label(configured);
        if (!normalized.empty()) out.resolved_device = normalized;
      }
    }
  }

  return out;
}

}  // namespace tsiemene
