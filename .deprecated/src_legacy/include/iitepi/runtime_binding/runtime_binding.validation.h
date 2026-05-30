// ./include/iitepi/runtime_binding/runtime_binding.validation.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "camahjucunu/dsl/canonical_path/canonical_path.h"
#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"
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
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
    ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
    --e;
  return s.substr(b, e - b);
}

[[nodiscard]] inline std::string canonicalize_runtime_node_path(
    const cuwacunu::camahjucunu::canonical_path_t &path) {
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

struct node_path_ref_t {
  std::string runtime_path{};
  std::string canonical_identity{};
  std::string hashimyei{};
  TsiTypeId type_id{TsiTypeId::SourceDataloader};
};

[[nodiscard]] inline std::optional<node_path_ref_t>
resolve_node_path_or_nullopt(const std::string &raw_path,
                             const std::string &contract_hash,
                             bool allow_family_without_hash,
                             std::string *error) {
  const auto parsed =
      contract_hash.empty()
          ? cuwacunu::camahjucunu::decode_canonical_path(raw_path)
          : cuwacunu::camahjucunu::decode_canonical_path(raw_path,
                                                         contract_hash);
  if (parsed.ok) {
    if (parsed.path_kind !=
        cuwacunu::camahjucunu::canonical_path_kind_e::Node) {
      if (error)
        *error = "path must be canonical node";
      return std::nullopt;
    }
    const auto type_id = parse_tsi_type_id(parsed.canonical_identity);
    if (!type_id.has_value()) {
      if (error)
        *error = "unsupported tsi type: " + parsed.canonical_identity;
      return std::nullopt;
    }
    return node_path_ref_t{
        .runtime_path = canonicalize_runtime_node_path(parsed),
        .canonical_identity = parsed.canonical_identity,
        .hashimyei = parsed.hashimyei,
        .type_id = *type_id,
    };
  }

  if (!allow_family_without_hash) {
    if (error)
      *error = parsed.error;
    return std::nullopt;
  }

  const std::string trimmed = trim_ascii_copy(raw_path);
  const auto type_id = parse_tsi_type_id(trimmed);
  if (!type_id.has_value()) {
    if (error)
      *error = parsed.error;
    return std::nullopt;
  }

  const std::string canonical_type = std::string(tsi_type_token(*type_id));
  if (trimmed != canonical_type) {
    if (error) {
      *error = "contract family path must be canonical type token: " +
               canonical_type;
    }
    return std::nullopt;
  }

  if (tsi_type_instance_policy(*type_id) !=
      TsiInstancePolicy::HashimyeiInstances) {
    if (error)
      *error = parsed.error;
    return std::nullopt;
  }

  return node_path_ref_t{
      .runtime_path = canonical_type,
      .canonical_identity = canonical_type,
      .hashimyei = {},
      .type_id = *type_id,
  };
}

[[nodiscard]] inline bool
contract_wave_path_match(const node_path_ref_t &contract_path,
                         const node_path_ref_t &wave_path) {
  const auto canonical_family_path = [](const node_path_ref_t &path) {
    const auto type_id = parse_tsi_type_id(path.canonical_identity);
    if (type_id.has_value()) {
      return std::string(tsi_type_token(*type_id));
    }
    return path.canonical_identity;
  };
  if (canonical_family_path(contract_path) !=
      canonical_family_path(wave_path)) {
    return false;
  }
  if (!contract_path.hashimyei.empty() &&
      contract_path.hashimyei != wave_path.hashimyei) {
    return false;
  }
  return true;
}

[[nodiscard]] inline std::optional<std::string>
canonical_node_path_or_nullopt(const std::string &raw_path,
                               const std::string &contract_hash,
                               std::string *error) {
  const auto resolved =
      resolve_node_path_or_nullopt(raw_path, contract_hash,
                                   /*allow_family_without_hash=*/false, error);
  if (!resolved.has_value())
    return std::nullopt;
  return resolved->runtime_path;
}

[[nodiscard]] inline bool select_effective_contract_circuit_instruction(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t &instruction,
    std::string_view wave_circuit_name,
    cuwacunu::camahjucunu::tsiemene_circuit_instruction_t *out,
    std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!out) {
    if (error)
      *error = "missing effective circuit output";
    return false;
  }
  *out = instruction;

  std::string selected_name = trim_ascii_copy(std::string(wave_circuit_name));
  if (selected_name.empty()) {
    if (instruction.circuits.empty()) {
      if (error)
        *error = "contract has no circuit declarations";
      return false;
    }
    if (instruction.circuits.size() == 1) {
      out->circuits = {instruction.circuits.front()};
      return true;
    }
    if (error) {
      *error = "contract declares multiple circuits; wave must set CIRCUIT to "
               "select one";
    }
    return false;
  }

  for (const auto &circuit : instruction.circuits) {
    if (trim_ascii_copy(circuit.name) != selected_name)
      continue;
    out->circuits = {circuit};
    return true;
  }

  if (error) {
    *error = "CIRCUIT selector '" + selected_name +
             "' does not match any contract circuit declaration";
  }
  return false;
}

