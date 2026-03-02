// ./include/iitepi/board.validation.h
// SPDX-License-Identifier: MIT
#pragma once

#include <cctype>
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "camahjucunu/dsl/canonical_path/canonical_path.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"
#include "camahjucunu/dsl/tsiemene_wave/tsiemene_wave.h"
#include "tsiemene/tsi.type.registry.h"

namespace tsiemene {

enum class CompatibilityCode {
  InvalidContractPath,
  InvalidWavePath,
  MissingContractPath,
  MissingWavePath,
  InvalidReference,
  ProfileNotFound,
  PathMismatch,
};

enum class CompatibilitySeverity {
  Error,
  Warning,
};

struct CompatibilityIndicator {
  CompatibilityCode code{CompatibilityCode::InvalidReference};
  CompatibilitySeverity severity{CompatibilitySeverity::Error};
  std::string contract_path{};
  std::string wave_path{};
  std::string message{};
};

struct ContractValidationReport {
  bool ok{false};
  std::vector<CompatibilityIndicator> indicators{};
};

struct WaveValidationReport {
  bool ok{false};
  std::vector<CompatibilityIndicator> indicators{};
};

struct CompatibilityReport {
  bool ok{false};
  std::string contract_id{};
  std::string wave_id{};
  std::vector<CompatibilityIndicator> indicators{};
  std::size_t missing{0};
  std::size_t extra{0};
  std::size_t mismatch{0};
  std::size_t invalid_ref{0};
};

[[nodiscard]] inline std::string trim_ascii_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] inline std::string canonicalize_runtime_node_path(
    const cuwacunu::camahjucunu::canonical_path_t& path) {
  std::string out = path.canonical_identity;
  if (!path.hashimyei.empty()) {
    const std::string suffix = "." + path.hashimyei;
    if (out.size() < suffix.size() ||
        out.compare(out.size() - suffix.size(), suffix.size(), suffix) != 0) {
      out.append(suffix);
    }
  }
  return out;
}

[[nodiscard]] inline std::optional<std::string> canonical_node_path_or_nullopt(
    const std::string& raw_path,
    const std::string& contract_hash,
    std::string* error) {
  const auto parsed = cuwacunu::camahjucunu::decode_canonical_path(
      raw_path, contract_hash);
  if (!parsed.ok) {
    if (error) *error = parsed.error;
    return std::nullopt;
  }
  if (parsed.path_kind != cuwacunu::camahjucunu::canonical_path_kind_e::Node) {
    if (error) *error = "path must be canonical node";
    return std::nullopt;
  }
  if (!parse_tsi_type_id(parsed.canonical_identity).has_value()) {
    if (error) *error = "unsupported tsi type: " + parsed.canonical_identity;
    return std::nullopt;
  }
  return canonicalize_runtime_node_path(parsed);
}

[[nodiscard]] inline ContractValidationReport validate_contract_definition(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t& circuit_instruction,
    const std::string& contract_hash = {}) {
  ContractValidationReport report{};
  report.ok = true;

  std::string semantic_error;
  if (!cuwacunu::camahjucunu::validate_circuit_instruction(
          circuit_instruction, &semantic_error)) {
    report.ok = false;
    report.indicators.push_back(CompatibilityIndicator{
        .code = CompatibilityCode::InvalidReference,
        .severity = CompatibilitySeverity::Error,
        .contract_path = {},
        .wave_path = {},
        .message = semantic_error,
    });
    return report;
  }

  for (const auto& circuit : circuit_instruction.circuits) {
    for (const auto& instance : circuit.instances) {
      std::string path_error;
      const auto node_path = canonical_node_path_or_nullopt(
          instance.tsi_type, contract_hash, &path_error);
      if (!node_path.has_value()) {
        report.ok = false;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::InvalidContractPath,
            .severity = CompatibilitySeverity::Error,
            .contract_path = instance.tsi_type,
            .wave_path = {},
            .message = "invalid contract tsi_type path for alias '" + instance.alias +
                       "': " + path_error,
        });
      }
    }
  }

  return report;
}