[[nodiscard]] inline ContractValidationReport validate_contract_definition(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t
        &circuit_instruction,
    const std::string &contract_hash = {}) {
  ContractValidationReport report{};
  report.ok = true;

  std::string semantic_error;
  if (!cuwacunu::camahjucunu::validate_circuit_instruction(circuit_instruction,
                                                           &semantic_error)) {
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

  for (const auto &circuit : circuit_instruction.circuits) {
    for (const auto &instance : circuit.instances) {
      std::string path_error;
      const auto node_path = resolve_node_path_or_nullopt(
          instance.tsi_type, contract_hash,
          /*allow_family_without_hash=*/true, &path_error);
      if (!node_path.has_value()) {
        report.ok = false;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::InvalidContractPath,
            .severity = CompatibilitySeverity::Error,
            .contract_path = instance.tsi_type,
            .wave_path = {},
            .message = "invalid contract tsi_type path for alias '" +
                       instance.alias + "': " + path_error,
        });
      }
    }
  }

  return report;
}

[[nodiscard]] inline WaveValidationReport
validate_wave_definition(const cuwacunu::camahjucunu::iitepi_wave_t &wave,
                         const std::string &contract_hash = {}) {
  WaveValidationReport report{};
  report.ok = true;

  std::unordered_set<std::string> wikimyei_paths;
  std::unordered_set<std::string> source_paths;
  std::unordered_set<std::string> probe_paths;
  std::unordered_set<std::string> sink_paths;
  std::unordered_set<std::string> binding_ids;
  bool has_train_true = false;

  const auto register_binding_id = [&](const char *component_kind,
                                       const std::string &binding_id,
                                       const std::string &runtime_path) {
    const std::string id = trim_ascii_copy(binding_id);
    if (id.empty()) {
      report.ok = false;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = runtime_path,
          .message = std::string("missing binding id for ") + component_kind +
                     " component (expected <id>)",
      });
      return;
    }
    if (!binding_ids.insert(id).second) {
      report.ok = false;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = runtime_path,
          .message = "duplicate component binding id in wave: " + id,
      });
    }
  };

  for (const auto &w : wave.wikimyeis) {
    register_binding_id("WIKIMYEI", w.binding_id, w.wikimyei_path);
    std::string path_error;
    const auto node_path = canonical_node_path_or_nullopt(
        w.wikimyei_path, contract_hash, &path_error);
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
    if (w.train)
      has_train_true = true;
  }

  for (const auto &s : wave.sources) {
    register_binding_id("SOURCE", s.binding_id, s.source_path);
    std::string path_error;
    const auto node_path = canonical_node_path_or_nullopt(
        s.source_path, contract_hash, &path_error);
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

  for (const auto &p : wave.probes) {
    register_binding_id("PROBE", p.binding_id, p.probe_path);
    std::string path_error;
    const auto node_path = canonical_node_path_or_nullopt(
        p.probe_path, contract_hash, &path_error);
    if (!node_path.has_value()) {
      report.ok = false;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidWavePath,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = p.probe_path,
          .message = "invalid wave PROBE PATH: " + path_error,
      });
      continue;
    }
    if (!probe_paths.insert(*node_path).second) {
      report.ok = false;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = *node_path,
          .message = "duplicate PROBE PATH in wave",
      });
    }
  }

  for (const auto &s : wave.sinks) {
    register_binding_id("SINK", s.binding_id, s.sink_path);
    std::string path_error;
    const auto node_path =
        canonical_node_path_or_nullopt(s.sink_path, contract_hash, &path_error);
    if (!node_path.has_value()) {
      report.ok = false;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidWavePath,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = s.sink_path,
          .message = "invalid wave SINK PATH: " + path_error,
      });
      continue;
    }
    if (!sink_paths.insert(*node_path).second) {
      report.ok = false;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = *node_path,
          .message = "duplicate SINK PATH in wave",
      });
    }
  }

  auto lower_ascii_copy = [](std::string s) {
    for (char &ch : s) {
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
  std::uint64_t mode_flags = wave.mode_flags;
  if (mode_flags == 0 && !mode.empty() && mode != "none") {
    std::string mode_error;
    if (!cuwacunu::camahjucunu::parse_iitepi_wave_mode_flags(
            mode, &mode_flags, nullptr, &mode_error)) {
      report.ok = false;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = wave.name,
          .message = "invalid MODE: " + mode_error,
      });
    }
  }

  const bool run_enabled = cuwacunu::camahjucunu::iitepi_wave_mode_has(
      mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Run);
  const bool train_enabled = cuwacunu::camahjucunu::iitepi_wave_mode_has(
      mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Train);
  if (!run_enabled && !train_enabled) {
    report.ok = false;
    report.indicators.push_back(CompatibilityIndicator{
        .code = CompatibilityCode::InvalidReference,
        .severity = CompatibilitySeverity::Error,
        .contract_path = {},
        .wave_path = wave.name,
        .message = "MODE must enable run or train",
    });
  }
  if (run_enabled && !train_enabled && has_train_true) {
    report.ok = false;
    report.indicators.push_back(CompatibilityIndicator{
        .code = CompatibilityCode::InvalidReference,
        .severity = CompatibilitySeverity::Error,
        .contract_path = {},
        .wave_path = wave.name,
        .message = "MODE without train bit forbids JKIMYEI.HALT_TRAIN=false",
    });
  }
  if (train_enabled && !has_train_true) {
    report.ok = false;
    report.indicators.push_back(CompatibilityIndicator{
        .code = CompatibilityCode::InvalidReference,
        .severity = CompatibilitySeverity::Error,
        .contract_path = {},
        .wave_path = wave.name,
        .message = "MODE with train bit requires at least one WIKIMYEI with "
                   "JKIMYEI.HALT_TRAIN=false",
    });
  }
  return report;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::jkimyei_specs_t::row_t *
find_component_profile_row(const cuwacunu::camahjucunu::jkimyei_specs_t &specs,
                           const std::string &component_id,
                           const std::string &profile_id) {
  const auto table_it = specs.tables.find("component_profiles_table");
  if (table_it == specs.tables.end())
    return nullptr;
  const std::string target_component = trim_ascii_copy(component_id);
  const std::string target_profile = trim_ascii_copy(profile_id);
  for (const auto &row : table_it->second) {
    const auto cid_it = row.find("component_id");
    const auto ctype_it = row.find("component_type");
    const auto pid_it = row.find("profile_id");
    if (pid_it == row.end())
      continue;
    if (trim_ascii_copy(pid_it->second) != target_profile)
      continue;
    const std::string row_component_id =
        (cid_it == row.end()) ? std::string{} : trim_ascii_copy(cid_it->second);
    const std::string row_component_type =
        (ctype_it == row.end()) ? std::string{}
                                : trim_ascii_copy(ctype_it->second);
    if (row_component_id != target_component &&
        row_component_type != target_component) {
      continue;
    }
    return &row;
  }
  return nullptr;
}

[[nodiscard]] inline std::vector<std::string> collect_component_profile_ids(
    const cuwacunu::camahjucunu::jkimyei_specs_t &specs,
    const std::vector<std::string> &component_ids) {
  std::vector<std::string> out{};
  const auto table_it = specs.tables.find("component_profiles_table");
  if (table_it == specs.tables.end())
    return out;

  std::unordered_set<std::string> wanted_components{};
  wanted_components.reserve(component_ids.size());
  for (const auto &component_id : component_ids) {
    const std::string normalized = trim_ascii_copy(component_id);
    if (!normalized.empty())
      wanted_components.insert(normalized);
  }
  if (wanted_components.empty())
    return out;

  std::unordered_set<std::string> seen_profiles{};
  for (const auto &row : table_it->second) {
    const auto pid_it = row.find("profile_id");
    if (pid_it == row.end())
      continue;
    const auto cid_it = row.find("component_id");
    const auto ctype_it = row.find("component_type");
    const std::string row_component_id =
        (cid_it == row.end()) ? std::string{} : trim_ascii_copy(cid_it->second);
    const std::string row_component_type =
        (ctype_it == row.end()) ? std::string{}
                                : trim_ascii_copy(ctype_it->second);
    if (wanted_components.find(row_component_id) == wanted_components.end() &&
        wanted_components.find(row_component_type) == wanted_components.end()) {
      continue;
    }
    const std::string profile_id = trim_ascii_copy(pid_it->second);
    if (profile_id.empty())
      continue;
    if (!seen_profiles.insert(profile_id).second)
      continue;
    out.push_back(profile_id);
  }
  std::sort(out.begin(), out.end());
  return out;
}