[[nodiscard]] inline WaveValidationReport validate_wave_definition(
    const cuwacunu::camahjucunu::tsiemene_wave_t& wave,
    const std::string& contract_hash = {}) {
  WaveValidationReport report{};
  report.ok = true;

  std::unordered_set<std::string> wikimyei_paths;
  std::unordered_set<std::string> source_paths;
  bool has_train_true = false;

  for (const auto& w : wave.wikimyeis) {
    std::string path_error;
    const auto node_path =
        canonical_node_path_or_nullopt(w.wikimyei_path, contract_hash, &path_error);
    if (!node_path.has_value()) {
      report.ok = false;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidWavePath,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = w.wikimyei_path,
          .message = "invalid wave WIKIMYEI PATH: " + path_error,
      });
      continue;
    }
    if (!wikimyei_paths.insert(*node_path).second) {
      report.ok = false;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = *node_path,
          .message = "duplicate WIKIMYEI PATH in wave",
      });
    }
    if (w.train) has_train_true = true;
  }

  for (const auto& s : wave.sources) {
    std::string path_error;
    const auto node_path =
        canonical_node_path_or_nullopt(s.source_path, contract_hash, &path_error);
    if (!node_path.has_value()) {
      report.ok = false;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidWavePath,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = s.source_path,
          .message = "invalid wave SOURCE PATH: " + path_error,
      });
      continue;
    }
    if (!source_paths.insert(*node_path).second) {
      report.ok = false;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = *node_path,
          .message = "duplicate SOURCE PATH in wave",
      });
    }
  }

  auto lower_ascii_copy = [](std::string s) {
    for (char& ch : s) {
      ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return s;
  };

  const std::string mode = lower_ascii_copy(trim_ascii_copy(wave.mode));
  const std::string sampler = lower_ascii_copy(trim_ascii_copy(wave.sampler));
  if (sampler != "sequential" && sampler != "random") {
    report.ok = false;
    report.indicators.push_back(CompatibilityIndicator{
        .code = CompatibilityCode::InvalidReference,
        .severity = CompatibilitySeverity::Error,
        .contract_path = {},
        .wave_path = wave.name,
        .message = "invalid SAMPLER; expected sequential|random",
    });
  }
  if (mode == "run" && has_train_true) {
    report.ok = false;
    report.indicators.push_back(CompatibilityIndicator{
        .code = CompatibilityCode::InvalidReference,
        .severity = CompatibilitySeverity::Error,
        .contract_path = {},
        .wave_path = wave.name,
        .message = "MODE=run forbids TRAIN=true",
    });
  }
  if (mode == "train" && !has_train_true) {
    report.ok = false;
    report.indicators.push_back(CompatibilityIndicator{
        .code = CompatibilityCode::InvalidReference,
        .severity = CompatibilitySeverity::Error,
        .contract_path = {},
        .wave_path = wave.name,
        .message = "MODE=train requires at least one TRAIN=true",
    });
  }
  return report;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::jkimyei_specs_t::row_t*
find_component_profile_row(
    const cuwacunu::camahjucunu::jkimyei_specs_t& specs,
    const std::string& component_id,
    const std::string& profile_id) {
  const auto table_it = specs.tables.find("component_profiles_table");
  if (table_it == specs.tables.end()) return nullptr;
  const std::string target_component = trim_ascii_copy(component_id);
  const std::string target_profile = trim_ascii_copy(profile_id);
  for (const auto& row : table_it->second) {
    const auto cid_it = row.find("component_id");
    const auto ctype_it = row.find("component_type");
    const auto pid_it = row.find("profile_id");
    if (pid_it == row.end()) continue;
    if (trim_ascii_copy(pid_it->second) != target_profile) continue;
    const std::string row_component_id =
        (cid_it == row.end()) ? std::string{} : trim_ascii_copy(cid_it->second);
    const std::string row_component_type =
        (ctype_it == row.end()) ? std::string{} : trim_ascii_copy(ctype_it->second);
    if (row_component_id != target_component &&
        row_component_type != target_component) {
      continue;
    }
    return &row;
  }
  return nullptr;
}

[[nodiscard]] inline CompatibilityReport validate_wave_contract_compatibility(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t& circuit_instruction,
    const cuwacunu::camahjucunu::tsiemene_wave_t& wave,
    const std::string& contract_hash,
    const cuwacunu::camahjucunu::jkimyei_specs_t* jkimyei_specs = nullptr,
    std::string contract_id = {},
    std::string wave_id = {}) {
  CompatibilityReport report{};
  report.ok = true;
  report.contract_id = std::move(contract_id);
  report.wave_id = std::move(wave_id);

  std::unordered_set<std::string> contract_paths;
  std::unordered_set<std::string> contract_wikimyei_paths;
  std::unordered_set<std::string> contract_source_paths;
  for (const auto& circuit : circuit_instruction.circuits) {
    for (const auto& instance : circuit.instances) {
      std::string path_error;
      const auto parsed_path =
          canonical_node_path_or_nullopt(instance.tsi_type, contract_hash, &path_error);
      if (!parsed_path.has_value()) {
        report.ok = false;
        ++report.invalid_ref;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::InvalidContractPath,
            .severity = CompatibilitySeverity::Error,
            .contract_path = instance.tsi_type,
            .wave_path = {},
            .message = "invalid contract path for alias '" + instance.alias + "': " +
                       path_error,
        });
        continue;
      }
      contract_paths.insert(*parsed_path);
      const auto parsed_canonical = cuwacunu::camahjucunu::decode_canonical_path(
          *parsed_path, contract_hash);
      const auto type_id = parse_tsi_type_id(parsed_canonical.canonical_identity);
      if (!type_id.has_value()) continue;
      if (tsi_type_domain(*type_id) == TsiDomain::Wikimyei) {
        contract_wikimyei_paths.insert(*parsed_path);
      } else if (tsi_type_domain(*type_id) == TsiDomain::Source) {
        contract_source_paths.insert(*parsed_path);
      }
    }
  }

  std::unordered_set<std::string> wave_paths;
  std::unordered_set<std::string> wave_wikimyei_paths;
  std::unordered_set<std::string> wave_source_paths;
  for (const auto& w : wave.wikimyeis) {
    std::string path_error;
    const auto parsed_path =
        canonical_node_path_or_nullopt(w.wikimyei_path, contract_hash, &path_error);
    if (!parsed_path.has_value()) {
      report.ok = false;
      ++report.invalid_ref;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidWavePath,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = w.wikimyei_path,
          .message = "invalid wave wikimyei path: " + path_error,
      });
      continue;
    }
    wave_paths.insert(*parsed_path);
    wave_wikimyei_paths.insert(*parsed_path);
    if (w.train && jkimyei_specs) {
      bool profile_found = false;
      const auto parsed_canonical = cuwacunu::camahjucunu::decode_canonical_path(
          *parsed_path, contract_hash);
      std::vector<std::string> component_id_candidates{};
      if (parsed_canonical.ok) {
        component_id_candidates.push_back(parsed_canonical.canonical_identity);
        if (const auto type_id =
                parse_tsi_type_id(parsed_canonical.canonical_identity);
            type_id.has_value()) {
          component_id_candidates.emplace_back(tsi_type_token(*type_id));
        }
      }
      component_id_candidates.push_back(*parsed_path);
      component_id_candidates.push_back(w.wikimyei_path);
      if (const auto type_id = parse_tsi_type_id(*parsed_path);
          type_id.has_value()) {
        component_id_candidates.emplace_back(tsi_type_token(*type_id));
      }
      for (const auto& cid : component_id_candidates) {
        if (find_component_profile_row(*jkimyei_specs, cid, w.profile_id)) {
          profile_found = true;
          break;
        }
      }
      if (!profile_found) {
        std::string candidates{};
        for (std::size_t ci = 0; ci < component_id_candidates.size(); ++ci) {
          if (ci != 0) candidates.append(", ");
          candidates.append(component_id_candidates[ci]);
        }
        report.ok = false;
        ++report.invalid_ref;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::ProfileNotFound,
            .severity = CompatibilitySeverity::Error,
            .contract_path = *parsed_path,
            .wave_path = *parsed_path,
            .message = "PROFILE_ID not found for TRAIN=true wikimyei path: " +
                       w.profile_id + " (component candidates: [" + candidates +
                       "])",
        });
      }
    }
  }
  for (const auto& s : wave.sources) {
    std::string path_error;
    const auto parsed_path =
        canonical_node_path_or_nullopt(s.source_path, contract_hash, &path_error);
    if (!parsed_path.has_value()) {
      report.ok = false;
      ++report.invalid_ref;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidWavePath,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = s.source_path,
          .message = "invalid wave source path: " + path_error,
      });
      continue;
    }
    wave_paths.insert(*parsed_path);
    wave_source_paths.insert(*parsed_path);
  }

  if (contract_source_paths.size() != 1) {
    report.ok = false;
    ++report.invalid_ref;
    report.indicators.push_back(CompatibilityIndicator{
        .code = CompatibilityCode::InvalidReference,
        .severity = CompatibilitySeverity::Error,
        .contract_path = {},
        .wave_path = {},
        .message =
            "runtime currently supports exactly one SOURCE path per circuit",
    });
  }
  if (wave_source_paths.size() != 1) {
    report.ok = false;
    ++report.invalid_ref;
    report.indicators.push_back(CompatibilityIndicator{
        .code = CompatibilityCode::InvalidReference,
        .severity = CompatibilitySeverity::Error,
        .contract_path = {},
        .wave_path = {},
        .message =
            "runtime currently supports exactly one SOURCE PATH in selected wave",
    });
  }

  for (const auto& wave_path : wave_wikimyei_paths) {
    if (contract_wikimyei_paths.find(wave_path) == contract_wikimyei_paths.end()) {
      report.ok = false;
      ++report.missing;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::MissingContractPath,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = wave_path,
          .message = "wave wikimyei path not present in contract",
      });
    }
  }
  for (const auto& wave_path : wave_source_paths) {
    if (contract_source_paths.find(wave_path) == contract_source_paths.end()) {
      report.ok = false;
      ++report.missing;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::MissingContractPath,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = wave_path,
          .message = "wave source path not present in contract",
      });
    }
  }

  for (const auto& contract_path : contract_wikimyei_paths) {
    if (wave_wikimyei_paths.find(contract_path) == wave_wikimyei_paths.end()) {
      report.ok = false;
      ++report.extra;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::MissingWavePath,
          .severity = CompatibilitySeverity::Error,
          .contract_path = contract_path,
          .wave_path = {},
          .message = "contract wikimyei path missing in wave declaration",
      });
    }
  }
  for (const auto& contract_path : contract_source_paths) {
    if (wave_source_paths.find(contract_path) == wave_source_paths.end()) {
      report.ok = false;
      ++report.extra;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::MissingWavePath,
          .severity = CompatibilitySeverity::Error,
          .contract_path = contract_path,
          .wave_path = {},
          .message = "contract source path missing in wave declaration",
      });
    }
  }

  return report;
}

}  // namespace tsiemene