[[nodiscard]] inline CompatibilityReport validate_wave_contract_compatibility(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t
        &circuit_instruction,
    const cuwacunu::camahjucunu::iitepi_wave_t &wave,
    const std::string &contract_hash,
    const cuwacunu::camahjucunu::jkimyei_specs_t *jkimyei_specs = nullptr,
    std::string contract_id = {}, std::string wave_id = {}) {
  CompatibilityReport report{};
  report.ok = true;
  report.contract_id = std::move(contract_id);
  report.wave_id = std::move(wave_id);

  cuwacunu::camahjucunu::tsiemene_circuit_instruction_t effective_instruction{};
  std::string circuit_error{};
  if (!select_effective_contract_circuit_instruction(
          circuit_instruction, wave.circuit_name, &effective_instruction,
          &circuit_error)) {
    report.ok = false;
    ++report.invalid_ref;
    report.indicators.push_back(CompatibilityIndicator{
        .code = CompatibilityCode::InvalidReference,
        .severity = CompatibilitySeverity::Error,
        .contract_path = {},
        .wave_path = wave.circuit_name,
        .message = circuit_error,
    });
    return report;
  }

  std::vector<node_path_ref_t> contract_wikimyei_paths{};
  std::vector<node_path_ref_t> contract_source_paths{};
  std::vector<node_path_ref_t> contract_probe_paths{};
  std::vector<node_path_ref_t> contract_sink_paths{};
  std::unordered_set<std::string> contract_wikimyei_seen{};
  std::unordered_set<std::string> contract_source_seen{};
  std::unordered_set<std::string> contract_probe_seen{};
  std::unordered_set<std::string> contract_sink_seen{};
  std::unordered_map<std::string, node_path_ref_t> contract_binding_paths{};
  std::size_t contract_source_component_count{0};

  const auto push_unique = [&](std::vector<node_path_ref_t> *out,
                               std::unordered_set<std::string> *seen,
                               const node_path_ref_t &value) {
    if (!out || !seen)
      return;
    if (seen->insert(value.runtime_path).second) {
      out->push_back(value);
    }
  };

  for (const auto &circuit : effective_instruction.circuits) {
    for (const auto &instance : circuit.instances) {
      std::string path_error;
      const auto parsed_path = resolve_node_path_or_nullopt(
          instance.tsi_type, contract_hash,
          /*allow_family_without_hash=*/true, &path_error);
      if (!parsed_path.has_value()) {
        report.ok = false;
        ++report.invalid_ref;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::InvalidContractPath,
            .severity = CompatibilitySeverity::Error,
            .contract_path = instance.tsi_type,
            .wave_path = {},
            .message = "invalid contract path for alias '" + instance.alias +
                       "': " + path_error,
        });
        continue;
      }
      const std::string binding_id = trim_ascii_copy(instance.alias);
      if (binding_id.empty()) {
        report.ok = false;
        ++report.invalid_ref;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::InvalidReference,
            .severity = CompatibilitySeverity::Error,
            .contract_path = parsed_path->runtime_path,
            .wave_path = {},
            .message = "empty circuit binding id is not allowed",
        });
      } else if (!contract_binding_paths.emplace(binding_id, *parsed_path)
                      .second) {
        report.ok = false;
        ++report.invalid_ref;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::InvalidReference,
            .severity = CompatibilitySeverity::Error,
            .contract_path = parsed_path->runtime_path,
            .wave_path = {},
            .message =
                "duplicated circuit binding id in contract: " + binding_id,
        });
      }
      if (tsi_type_domain(parsed_path->type_id) == TsiDomain::Wikimyei) {
        push_unique(&contract_wikimyei_paths, &contract_wikimyei_seen,
                    *parsed_path);
      } else if (tsi_type_domain(parsed_path->type_id) == TsiDomain::Source) {
        ++contract_source_component_count;
        push_unique(&contract_source_paths, &contract_source_seen,
                    *parsed_path);
      } else if (tsi_type_domain(parsed_path->type_id) == TsiDomain::Probe) {
        push_unique(&contract_probe_paths, &contract_probe_seen, *parsed_path);
      } else if (tsi_type_domain(parsed_path->type_id) == TsiDomain::Sink) {
        push_unique(&contract_sink_paths, &contract_sink_seen, *parsed_path);
      }
    }
  }

  std::vector<node_path_ref_t> wave_wikimyei_paths{};
  std::vector<node_path_ref_t> wave_source_paths{};
  std::vector<node_path_ref_t> wave_probe_paths{};
  std::vector<node_path_ref_t> wave_sink_paths{};
  std::unordered_set<std::string> wave_wikimyei_seen{};
  std::unordered_set<std::string> wave_source_seen{};
  std::unordered_set<std::string> wave_probe_seen{};
  std::unordered_set<std::string> wave_sink_seen{};
  std::unordered_map<std::string, node_path_ref_t> wave_binding_paths{};
  std::size_t wave_source_component_count{0};

  for (const auto &w : wave.wikimyeis) {
    const std::string binding_id = trim_ascii_copy(w.binding_id);
    if (binding_id.empty()) {
      report.ok = false;
      ++report.invalid_ref;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = w.wikimyei_path,
          .message = "missing WIKIMYEI binding id in wave declaration",
      });
    }
    std::string path_error;
    const auto parsed_path = resolve_node_path_or_nullopt(
        w.wikimyei_path, contract_hash,
        /*allow_family_without_hash=*/false, &path_error);
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
    if (!binding_id.empty() &&
        !wave_binding_paths.emplace(binding_id, *parsed_path).second) {
      report.ok = false;
      ++report.invalid_ref;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = parsed_path->runtime_path,
          .message = "duplicate wave binding id: " + binding_id,
      });
    }
    push_unique(&wave_wikimyei_paths, &wave_wikimyei_seen, *parsed_path);
    if (jkimyei_specs) {
      const auto parsed_canonical =
          cuwacunu::camahjucunu::decode_canonical_path(
              parsed_path->runtime_path, contract_hash);
      std::vector<std::string> component_id_candidates{};
      if (parsed_canonical.ok) {
        component_id_candidates.push_back(parsed_canonical.canonical_identity);
        if (const auto type_id =
                parse_tsi_type_id(parsed_canonical.canonical_identity);
            type_id.has_value()) {
          component_id_candidates.emplace_back(tsi_type_token(*type_id));
        }
      }
      component_id_candidates.push_back(parsed_path->runtime_path);
      component_id_candidates.push_back(w.wikimyei_path);
      if (const auto type_id = parse_tsi_type_id(parsed_path->runtime_path);
          type_id.has_value()) {
        component_id_candidates.emplace_back(tsi_type_token(*type_id));
      }
      const std::string requested_profile_id = trim_ascii_copy(w.profile_id);
      if (requested_profile_id.empty()) {
        const std::vector<std::string> profile_ids =
            collect_component_profile_ids(*jkimyei_specs,
                                          component_id_candidates);
        if (profile_ids.size() == 1) {
          continue;
        }
        std::string candidates{};
        for (std::size_t ci = 0; ci < component_id_candidates.size(); ++ci) {
          if (ci != 0)
            candidates.append(", ");
          candidates.append(component_id_candidates[ci]);
        }
        std::string profile_list{};
        for (std::size_t pi = 0; pi < profile_ids.size(); ++pi) {
          if (pi != 0)
            profile_list.append(", ");
          profile_list.append(profile_ids[pi]);
        }
        report.ok = false;
        ++report.invalid_ref;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::ProfileNotFound,
            .severity = CompatibilitySeverity::Error,
            .contract_path = parsed_path->runtime_path,
            .wave_path = parsed_path->runtime_path,
            .message = profile_ids.empty()
                           ? "JKIMYEI.PROFILE_ID omitted for wikimyei path and "
                             "no compatible contract profile was found "
                             "(component candidates: [" +
                                 candidates + "])"
                           : "JKIMYEI.PROFILE_ID omitted for wikimyei path and "
                             "multiple compatible contract profiles exist: [" +
                                 profile_list + "] (component candidates: [" +
                                 candidates + "])",
        });
        continue;
      }
      bool profile_found = false;
      for (const auto &cid : component_id_candidates) {
        if (find_component_profile_row(*jkimyei_specs, cid,
                                       requested_profile_id)) {
          profile_found = true;
          break;
        }
      }
      if (!profile_found) {
        std::string candidates{};
        for (std::size_t ci = 0; ci < component_id_candidates.size(); ++ci) {
          if (ci != 0)
            candidates.append(", ");
          candidates.append(component_id_candidates[ci]);
        }
        report.ok = false;
        ++report.invalid_ref;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::ProfileNotFound,
            .severity = CompatibilitySeverity::Error,
            .contract_path = parsed_path->runtime_path,
            .wave_path = parsed_path->runtime_path,
            .message = "PROFILE_ID not found for wikimyei path: " +
                       requested_profile_id + " (component candidates: [" +
                       candidates + "])",
        });
      }
    }
  }
  for (const auto &s : wave.sources) {
    ++wave_source_component_count;
    const std::string binding_id = trim_ascii_copy(s.binding_id);
    if (binding_id.empty()) {
      report.ok = false;
      ++report.invalid_ref;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = s.source_path,
          .message = "missing SOURCE binding id in wave declaration",
      });
    }
    std::string path_error;
    const auto parsed_path = resolve_node_path_or_nullopt(
        s.source_path, contract_hash,
        /*allow_family_without_hash=*/false, &path_error);
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
    if (!binding_id.empty() &&
        !wave_binding_paths.emplace(binding_id, *parsed_path).second) {
      report.ok = false;
      ++report.invalid_ref;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = parsed_path->runtime_path,
          .message = "duplicate wave binding id: " + binding_id,
      });
    }
    push_unique(&wave_source_paths, &wave_source_seen, *parsed_path);
  }
  for (const auto &p : wave.probes) {
    const std::string binding_id = trim_ascii_copy(p.binding_id);
    if (binding_id.empty()) {
      report.ok = false;
      ++report.invalid_ref;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = p.probe_path,
          .message = "missing PROBE binding id in wave declaration",
      });
    }
    std::string path_error;
    const auto parsed_path = resolve_node_path_or_nullopt(
        p.probe_path, contract_hash,
        /*allow_family_without_hash=*/false, &path_error);
    if (!parsed_path.has_value()) {
      report.ok = false;
      ++report.invalid_ref;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidWavePath,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = p.probe_path,
          .message = "invalid wave probe path: " + path_error,
      });
      continue;
    }
    if (!binding_id.empty() &&
        !wave_binding_paths.emplace(binding_id, *parsed_path).second) {
      report.ok = false;
      ++report.invalid_ref;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = parsed_path->runtime_path,
          .message = "duplicate wave binding id: " + binding_id,
      });
    }
    push_unique(&wave_probe_paths, &wave_probe_seen, *parsed_path);
  }
  for (const auto &s : wave.sinks) {
    const std::string binding_id = trim_ascii_copy(s.binding_id);
    if (binding_id.empty()) {
      report.ok = false;
      ++report.invalid_ref;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = s.sink_path,
          .message = "missing SINK binding id in wave declaration",
      });
    }
    std::string path_error;
    const auto parsed_path = resolve_node_path_or_nullopt(
        s.sink_path, contract_hash,
        /*allow_family_without_hash=*/false, &path_error);
    if (!parsed_path.has_value()) {
      report.ok = false;
      ++report.invalid_ref;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidWavePath,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = s.sink_path,
          .message = "invalid wave sink path: " + path_error,
      });
      continue;
    }
    if (!binding_id.empty() &&
        !wave_binding_paths.emplace(binding_id, *parsed_path).second) {
      report.ok = false;
      ++report.invalid_ref;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::InvalidReference,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = parsed_path->runtime_path,
          .message = "duplicate wave binding id: " + binding_id,
      });
    }
    push_unique(&wave_sink_paths, &wave_sink_seen, *parsed_path);
  }

  if (contract_source_component_count != 1) {
    report.ok = false;
    ++report.invalid_ref;
    report.indicators.push_back(CompatibilityIndicator{
        .code = CompatibilityCode::InvalidReference,
        .severity = CompatibilitySeverity::Error,
        .contract_path = {},
        .wave_path = {},
        .message = "runtime requires exactly one SOURCE component in contract "
                   "circuit for wave binding",
    });
  }
  if (wave_source_component_count != 1U) {
    report.ok = false;
    ++report.invalid_ref;
    report.indicators.push_back(CompatibilityIndicator{
        .code = CompatibilityCode::InvalidReference,
        .severity = CompatibilitySeverity::Error,
        .contract_path = {},
        .wave_path = {},
        .message = "runtime requires exactly one SOURCE block in selected wave "
                   "for contract binding",
    });
  }

  const auto has_contract_match =
      [&](const std::vector<node_path_ref_t> &contract_paths,
          const node_path_ref_t &wave_path) {
        return std::any_of(contract_paths.begin(), contract_paths.end(),
                           [&](const node_path_ref_t &contract_path) {
                             return contract_wave_path_match(contract_path,
                                                             wave_path);
                           });
      };
  const auto has_wave_match =
      [&](const std::vector<node_path_ref_t> &wave_paths,
          const node_path_ref_t &contract_path) {
        return std::any_of(wave_paths.begin(), wave_paths.end(),
                           [&](const node_path_ref_t &wave_path) {
                             return contract_wave_path_match(contract_path,
                                                             wave_path);
                           });
      };

  for (const auto &[wave_binding_id, wave_path] : wave_binding_paths) {
    const auto contract_it = contract_binding_paths.find(wave_binding_id);
    if (contract_it == contract_binding_paths.end()) {
      report.ok = false;
      ++report.missing;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::MissingContractPath,
          .severity = CompatibilitySeverity::Error,
          .contract_path = {},
          .wave_path = wave_path.runtime_path,
          .message = "wave binding id '" + wave_binding_id +
                     "' is not acknowledged by contract circuit",
      });
      continue;
    }
    if (!contract_wave_path_match(contract_it->second, wave_path)) {
      report.ok = false;
      ++report.mismatch;
      report.indicators.push_back(CompatibilityIndicator{
          .code = CompatibilityCode::PathMismatch,
          .severity = CompatibilitySeverity::Error,
          .contract_path = contract_it->second.runtime_path,
          .wave_path = wave_path.runtime_path,
          .message = "binding id '" + wave_binding_id +
                     "' has mismatched family/path between contract and wave",
      });
    }
  }

  for (const auto &[contract_binding_id, contract_path] :
       contract_binding_paths) {
    if (wave_binding_paths.find(contract_binding_id) !=
        wave_binding_paths.end()) {
      continue;
    }
    report.ok = false;
    ++report.extra;
    report.indicators.push_back(CompatibilityIndicator{
        .code = CompatibilityCode::MissingWavePath,
        .severity = CompatibilitySeverity::Error,
        .contract_path = contract_path.runtime_path,
        .wave_path = {},
        .message = "contract binding id '" + contract_binding_id +
                   "' missing runtime in wave",
    });
  }

  if (wave_binding_paths.empty() || contract_binding_paths.empty()) {
    for (const auto &wave_path : wave_wikimyei_paths) {
      if (!has_contract_match(contract_wikimyei_paths, wave_path)) {
        report.ok = false;
        ++report.missing;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::MissingContractPath,
            .severity = CompatibilitySeverity::Error,
            .contract_path = {},
            .wave_path = wave_path.runtime_path,
            .message = "wave wikimyei path not present in contract",
        });
      }
    }
    for (const auto &wave_path : wave_source_paths) {
      if (!has_contract_match(contract_source_paths, wave_path)) {
        report.ok = false;
        ++report.missing;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::MissingContractPath,
            .severity = CompatibilitySeverity::Error,
            .contract_path = {},
            .wave_path = wave_path.runtime_path,
            .message = "wave source path not present in contract",
        });
      }
    }
    for (const auto &wave_path : wave_probe_paths) {
      if (!has_contract_match(contract_probe_paths, wave_path)) {
        report.ok = false;
        ++report.missing;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::MissingContractPath,
            .severity = CompatibilitySeverity::Error,
            .contract_path = {},
            .wave_path = wave_path.runtime_path,
            .message = "wave probe path not present in contract",
        });
      }
    }
    for (const auto &wave_path : wave_sink_paths) {
      if (!has_contract_match(contract_sink_paths, wave_path)) {
        report.ok = false;
        ++report.missing;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::MissingContractPath,
            .severity = CompatibilitySeverity::Error,
            .contract_path = {},
            .wave_path = wave_path.runtime_path,
            .message = "wave sink path not present in contract",
        });
      }
    }

    for (const auto &contract_path : contract_wikimyei_paths) {
      if (!has_wave_match(wave_wikimyei_paths, contract_path)) {
        report.ok = false;
        ++report.extra;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::MissingWavePath,
            .severity = CompatibilitySeverity::Error,
            .contract_path = contract_path.runtime_path,
            .wave_path = {},
            .message = "contract wikimyei path missing in wave declaration",
        });
      }
    }
    for (const auto &contract_path : contract_source_paths) {
      if (!has_wave_match(wave_source_paths, contract_path)) {
        report.ok = false;
        ++report.extra;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::MissingWavePath,
            .severity = CompatibilitySeverity::Error,
            .contract_path = contract_path.runtime_path,
            .wave_path = {},
            .message = "contract source path missing in wave declaration",
        });
      }
    }
    for (const auto &contract_path : contract_probe_paths) {
      if (!has_wave_match(wave_probe_paths, contract_path)) {
        report.ok = false;
        ++report.extra;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::MissingWavePath,
            .severity = CompatibilitySeverity::Error,
            .contract_path = contract_path.runtime_path,
            .wave_path = {},
            .message = "contract probe path missing in wave declaration",
        });
      }
    }
    for (const auto &contract_path : contract_sink_paths) {
      if (!has_wave_match(wave_sink_paths, contract_path)) {
        report.ok = false;
        ++report.extra;
        report.indicators.push_back(CompatibilityIndicator{
            .code = CompatibilityCode::MissingWavePath,
            .severity = CompatibilitySeverity::Error,
            .contract_path = contract_path.runtime_path,
            .wave_path = {},
            .message = "contract sink path missing in wave declaration",
        });
      }
    }
  }

  return report;
}

} // namespace tsiemene
