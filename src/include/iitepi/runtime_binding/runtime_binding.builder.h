// ./include/iitepi/runtime_binding/runtime_binding.builder.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
#include <limits>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <torch/torch.h>

#include "camahjucunu/dsl/canonical_path/canonical_path.h"
#include "camahjucunu/dsl/iitepi_wave/iitepi_wave.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"

#include "hero/hashimyei_hero/hashimyei_identity.h"
#include "iitepi/observation_dsl_paths.h"
#include "iitepi/runtime_binding/runtime_binding.h"
#include "iitepi/runtime_binding/runtime_binding.validation.h"
#include "jkimyei/training_setup/jk_setup.h"
#include "piaabo/dconfig.h"
#include "piaabo/dfiles.h"
#include "tsiemene/tsi.sink.log.sys.h"
#include "tsiemene/tsi.sink.null.h"
#include "tsiemene/tsi.source.dataloader.h"
#include "tsiemene/tsi.type.registry.h"
#include "tsiemene/tsi.wikimyei.inference.mdn.h"
#include "tsiemene/tsi.wikimyei.representation.vicreg.h"

namespace tsiemene {
namespace runtime_binding_builder {

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
using DataloaderT = TsiSourceDataloader<Datatype_t, Sampler_t>;

[[nodiscard]] inline bool is_blank_ascii(std::string_view text) {
  for (char ch : text) {
    if (!std::isspace(static_cast<unsigned char>(ch)))
      return false;
  }
  return true;
}

[[nodiscard]] inline bool load_required_dsl_text(std::string_view key,
                                                 std::string text,
                                                 std::string *out,
                                                 std::string *error) {
  if (!out)
    return false;
  if (is_blank_ascii(text)) {
    if (error)
      *error = "missing required DSL text for key: " + std::string(key);
    return false;
  }
  *out = std::move(text);
  return true;
}

[[nodiscard]] inline std::string trim_ascii_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b])))
    ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1])))
    --e;
  return s.substr(b, e - b);
}

[[nodiscard]] inline std::string unquote_if_wrapped(std::string s) {
  s = trim_ascii_copy(std::move(s));
  if (s.size() >= 2) {
    const char a = s.front();
    const char b = s.back();
    if ((a == '\'' && b == '\'') || (a == '"' && b == '"')) {
      return s.substr(1, s.size() - 2);
    }
  }
  return s;
}

[[nodiscard]] inline std::string lower_ascii_copy(std::string s) {
  for (char &ch : s) {
    if (ch >= 'A' && ch <= 'Z')
      ch = static_cast<char>(ch - 'A' + 'a');
  }
  return s;
}

[[nodiscard]] inline std::string global_config_base_folder() {
  return std::filesystem::path(
             cuwacunu::iitepi::config_space_t::config_file_path)
      .parent_path()
      .string();
}

[[nodiscard]] inline std::optional<bool> parse_bool_ascii(std::string value) {
  value = lower_ascii_copy(trim_ascii_copy(std::move(value)));
  if (value == "1" || value == "true" || value == "yes" || value == "on")
    return true;
  if (value == "0" || value == "false" || value == "no" || value == "off")
    return false;
  return std::nullopt;
}

[[nodiscard]] inline std::string
render_instance_args_suffix(const std::vector<std::string> &args) {
  if (args.empty())
    return {};
  std::ostringstream oss;
  oss << "(";
  for (std::size_t i = 0; i < args.size(); ++i) {
    if (i > 0)
      oss << ", ";
    oss << args[i];
  }
  oss << ")";
  return oss.str();
}

[[nodiscard]] inline std::string resolve_path_from_folder(std::string folder,
                                                          std::string path) {
  folder = trim_ascii_copy(std::move(folder));
  path = trim_ascii_copy(std::move(path));
  if (path.empty())
    return {};
  const std::filesystem::path p(path);
  if (p.is_absolute())
    return p.string();
  if (folder.empty())
    return p.string();
  return (std::filesystem::path(folder) / p).string();
}

[[nodiscard]] inline bool load_wave_dataloader_observation_payloads(
    const std::shared_ptr<const cuwacunu::iitepi::contract_record_t>
        &contract_record,
    const std::shared_ptr<const cuwacunu::iitepi::wave_record_t> &wave_record,
    const cuwacunu::camahjucunu::iitepi_wave_t &selected_wave,
    std::string *out_sources_dsl, std::string *out_channels_dsl,
    cuwacunu::camahjucunu::observation_spec_t *out_observation,
    std::string *error = nullptr) {
  if (!wave_record) {
    if (error)
      *error = "missing wave record for observation payload resolution";
    return false;
  }

  cuwacunu::iitepi::observation_dsl_path_resolution_t observation_paths{};
  if (!cuwacunu::iitepi::resolve_observation_dsl_paths(
          contract_record, wave_record, selected_wave, &observation_paths,
          error)) {
    return false;
  }

  if (!std::filesystem::exists(observation_paths.sources_path) ||
      !std::filesystem::is_regular_file(observation_paths.sources_path)) {
    if (error) {
      *error = "wave '" + selected_wave.name +
               "' has invalid observation sources DSL path: " +
               observation_paths.sources_path;
    }
    return false;
  }
  if (!std::filesystem::exists(observation_paths.channels_path) ||
      !std::filesystem::is_regular_file(observation_paths.channels_path)) {
    if (error) {
      *error = "wave '" + selected_wave.name +
               "' has invalid observation channels DSL path: " +
               observation_paths.channels_path;
    }
    return false;
  }

  const std::string sources_dsl = cuwacunu::piaabo::dfiles::readFileToString(
      observation_paths.sources_path);
  const std::string channels_dsl = cuwacunu::piaabo::dfiles::readFileToString(
      observation_paths.channels_path);
  if (is_blank_ascii(sources_dsl) || is_blank_ascii(channels_dsl)) {
    if (error) {
      *error = "wave '" + selected_wave.name +
               "' dataloader observation DSL file is empty";
    }
    return false;
  }

  if (out_sources_dsl)
    *out_sources_dsl = sources_dsl;
  if (out_channels_dsl)
    *out_channels_dsl = channels_dsl;

  if (out_observation) {
    const std::string sources_grammar_path = resolve_path_from_folder(
        global_config_base_folder(),
        cuwacunu::iitepi::config_space_t::get<std::string>(
            "BNF", "observation_sources_grammar_filename"));
    const std::string channels_grammar_path = resolve_path_from_folder(
        global_config_base_folder(),
        cuwacunu::iitepi::config_space_t::get<std::string>(
            "BNF", "observation_channels_grammar_filename"));
    const std::string sources_grammar =
        cuwacunu::piaabo::dfiles::readFileToString(sources_grammar_path);
    const std::string channels_grammar =
        cuwacunu::piaabo::dfiles::readFileToString(channels_grammar_path);
    if (is_blank_ascii(sources_grammar) || is_blank_ascii(channels_grammar)) {
      if (error) {
        *error = "observation grammar payload is empty";
      }
      return false;
    }
    try {
      *out_observation =
          cuwacunu::camahjucunu::decode_observation_spec_from_split_dsl(
              sources_grammar, sources_dsl, channels_grammar, channels_dsl);
    } catch (const std::exception &e) {
      if (error) {
        *error = "failed to decode wave-selected observation DSL: " +
                 std::string(e.what());
      }
      return false;
    }
  }

  return true;
}

[[nodiscard]] inline bool load_contract_jkimyei_payloads(
    const std::shared_ptr<const cuwacunu::iitepi::contract_record_t>
        &contract_record,
    std::string *out_jkimyei_dsl,
    cuwacunu::camahjucunu::jkimyei_specs_t *out_jkimyei,
    std::string *error = nullptr) {
  if (!contract_record || !out_jkimyei_dsl || !out_jkimyei) {
    if (error)
      *error = "missing output(s) for jkimyei payload resolution";
    return false;
  }
  const auto sec_it = contract_record->module_sections.find("VICReg");
  if (sec_it == contract_record->module_sections.end()) {
    if (error)
      *error = "missing VICReg module section for jkimyei payload";
    return false;
  }
  const auto key_it = sec_it->second.find("jkimyei_dsl_file");
  if (key_it == sec_it->second.end()) {
    if (error) {
      *error = "missing component-local jkimyei payload in VICReg module "
               "(expected key jkimyei_dsl_file)";
    }
    return false;
  }

  std::string module_path{};
  const auto module_it = contract_record->module_section_paths.find("VICReg");
  if (module_it != contract_record->module_section_paths.end()) {
    module_path = trim_ascii_copy(module_it->second);
  }
  std::string module_folder{};
  if (!module_path.empty()) {
    const std::filesystem::path p(module_path);
    if (p.has_parent_path())
      module_folder = p.parent_path().string();
  }
  const std::string dsl_path = resolve_path_from_folder(
      module_folder, unquote_if_wrapped(key_it->second));
  if (dsl_path.empty() || !std::filesystem::exists(dsl_path) ||
      !std::filesystem::is_regular_file(dsl_path)) {
    if (error) {
      *error = "invalid VICReg.jkimyei_dsl_file path: " + dsl_path;
    }
    return false;
  }

  const std::string jkimyei_dsl =
      cuwacunu::piaabo::dfiles::readFileToString(dsl_path);
  if (is_blank_ascii(jkimyei_dsl)) {
    if (error)
      *error = "empty jkimyei DSL payload: " + dsl_path;
    return false;
  }

  const std::string grammar_path = resolve_path_from_folder(
      global_config_base_folder(),
      cuwacunu::iitepi::config_space_t::get<std::string>(
          "BNF", "jkimyei_specs_grammar_filename"));
  if (grammar_path.empty() || !std::filesystem::exists(grammar_path) ||
      !std::filesystem::is_regular_file(grammar_path)) {
    if (error) {
      *error =
          "invalid [BNF].jkimyei_specs_grammar_filename path: " + grammar_path;
    }
    return false;
  }
  const std::string grammar_text =
      cuwacunu::piaabo::dfiles::readFileToString(grammar_path);
  if (is_blank_ascii(grammar_text)) {
    if (error)
      *error = "empty jkimyei grammar payload";
    return false;
  }

  try {
    *out_jkimyei = cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(
        grammar_text, jkimyei_dsl, dsl_path);
    *out_jkimyei_dsl = jkimyei_dsl;
    return true;
  } catch (const std::exception &e) {
    if (error) {
      *error = std::string("failed to decode component-local jkimyei DSL: ") +
               e.what();
    }
    return false;
  }
}

[[nodiscard]] inline const cuwacunu::camahjucunu::jkimyei_specs_t::row_t *
find_jkimyei_row_by_id(const cuwacunu::camahjucunu::jkimyei_specs_t &specs,
                       std::string_view table_name, std::string_view row_id) {
  const auto table_it = specs.tables.find(std::string(table_name));
  if (table_it == specs.tables.end())
    return nullptr;
  for (const auto &row : table_it->second) {
    const auto rid_it = row.find(ROW_ID_COLUMN_HEADER);
    if (rid_it == row.end())
      continue;
    if (trim_ascii_copy(rid_it->second) == row_id)
      return &row;
  }
  return nullptr;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::jkimyei_specs_t::row_t *
find_jkimyei_component_profile_row(
    const cuwacunu::camahjucunu::jkimyei_specs_t &specs,
    std::string_view component_id, std::string_view profile_id) {
  const auto table_it = specs.tables.find("component_profiles_table");
  if (table_it == specs.tables.end())
    return nullptr;
  const std::string target_component =
      trim_ascii_copy(std::string(component_id));
  const std::string target_profile = trim_ascii_copy(std::string(profile_id));
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

[[nodiscard]] inline std::string resolve_wikimyei_component_lookup_name(
    TsiTypeId wikimyei_type_id, std::string_view wikimyei_hashimyei,
    const cuwacunu::camahjucunu::jkimyei_specs_t &jkimyei_specs,
    std::string_view fallback_base) {
  const std::string canonical_type =
      std::string(tsi_type_token(wikimyei_type_id));
  auto resolve_component_base = [&]() -> std::string {
    const auto table_it = jkimyei_specs.tables.find("components_table");
    if (table_it == jkimyei_specs.tables.end())
      return std::string(fallback_base);
    for (const auto &row : table_it->second) {
      const auto ctype_it = row.find("component_type");
      const auto rid_it = row.find(ROW_ID_COLUMN_HEADER);
      if (ctype_it == row.end() || rid_it == row.end())
        continue;
      if (trim_ascii_copy(ctype_it->second) != canonical_type)
        continue;
      const std::string resolved_id = trim_ascii_copy(rid_it->second);
      if (!resolved_id.empty())
        return resolved_id;
    }
    return std::string(fallback_base);
  };

  const std::string base = resolve_component_base();
  const std::string normalized_hashimyei =
      trim_ascii_copy(std::string(wikimyei_hashimyei));
  if (normalized_hashimyei.empty())
    return base;

  const std::string dot_name = base + "." + normalized_hashimyei;
  if (find_jkimyei_row_by_id(jkimyei_specs, "components_table", dot_name)) {
    return dot_name;
  }

  const std::string underscore_name = base + "_" + normalized_hashimyei;
  if (find_jkimyei_row_by_id(jkimyei_specs, "components_table",
                             underscore_name)) {
    return underscore_name;
  }

  return base;
}

[[nodiscard]] inline std::string resolve_vicreg_component_lookup_name(
    std::string_view vicreg_hashimyei,
    const cuwacunu::camahjucunu::jkimyei_specs_t &jkimyei_specs) {
  return resolve_wikimyei_component_lookup_name(
      TsiTypeId::WikimyeiRepresentationVicreg, vicreg_hashimyei, jkimyei_specs,
      "tsi.wikimyei.representation.vicreg");
}

[[nodiscard]] inline std::string resolve_expected_value_component_lookup_name(
    std::string_view expected_value_hashimyei,
    const cuwacunu::camahjucunu::jkimyei_specs_t &jkimyei_specs) {
  return resolve_wikimyei_component_lookup_name(
      TsiTypeId::WikimyeiInferenceMdn, expected_value_hashimyei, jkimyei_specs,
      "tsi.wikimyei.inference.mdn");
}

inline void apply_vicreg_flag_overrides_from_component_row(
    RuntimeBindingContract::Spec *spec,
    const cuwacunu::camahjucunu::jkimyei_specs_t::row_t *row) {
  if (!spec || !row)
    return;

  const auto assign_if_present = [&](const char *key, bool *out_value) {
    const auto it = row->find(key);
    if (it == row->end() || !out_value)
      return;
    const auto parsed = parse_bool_ascii(it->second);
    if (parsed.has_value())
      *out_value = *parsed;
  };

  assign_if_present("vicreg_use_swa", &spec->vicreg_use_swa);
  assign_if_present("vicreg_detach_to_cpu", &spec->vicreg_detach_to_cpu);
}

[[nodiscard]] inline std::string
compose_wikimyei_runtime_component_name(std::string_view lookup_component_name,
                                        std::string_view circuit_name,
                                        std::string_view alias) {
  std::string out(lookup_component_name);
  out.append("@");
  out.append(circuit_name);
  out.append(".");
  out.append(alias);
  return out;
}

[[nodiscard]] inline std::optional<node_path_ref_t> resolve_runtime_node_path(
    const std::string &raw_path, const std::string &contract_hash,
    bool allow_family_without_hash, std::string *error = nullptr);

[[nodiscard]] inline bool
canonical_tsi_type_for_contract(const std::string &contract_hash,
                                const std::string &raw_tsi_type,
                                std::string *out, std::string *error) {
  if (!out)
    return false;

  const auto type_path = resolve_runtime_node_path(
      raw_tsi_type, contract_hash, /*allow_family_without_hash=*/true, error);
  if (!type_path.has_value()) {
    if (error && error->empty())
      *error = "invalid tsi_type canonical path";
    return false;
  }

  const auto type_id = std::optional<TsiTypeId>(type_path->type_id);
  if (!type_id.has_value()) {
    if (error)
      *error = "unsupported tsi_type: " + type_path->canonical_identity;
    return false;
  }

  const auto *type_desc = find_tsi_type(*type_id);
  if (!type_desc) {
    if (error) {
      *error = "missing tsi type descriptor in manifest for: " +
               type_path->canonical_identity;
    }
    return false;
  }

  *out = std::string(type_desc->canonical);
  if (!type_path->hashimyei.empty()) {
    out->append(".");
    out->append(type_path->hashimyei);
  }
  return true;
}

[[nodiscard]] inline bool render_contract_circuit_dsl(
    const std::string &contract_hash,
    const cuwacunu::camahjucunu::tsiemene_circuit_decl_t &parsed,
    std::string *out, std::string *error) {
  if (!out)
    return false;

  std::ostringstream oss;
  oss << parsed.name << " = {\n";
  for (const auto &decl : parsed.instances) {
    std::string canonical_tsi_type;
    std::string local_error;
    if (!canonical_tsi_type_for_contract(contract_hash, decl.tsi_type,
                                         &canonical_tsi_type, &local_error)) {
      if (error) {
        *error = "unable to canonicalize tsi_type for alias " + decl.alias +
                 ": " + local_error;
      }
      return false;
    }
    oss << "  " << decl.alias << " = " << canonical_tsi_type
        << render_instance_args_suffix(decl.args) << "\n";
  }
  for (const auto &h : parsed.hops) {
    if (h.to.directive.empty()) {
      if (error) {
        *error = "hop target directive is empty while rendering canonical "
                 "circuit DSL: " +
                 h.from.instance + " -> " + h.to.instance;
      }
      return false;
    }
    oss << "  " << h.from.instance << "@" << h.from.directive << ":"
        << h.from.kind << " -> " << h.to.instance << "@" << h.to.directive
        << "\n";
  }
  oss << "}\n";
  *out = oss.str();
  return true;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::iitepi_wave_t *
select_wave_by_id(const cuwacunu::camahjucunu::iitepi_wave_set_t &instruction,
                  std::string wave_id, std::string *error = nullptr) {
  wave_id = trim_ascii_copy(std::move(wave_id));
  if (wave_id.empty()) {
    if (error)
      *error = "missing required wave id";
    return nullptr;
  }
  const cuwacunu::camahjucunu::iitepi_wave_t *chosen = nullptr;
  for (const auto &wave : instruction.waves) {
    if (trim_ascii_copy(wave.name) == wave_id) {
      if (chosen) {
        if (error) {
          *error = "ambiguous wave selection: wave id '" + wave_id +
                   "' matches multiple WAVE blocks";
        }
        return nullptr;
      }
      chosen = &wave;
    }
  }
  if (chosen)
    return chosen;
  if (error) {
    *error = "no WAVE matches requested wave id '" + wave_id + "'";
  }
  return nullptr;
}

[[nodiscard]] inline bool select_contract_circuit(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t &instruction,
    std::string_view wave_circuit_name,
    cuwacunu::camahjucunu::tsiemene_circuit_instruction_t *out,
    std::string *error = nullptr) {
  return select_effective_contract_circuit_instruction(
      instruction, wave_circuit_name, out, error);
}

[[nodiscard]] inline std::optional<node_path_ref_t>
resolve_runtime_node_path(const std::string &raw_path,
                          const std::string &contract_hash,
                          bool allow_family_without_hash, std::string *error) {
  return resolve_node_path_or_nullopt(raw_path, contract_hash,
                                      allow_family_without_hash, error);
}

[[nodiscard]] inline std::string
canonical_runtime_node_path(const std::string &raw_path,
                            const std::string &contract_hash,
                            std::string *error = nullptr) {
  const auto resolved = resolve_runtime_node_path(
      raw_path, contract_hash, /*allow_family_without_hash=*/false, error);
  if (!resolved.has_value())
    return {};
  return resolved->runtime_path;
}

[[nodiscard]] inline bool
wave_decl_matches_contract_path(const node_path_ref_t &contract_path,
                                const node_path_ref_t &wave_path) {
  return contract_wave_path_match(contract_path, wave_path);
}

[[nodiscard]] inline const cuwacunu::camahjucunu::iitepi_wave_wikimyei_decl_t *
find_wave_wikimyei_decl_for_binding_id_or_path(
    const cuwacunu::camahjucunu::iitepi_wave_t &wave,
    const node_path_ref_t &contract_path, std::string_view contract_binding_id,
    const std::string &contract_hash) {
  const std::string binding_id =
      trim_ascii_copy(std::string(contract_binding_id));
  if (!binding_id.empty()) {
    for (const auto &w : wave.wikimyeis) {
      if (trim_ascii_copy(w.binding_id) != binding_id)
        continue;
      const auto wave_path = resolve_runtime_node_path(
          w.wikimyei_path, contract_hash, /*allow_family_without_hash=*/false,
          nullptr);
      if (!wave_path.has_value())
        continue;
      if (wave_decl_matches_contract_path(contract_path, *wave_path))
        return &w;
    }
  }
  for (const auto &w : wave.wikimyeis) {
    const auto wave_path =
        resolve_runtime_node_path(w.wikimyei_path, contract_hash,
                                  /*allow_family_without_hash=*/false, nullptr);
    if (!wave_path.has_value())
      continue;
    if (wave_decl_matches_contract_path(contract_path, *wave_path))
      return &w;
  }
  return nullptr;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::iitepi_wave_source_decl_t *
find_wave_source_decl_for_binding_id_or_path(
    const cuwacunu::camahjucunu::iitepi_wave_t &wave,
    const node_path_ref_t &contract_path, std::string_view contract_binding_id,
    const std::string &contract_hash) {
  const std::string binding_id =
      trim_ascii_copy(std::string(contract_binding_id));
  if (!binding_id.empty()) {
    for (const auto &s : wave.sources) {
      if (trim_ascii_copy(s.binding_id) != binding_id)
        continue;
      const auto wave_path = resolve_runtime_node_path(
          s.source_path, contract_hash, /*allow_family_without_hash=*/false,
          nullptr);
      if (!wave_path.has_value())
        continue;
      if (wave_decl_matches_contract_path(contract_path, *wave_path))
        return &s;
    }
  }
  for (const auto &s : wave.sources) {
    const auto wave_path =
        resolve_runtime_node_path(s.source_path, contract_hash,
                                  /*allow_family_without_hash=*/false, nullptr);
    if (!wave_path.has_value())
      continue;
    if (wave_decl_matches_contract_path(contract_path, *wave_path))
      return &s;
  }
  return nullptr;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::iitepi_wave_probe_decl_t *
find_wave_probe_decl_for_binding_id_or_path(
    const cuwacunu::camahjucunu::iitepi_wave_t &wave,
    const node_path_ref_t &contract_path, std::string_view contract_binding_id,
    const std::string &contract_hash) {
  const std::string binding_id =
      trim_ascii_copy(std::string(contract_binding_id));
  if (!binding_id.empty()) {
    for (const auto &p : wave.probes) {
      if (trim_ascii_copy(p.binding_id) != binding_id)
        continue;
      const auto wave_path = resolve_runtime_node_path(
          p.probe_path, contract_hash, /*allow_family_without_hash=*/false,
          nullptr);
      if (!wave_path.has_value())
        continue;
      if (wave_decl_matches_contract_path(contract_path, *wave_path))
        return &p;
    }
  }
  for (const auto &p : wave.probes) {
    const auto wave_path =
        resolve_runtime_node_path(p.probe_path, contract_hash,
                                  /*allow_family_without_hash=*/false, nullptr);
    if (!wave_path.has_value())
      continue;
    if (wave_decl_matches_contract_path(contract_path, *wave_path))
      return &p;
  }
  return nullptr;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::iitepi_wave_sink_decl_t *
find_wave_sink_decl_for_binding_id_or_path(
    const cuwacunu::camahjucunu::iitepi_wave_t &wave,
    const node_path_ref_t &contract_path, std::string_view contract_binding_id,
    const std::string &contract_hash) {
  const std::string binding_id =
      trim_ascii_copy(std::string(contract_binding_id));
  if (!binding_id.empty()) {
    for (const auto &s : wave.sinks) {
      if (trim_ascii_copy(s.binding_id) != binding_id)
        continue;
      const auto wave_path = resolve_runtime_node_path(
          s.sink_path, contract_hash, /*allow_family_without_hash=*/false,
          nullptr);
      if (!wave_path.has_value())
        continue;
      if (wave_decl_matches_contract_path(contract_path, *wave_path))
        return &s;
    }
  }
  for (const auto &s : wave.sinks) {
    const auto wave_path =
        resolve_runtime_node_path(s.sink_path, contract_hash,
                                  /*allow_family_without_hash=*/false, nullptr);
    if (!wave_path.has_value())
      continue;
    if (wave_decl_matches_contract_path(contract_path, *wave_path))
      return &s;
  }
  return nullptr;
}

[[nodiscard]] inline std::string compose_source_range_command(
    const cuwacunu::camahjucunu::iitepi_wave_source_decl_t &source) {
  return source.symbol + "[" + source.from + "," + source.to + "]";
}

[[nodiscard]] inline std::string compose_invoke_payload_from_wave_source(
    const cuwacunu::camahjucunu::iitepi_wave_source_decl_t &source,
    const cuwacunu::camahjucunu::iitepi_wave_t &wave) {
  const std::string source_command = compose_source_range_command(source);
  std::string payload = "wave@symbol:" + source.symbol +
                        ",epochs:" + std::to_string(wave.epochs) +
                        ",episode:0,batch:0,i:0,from:" + source.from +
                        ",to:" + source.to;
  if (wave.max_batches_per_epoch > 0) {
    payload += ",max_batches:" + std::to_string(wave.max_batches_per_epoch);
  }
  payload += "@" + source_command;
  return payload;
}

template <typename Datatype_t>
[[nodiscard]] inline std::string contract_sample_type_name() {
  const char *record =
      cuwacunu::camahjucunu::data::detail::record_type_name_for_datatype<
          Datatype_t>();
  if (!record)
    return {};
  return std::string(record);
}

[[nodiscard]] inline bool parse_instance_arg_key_value(std::string text,
                                                       std::string *out_key,
                                                       std::string *out_value) {
  if (!out_key || !out_value)
    return false;
  text = trim_ascii_copy(std::move(text));
  const std::size_t eq = text.find('=');
  if (eq == std::string::npos || eq == 0 || eq + 1 >= text.size())
    return false;
  *out_key = lower_ascii_copy(trim_ascii_copy(text.substr(0, eq)));
  *out_value = lower_ascii_copy(trim_ascii_copy(text.substr(eq + 1)));
  return !out_key->empty() && !out_value->empty();
}

[[nodiscard]] inline bool parse_probe_log_mode_from_instance_args(
    const cuwacunu::camahjucunu::tsiemene_instance_decl_t &decl,
    TsiSinkLogSys::LogMode *out_mode, std::string *error) {
  if (!out_mode)
    return false;
  *out_mode = TsiSinkLogSys::LogMode::EachBatch;
  if (decl.args.empty())
    return true;

  bool mode_seen = false;
  for (const auto &raw_arg : decl.args) {
    const std::string token = lower_ascii_copy(trim_ascii_copy(raw_arg));
    if (token.empty()) {
      if (error)
        *error = "empty instance argument for alias '" + decl.alias + "'";
      return false;
    }

    std::string key;
    std::string value;
    if (parse_instance_arg_key_value(token, &key, &value)) {
      if (key != "mode" && key != "log_mode" && key != "cadence") {
        if (error) {
          *error = "unsupported argument key '" + key +
                   "' for tsi.probe.log alias '" + decl.alias + "'";
        }
        return false;
      }
      const auto parsed_mode = TsiSinkLogSys::parse_mode_token(value);
      if (!parsed_mode.has_value()) {
        if (error) {
          *error = "invalid tsi.probe.log mode '" + value + "' for alias '" +
                   decl.alias + "' (expected batch|epoch)";
        }
        return false;
      }
      if (mode_seen) {
        if (error) {
          *error = "duplicated tsi.probe.log mode argument for alias '" +
                   decl.alias + "'";
        }
        return false;
      }
      *out_mode = *parsed_mode;
      mode_seen = true;
      continue;
    }

    const auto parsed_mode = TsiSinkLogSys::parse_mode_token(token);
    if (!parsed_mode.has_value()) {
      if (error) {
        *error = "invalid tsi.probe.log argument '" + token + "' for alias '" +
                 decl.alias +
                 "' (expected mode=batch|epoch or positional batch|epoch)";
      }
      return false;
    }
    if (mode_seen) {
      if (error) {
        *error = "duplicated tsi.probe.log mode argument for alias '" +
                 decl.alias + "'";
      }
      return false;
    }
    *out_mode = *parsed_mode;
    mode_seen = true;
  }

  return true;
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
std::unique_ptr<Tsi> make_tsi_for_decl(
    TsiId id, const cuwacunu::iitepi::contract_hash_t &contract_hash,
    TsiTypeId type_id,
    const cuwacunu::camahjucunu::tsiemene_instance_decl_t &decl,
    RuntimeBindingContract::Spec *spec,
    const cuwacunu::camahjucunu::observation_spec_t &observation_instruction,
    const cuwacunu::camahjucunu::jkimyei_specs_t &jkimyei_specs,
    std::string_view jkimyei_specs_dsl_text, std::string_view circuit_name,
    torch::Device device, std::size_t source_batch_size_override,
    std::uint64_t dataloader_workers, bool dataloader_force_rebuild_cache,
    std::uint64_t dataloader_range_warn_batches,
    const DataloaderT<Datatype_t, Sampler_t> *first_dataloader,
    std::string_view resolved_wikimyei_hashimyei,
    const cuwacunu::camahjucunu::iitepi_wave_wikimyei_decl_t
        *wave_wikimyei_decl,
    bool wave_mode_train_enabled, bool *made_dataloader,
    std::string *runtime_component_name_out, std::string *error) {
  if (made_dataloader)
    *made_dataloader = false;
  if (runtime_component_name_out)
    runtime_component_name_out->clear();
  if (!spec)
    return nullptr;

  switch (type_id) {
  case TsiTypeId::SourceDataloader:
    if (!decl.args.empty()) {
      if (error) {
        *error = "tsi.source.dataloader does not accept instance arguments for "
                 "alias '" +
                 decl.alias + "'";
      }
      return nullptr;
    }
    if (made_dataloader)
      *made_dataloader = true;
    return std::make_unique<DataloaderT<Datatype_t, Sampler_t>>(
        id, spec->instrument, observation_instruction, device,
        source_batch_size_override, dataloader_workers,
        dataloader_force_rebuild_cache, dataloader_range_warn_batches,
        contract_hash);
  case TsiTypeId::WikimyeiRepresentationVicreg: {
    if (!decl.args.empty()) {
      if (error) {
        *error = "tsi.wikimyei.representation.vicreg does not accept instance "
                 "arguments for alias '" +
                 decl.alias + "'";
      }
      return nullptr;
    }
    if (!first_dataloader)
      return nullptr;

    const std::string node_hashimyei =
        trim_ascii_copy(std::string(resolved_wikimyei_hashimyei)).empty()
            ? spec->representation_hashimyei
            : trim_ascii_copy(std::string(resolved_wikimyei_hashimyei));
    const std::string base_component_lookup_name =
        resolve_vicreg_component_lookup_name(node_hashimyei, jkimyei_specs);
    std::string lookup_component_name = base_component_lookup_name;
    const cuwacunu::camahjucunu::jkimyei_specs_t::row_t *selected_profile_row =
        nullptr;

    if (wave_wikimyei_decl) {
      spec->vicreg_train = wave_mode_train_enabled && wave_wikimyei_decl->train;
    }
    {
      std::string selected_profile_id =
          wave_wikimyei_decl ? trim_ascii_copy(wave_wikimyei_decl->profile_id)
                             : std::string{};
      const bool profile_selected_by_wave = !selected_profile_id.empty();
      if (selected_profile_id.empty()) {
        const std::vector<std::string> profile_ids =
            collect_component_profile_ids(jkimyei_specs,
                                          {base_component_lookup_name});
        if (profile_ids.size() == 1) {
          selected_profile_id = profile_ids.front();
        } else {
          if (error) {
            if (profile_ids.empty()) {
              *error = "no compatible jkimyei profile found for component '" +
                       base_component_lookup_name + "'";
            } else {
              std::ostringstream oss;
              for (std::size_t i = 0; i < profile_ids.size(); ++i) {
                if (i != 0)
                  oss << ", ";
                oss << profile_ids[i];
              }
              *error = "wave omitted PROFILE_ID for component '" +
                       base_component_lookup_name +
                       "' but multiple compatible profiles exist: [" +
                       oss.str() + "]";
            }
          }
          return nullptr;
        }
      }
      selected_profile_row = find_jkimyei_component_profile_row(
          jkimyei_specs, base_component_lookup_name, selected_profile_id);
      if (!selected_profile_row) {
        if (error) {
          *error = (profile_selected_by_wave ? "wave profile_id '"
                                             : "resolved profile_id '") +
                   selected_profile_id + "' not found for component '" +
                   base_component_lookup_name + "'";
        }
        return nullptr;
      }
      const auto row_id_it = selected_profile_row->find(ROW_ID_COLUMN_HEADER);
      if (row_id_it == selected_profile_row->end() ||
          trim_ascii_copy(row_id_it->second).empty()) {
        if (error) {
          *error = "selected jkimyei component profile row has empty row_id "
                   "for component '" +
                   base_component_lookup_name + "'";
        }
        return nullptr;
      }
      lookup_component_name = trim_ascii_copy(row_id_it->second);
    }

    const auto *component_row = find_jkimyei_row_by_id(
        jkimyei_specs, "components_table", base_component_lookup_name);
    if (selected_profile_row) {
      apply_vicreg_flag_overrides_from_component_row(spec,
                                                     selected_profile_row);
    } else {
      apply_vicreg_flag_overrides_from_component_row(spec, component_row);
    }
    if (wave_wikimyei_decl) {
      spec->vicreg_train = wave_mode_train_enabled && wave_wikimyei_decl->train;
    }

    const std::string runtime_component_name =
        compose_wikimyei_runtime_component_name(lookup_component_name,
                                                circuit_name, decl.alias);
    spec->representation_component_name = runtime_component_name;
    if (runtime_component_name_out) {
      *runtime_component_name_out = runtime_component_name;
    }
    if (!jkimyei_specs_dsl_text.empty()) {
      cuwacunu::jkimyei::jk_setup_t::registry
          .set_component_instruction_override(
              contract_hash, runtime_component_name, lookup_component_name,
              std::string(jkimyei_specs_dsl_text));
    }

    // Runtime ctor settings are derived from contract spec.
    const int C = (spec->channels > 0)
                      ? static_cast<int>(spec->channels)
                      : static_cast<int>(first_dataloader->C());
    const int T = (spec->timesteps > 0)
                      ? static_cast<int>(spec->timesteps)
                      : static_cast<int>(first_dataloader->T());
    const int D = (spec->features > 0)
                      ? static_cast<int>(spec->features)
                      : static_cast<int>(first_dataloader->D());
    return std::make_unique<TsiWikimyeiRepresentationVicreg>(
        id, decl.alias, contract_hash, node_hashimyei, runtime_component_name,
        C, T, D,
        /*train=*/spec->vicreg_train,
        /*use_swa=*/spec->vicreg_use_swa,
        /*detach_to_cpu=*/spec->vicreg_detach_to_cpu);
  }
  case TsiTypeId::WikimyeiInferenceMdn: {
    if (!decl.args.empty()) {
      if (error) {
        *error = "tsi.wikimyei.inference.mdn does not accept instance "
                 "arguments for alias '" +
                 decl.alias + "'";
      }
      return nullptr;
    }

    const std::string node_hashimyei =
        trim_ascii_copy(std::string(resolved_wikimyei_hashimyei));
    const std::string base_component_lookup_name =
        resolve_expected_value_component_lookup_name(node_hashimyei,
                                                     jkimyei_specs);
    std::string lookup_component_name = base_component_lookup_name;
    const cuwacunu::camahjucunu::jkimyei_specs_t::row_t *selected_profile_row =
        nullptr;
    bool train_enabled = wave_mode_train_enabled;
    if (wave_wikimyei_decl) {
      train_enabled = wave_mode_train_enabled && wave_wikimyei_decl->train;
    }

    {
      std::string selected_profile_id =
          wave_wikimyei_decl ? trim_ascii_copy(wave_wikimyei_decl->profile_id)
                             : std::string{};
      const bool profile_selected_by_wave = !selected_profile_id.empty();
      if (selected_profile_id.empty()) {
        const std::vector<std::string> profile_ids =
            collect_component_profile_ids(jkimyei_specs,
                                          {base_component_lookup_name});
        if (profile_ids.size() == 1) {
          selected_profile_id = profile_ids.front();
        } else {
          if (error) {
            if (profile_ids.empty()) {
              *error = "no compatible jkimyei profile found for component '" +
                       base_component_lookup_name + "'";
            } else {
              std::ostringstream oss;
              for (std::size_t i = 0; i < profile_ids.size(); ++i) {
                if (i != 0)
                  oss << ", ";
                oss << profile_ids[i];
              }
              *error = "wave omitted PROFILE_ID for component '" +
                       base_component_lookup_name +
                       "' but multiple compatible profiles exist: [" +
                       oss.str() + "]";
            }
          }
          return nullptr;
        }
      }
      selected_profile_row = find_jkimyei_component_profile_row(
          jkimyei_specs, base_component_lookup_name, selected_profile_id);
      if (!selected_profile_row) {
        if (error) {
          *error = (profile_selected_by_wave ? "wave profile_id '"
                                             : "resolved profile_id '") +
                   selected_profile_id + "' not found for component '" +
                   base_component_lookup_name + "'";
        }
        return nullptr;
      }
      const auto row_id_it = selected_profile_row->find(ROW_ID_COLUMN_HEADER);
      if (row_id_it == selected_profile_row->end() ||
          trim_ascii_copy(row_id_it->second).empty()) {
        if (error) {
          *error = "selected jkimyei component profile row has empty row_id "
                   "for component '" +
                   base_component_lookup_name + "'";
        }
        return nullptr;
      }
      lookup_component_name = trim_ascii_copy(row_id_it->second);
    }

    const std::string runtime_component_name =
        compose_wikimyei_runtime_component_name(lookup_component_name,
                                                circuit_name, decl.alias);
    if (runtime_component_name_out) {
      *runtime_component_name_out = runtime_component_name;
    }
    if (!jkimyei_specs_dsl_text.empty()) {
      cuwacunu::jkimyei::jk_setup_t::registry
          .set_component_instruction_override(
              contract_hash, runtime_component_name, lookup_component_name,
              std::string(jkimyei_specs_dsl_text));
    }

    return std::make_unique<TsiWikimyeiInferenceMdn>(
        id, decl.alias, contract_hash, node_hashimyei, runtime_component_name,
        train_enabled);
  }
  case TsiTypeId::SinkNull:
    if (!decl.args.empty()) {
      if (error) {
        *error =
            "tsi.sink.null does not accept instance arguments for alias '" +
            decl.alias + "'";
      }
      return nullptr;
    }
    return std::make_unique<TsiSinkNull>(id, decl.alias);
  case TsiTypeId::SinkLogSys: {
    TsiSinkLogSys::LogMode log_mode = TsiSinkLogSys::LogMode::EachBatch;
    if (!parse_probe_log_mode_from_instance_args(decl, &log_mode, error)) {
      return nullptr;
    }
    return std::make_unique<TsiSinkLogSys>(id, decl.alias, log_mode);
  }
  }
  return nullptr;
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
bool build_runtime_circuit_from_decl(
    const cuwacunu::camahjucunu::tsiemene_circuit_decl_t &parsed,
    const cuwacunu::iitepi::contract_hash_t &contract_hash,
    const cuwacunu::camahjucunu::observation_spec_t &observation_instruction,
    const cuwacunu::camahjucunu::jkimyei_specs_t &jkimyei_specs,
    std::string_view jkimyei_specs_dsl_text,
    const cuwacunu::camahjucunu::iitepi_wave_t *wave, torch::Device device,
    RuntimeBindingContract *out, std::string *error = nullptr) {
  if (!out)
    return false;

  {
    std::string semantic_error;
    if (!cuwacunu::camahjucunu::validate_circuit_decl(parsed,
                                                      &semantic_error)) {
      if (error)
        *error = semantic_error;
      return false;
    }
  }

  std::string effective_invoke_name = parsed.invoke_name;
  if (is_blank_ascii(effective_invoke_name))
    effective_invoke_name = parsed.name;
  std::string effective_invoke_payload = trim_ascii_copy(parsed.invoke_payload);

  if (is_blank_ascii(effective_invoke_name)) {
    if (error)
      *error = "empty circuit invoke name";
    return false;
  }

  out->name = parsed.name;
  out->invoke_name = effective_invoke_name;
  out->invoke_payload.clear();
  out->invoke_source_command.clear();
  out->nodes.clear();
  out->hops.clear();
  out->invalidate_compiled_runtime();
  out->spec = RuntimeBindingContract::Spec{};
  out->execution = RuntimeBindingContract::Execution{};
  out->spec.contract_hash = contract_hash;
  out->spec.sample_type = contract_sample_type_name<Datatype_t>();
  out->spec.sourced_from_config = true;
  {
    const int trace_level_cfg = cuwacunu::iitepi::config_space_t::get<int>(
        "TSI_RUNTIME", "runtime_trace_level",
        std::optional<int>{static_cast<int>(RuntimeTraceLevel::Verbose)});
    if (trace_level_cfg <= 0) {
      out->execution.runtime.trace_level = RuntimeTraceLevel::Off;
    } else if (trace_level_cfg == 1) {
      out->execution.runtime.trace_level = RuntimeTraceLevel::Step;
    } else {
      out->execution.runtime.trace_level = RuntimeTraceLevel::Verbose;
    }

    const int max_queue_cfg = cuwacunu::iitepi::config_space_t::get<int>(
        "TSI_RUNTIME", "runtime_max_queue_size", std::optional<int>{0});
    if (max_queue_cfg < 0) {
      if (error) {
        *error = "TSI_RUNTIME.runtime_max_queue_size must be >= 0";
      }
      return false;
    }
    out->execution.runtime.max_queue_size =
        static_cast<std::size_t>(max_queue_cfg);
  }
  bool wave_mode_train_enabled = false;
  if (wave) {
    std::uint64_t mode_flags = wave->mode_flags;
    if (mode_flags == 0) {
      std::string mode_error;
      if (!cuwacunu::camahjucunu::parse_iitepi_wave_mode_flags(
              wave->mode, &mode_flags, nullptr, &mode_error)) {
        if (error) {
          *error =
              "wave MODE parse failed for '" + wave->name + "': " + mode_error;
        }
        return false;
      }
    }
    out->execution.wave_mode_flags = mode_flags;
    out->execution.debug_enabled = cuwacunu::camahjucunu::iitepi_wave_mode_has(
        mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Debug);
    wave_mode_train_enabled = cuwacunu::camahjucunu::iitepi_wave_mode_has(
        mode_flags, cuwacunu::camahjucunu::iitepi_wave_mode_flag_e::Train);
    out->execution.epochs = wave->epochs;
    out->execution.batch_size = wave->batch_size;
  }
  if (wave && wave->batch_size > static_cast<std::uint64_t>(
                                     std::numeric_limits<std::size_t>::max())) {
    if (error) {
      *error = "wave BATCH_SIZE exceeds runtime supported range";
    }
    return false;
  }
  const std::size_t source_batch_size_override =
      (out->execution.batch_size > 0)
          ? static_cast<std::size_t>(out->execution.batch_size)
          : 0;
  std::uint64_t dataloader_workers = 0;
  bool dataloader_force_rebuild_cache = true;
  std::uint64_t dataloader_range_warn_batches = 256;

  std::unordered_map<std::string, Tsi *> alias_to_tsi;
  std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t> resolved_hops;
  DataloaderT<Datatype_t, Sampler_t> *first_dataloader = nullptr;
  std::uint64_t next_id = 1;
  std::size_t circuit_source_component_count = 0;
  std::unordered_map<std::string, node_path_ref_t> circuit_alias_refs{};
  std::vector<node_path_ref_t> circuit_wikimyei_refs{};
  std::vector<node_path_ref_t> circuit_source_refs{};
  std::vector<node_path_ref_t> circuit_probe_refs{};
  std::vector<node_path_ref_t> circuit_sink_refs{};
  const cuwacunu::camahjucunu::iitepi_wave_source_decl_t *selected_wave_source =
      nullptr;
  struct pending_decl_t {
    const cuwacunu::camahjucunu::tsiemene_instance_decl_t *decl{nullptr};
    TsiTypeId type_id{TsiTypeId::SourceDataloader};
    const TsiTypeDescriptor *type_desc{nullptr};
    std::string canonical_identity{};
    std::string decl_path{};
    std::string probe_hashimyei{};
    std::string resolved_wikimyei_hashimyei{};
    const cuwacunu::camahjucunu::iitepi_wave_wikimyei_decl_t
        *wave_wikimyei_decl{nullptr};
    const cuwacunu::camahjucunu::iitepi_wave_probe_decl_t *wave_probe_decl{
        nullptr};
  };
  std::vector<pending_decl_t> pending_decls;
  pending_decls.reserve(parsed.instances.size());

  for (const auto &decl : parsed.instances) {
    std::string type_path_error{};
    const auto type_path = resolve_runtime_node_path(
        decl.tsi_type, contract_hash, /*allow_family_without_hash=*/true,
        &type_path_error);
    if (!type_path.has_value()) {
      if (error) {
        *error = "invalid tsi_type canonical path for alias " + decl.alias +
                 ": " + type_path_error;
      }
      return false;
    }

    const auto type_id = std::optional<TsiTypeId>(type_path->type_id);
    const auto *type_desc = find_tsi_type(*type_id);
    if (!type_desc) {
      if (error) {
        *error = "missing tsi type descriptor in manifest for: " +
                 type_path->canonical_identity;
      }
      return false;
    }

    const std::string canonical_type(type_desc->canonical);
    if (std::find(out->spec.component_types.begin(),
                  out->spec.component_types.end(),
                  canonical_type) == out->spec.component_types.end()) {
      out->spec.component_types.push_back(canonical_type);
    }
    if (type_desc->domain == TsiDomain::Source &&
        out->spec.source_type.empty()) {
      out->spec.source_type = canonical_type;
    }
    if (type_desc->domain == TsiDomain::Wikimyei &&
        out->spec.representation_type.empty()) {
      out->spec.representation_type = canonical_type;
    }
    if (type_desc->domain == TsiDomain::Wikimyei &&
        out->spec.representation_hashimyei.empty() &&
        !type_path->hashimyei.empty()) {
      out->spec.representation_hashimyei = type_path->hashimyei;
    }
    const std::string decl_path = type_path->runtime_path;
    std::string resolved_wikimyei_hashimyei = type_path->hashimyei;
    const std::string decl_binding_id = trim_ascii_copy(decl.alias);
    if (decl_binding_id.empty()) {
      if (error) {
        *error = "empty circuit binding id is not allowed";
      }
      return false;
    }
    if (!circuit_alias_refs.emplace(decl_binding_id, *type_path).second) {
      if (error) {
        *error = "duplicated circuit binding id in runtime declaration: " +
                 decl_binding_id;
      }
      return false;
    }

    const cuwacunu::camahjucunu::iitepi_wave_wikimyei_decl_t
        *wave_wikimyei_decl = nullptr;
    const cuwacunu::camahjucunu::iitepi_wave_probe_decl_t *wave_probe_decl =
        nullptr;
    if (wave && type_desc->domain == TsiDomain::Wikimyei) {
      circuit_wikimyei_refs.push_back(*type_path);
      wave_wikimyei_decl = find_wave_wikimyei_decl_for_binding_id_or_path(
          *wave, *type_path, decl.alias, contract_hash);
      if (!wave_wikimyei_decl) {
        if (error) {
          *error = "missing WIKIMYEI wave block for binding id '" + decl.alias +
                   "' path '" + decl_path + "'";
        }
        return false;
      }
      std::string wave_path_error{};
      const auto wave_wikimyei_path = resolve_runtime_node_path(
          wave_wikimyei_decl->wikimyei_path, contract_hash,
          /*allow_family_without_hash=*/false, &wave_path_error);
      if (!wave_wikimyei_path.has_value()) {
        if (error) {
          *error = "invalid WIKIMYEI PATH for binding id '" + decl.alias +
                   "': " + wave_path_error;
        }
        return false;
      }
      if (!wave_wikimyei_path->hashimyei.empty()) {
        resolved_wikimyei_hashimyei = wave_wikimyei_path->hashimyei;
        if (out->spec.representation_hashimyei.empty()) {
          out->spec.representation_hashimyei = wave_wikimyei_path->hashimyei;
        }
      }
    }
    if (wave && type_desc->domain == TsiDomain::Source) {
      ++circuit_source_component_count;
      circuit_source_refs.push_back(*type_path);
      const auto *wave_source_decl =
          find_wave_source_decl_for_binding_id_or_path(
              *wave, *type_path, decl.alias, contract_hash);
      if (!wave_source_decl) {
        if (error) {
          *error = "missing SOURCE wave block for binding id '" + decl.alias +
                   "' path '" + decl_path + "'";
        }
        return false;
      }
      if (!selected_wave_source)
        selected_wave_source = wave_source_decl;
      // Source dataloader construction requires instrument up-front.
      if (out->spec.instrument.empty()) {
        out->spec.instrument = trim_ascii_copy(wave_source_decl->symbol);
      }
      if (out->spec.source_runtime_cursor.empty()) {
        out->spec.source_runtime_cursor =
            cuwacunu::source::dataloader::make_source_runtime_cursor(
                wave_source_decl->symbol, wave_source_decl->from,
                wave_source_decl->to);
      }
    }
    if (wave && type_desc->domain == TsiDomain::Probe) {
      circuit_probe_refs.push_back(*type_path);
      wave_probe_decl = find_wave_probe_decl_for_binding_id_or_path(
          *wave, *type_path, decl.alias, contract_hash);
      if (!wave_probe_decl) {
        if (error) {
          *error = "missing PROBE wave block for binding id '" + decl.alias +
                   "' path '" + decl_path + "'";
        }
        return false;
      }
    }
    if (wave && type_desc->domain == TsiDomain::Sink) {
      circuit_sink_refs.push_back(*type_path);
      const auto *wave_sink_decl = find_wave_sink_decl_for_binding_id_or_path(
          *wave, *type_path, decl.alias, contract_hash);
      if (!wave_sink_decl) {
        if (error) {
          *error = "missing SINK wave block for binding id '" + decl.alias +
                   "' path '" + decl_path + "'";
        }
        return false;
      }
    }

    std::string probe_hashimyei = type_path->hashimyei;
    if (type_desc->domain == TsiDomain::Probe) {
      if (probe_hashimyei.empty() && wave_probe_decl) {
        const auto probe_path = resolve_runtime_node_path(
            wave_probe_decl->probe_path, contract_hash,
            /*allow_family_without_hash=*/false, error);
        if (!probe_path.has_value())
          return false;
        probe_hashimyei = probe_path->hashimyei;
      }
      if (probe_hashimyei.empty()) {
        if (error) {
          *error = "probe tsi_type for alias '" + decl.alias +
                   "' requires hashimyei from wave PROBE.PATH";
        }
        return false;
      }
      if (!cuwacunu::hashimyei::is_hex_hash_name(probe_hashimyei)) {
        if (error) {
          *error = "probe hashimyei suffix is invalid for alias '" +
                   decl.alias + "': " + probe_hashimyei;
        }
        return false;
      }
    }

    pending_decls.push_back(pending_decl_t{
        .decl = &decl,
        .type_id = *type_id,
        .type_desc = type_desc,
        .canonical_identity = std::string(type_path->canonical_identity),
        .decl_path = decl_path,
        .probe_hashimyei = probe_hashimyei,
        .resolved_wikimyei_hashimyei = resolved_wikimyei_hashimyei,
        .wave_wikimyei_decl = wave_wikimyei_decl,
        .wave_probe_decl = wave_probe_decl,
    });
  }

  if (wave) {
    const auto has_matching_contract_ref =
        [&](const std::vector<node_path_ref_t> &contract_refs,
            const node_path_ref_t &wave_path) {
          return std::any_of(contract_refs.begin(), contract_refs.end(),
                             [&](const node_path_ref_t &contract_path) {
                               return wave_decl_matches_contract_path(
                                   contract_path, wave_path);
                             });
        };

    const auto validate_wave_binding_id_and_path =
        [&](const char *kind, const std::string &binding_id,
            const std::string &raw_wave_path,
            const std::vector<node_path_ref_t> &domain_contract_refs) -> bool {
      const std::string normalized_binding_id = trim_ascii_copy(binding_id);
      if (normalized_binding_id.empty()) {
        if (error) {
          *error = "wave '" + wave->name + "' " + kind +
                   " component is missing binding id (<...>)";
        }
        return false;
      }
      const auto contract_it = circuit_alias_refs.find(normalized_binding_id);
      if (contract_it == circuit_alias_refs.end()) {
        if (error) {
          *error = "wave '" + wave->name + "' " + kind + " binding id '" +
                   normalized_binding_id +
                   "' is not present in contract circuit acknowledgements";
        }
        return false;
      }
      const auto wave_path = resolve_runtime_node_path(
          raw_wave_path, contract_hash, /*allow_family_without_hash=*/false,
          nullptr);
      if (!wave_path.has_value()) {
        if (error) {
          *error = "wave '" + wave->name + "' has invalid " + kind +
                   " PATH for binding id '" + normalized_binding_id +
                   "': " + raw_wave_path;
        }
        return false;
      }
      if (!wave_decl_matches_contract_path(contract_it->second, *wave_path)) {
        if (error) {
          *error = "wave '" + wave->name + "' " + kind + " binding id '" +
                   normalized_binding_id + "' path mismatch: contract=" +
                   contract_it->second.runtime_path +
                   " wave=" + wave_path->runtime_path;
        }
        return false;
      }
      if (!has_matching_contract_ref(domain_contract_refs, *wave_path)) {
        if (error) {
          *error =
              "wave '" + wave->name + "' " + kind +
              " PATH not present in circuit domain: " + wave_path->runtime_path;
        }
        return false;
      }
      return true;
    };

    for (const auto &w : wave->wikimyeis) {
      if (!validate_wave_binding_id_and_path("WIKIMYEI", w.binding_id,
                                             w.wikimyei_path,
                                             circuit_wikimyei_refs)) {
        return false;
      }
    }
    for (const auto &s : wave->sources) {
      if (!validate_wave_binding_id_and_path(
              "SOURCE", s.binding_id, s.source_path, circuit_source_refs)) {
        return false;
      }
    }
    for (const auto &p : wave->probes) {
      if (!validate_wave_binding_id_and_path(
              "PROBE", p.binding_id, p.probe_path, circuit_probe_refs)) {
        return false;
      }
    }
    for (const auto &s : wave->sinks) {
      if (!validate_wave_binding_id_and_path("SINK", s.binding_id, s.sink_path,
                                             circuit_sink_refs)) {
        return false;
      }
    }

    if (circuit_source_component_count != 1) {
      if (error) {
        *error = "runtime requires exactly one SOURCE component in contract "
                 "circuit for wave binding";
      }
      return false;
    }
    if (wave->sources.size() != 1U) {
      if (error) {
        *error = "wave '" + wave->name +
                 "' must declare exactly one SOURCE block for contract binding";
      }
      return false;
    }
    if (!selected_wave_source) {
      if (error) {
        *error = "wave '" + wave->name +
                 "' missing SOURCE block for circuit source path";
      }
      return false;
    }
    dataloader_workers = selected_wave_source->workers;
    dataloader_force_rebuild_cache = selected_wave_source->force_rebuild_cache;
    dataloader_range_warn_batches = selected_wave_source->range_warn_batches;
    if (dataloader_range_warn_batches == 0) {
      if (error) {
        *error = "wave SOURCE.RANGE_WARN_BATCHES must be >= 1";
      }
      return false;
    }
    effective_invoke_payload =
        compose_invoke_payload_from_wave_source(*selected_wave_source, *wave);
  } else if (is_blank_ascii(effective_invoke_payload)) {
    if (error)
      *error = "empty circuit invoke payload";
    return false;
  }

  for (const auto &pending : pending_decls) {
    if (pending.type_desc->domain != TsiDomain::Wikimyei)
      continue;
    if (!validate_runtime_hashimyei_contract_docking(
            pending.type_desc->canonical, pending.resolved_wikimyei_hashimyei,
            contract_hash,
            /*require_registered_manifest=*/false, error)) {
      return false;
    }
  }

  out->invoke_payload = effective_invoke_payload;
  cuwacunu::camahjucunu::tsiemene_circuit_decl_t invoke_decl = parsed;
  invoke_decl.invoke_name = effective_invoke_name;
  invoke_decl.invoke_payload = effective_invoke_payload;

  cuwacunu::camahjucunu::iitepi_wave_invoke_t invoke{};
  if (!cuwacunu::camahjucunu::parse_circuit_invoke_wave(invoke_decl, &invoke,
                                                        error)) {
    return false;
  }
  out->invoke_source_command = invoke.source_command;
  out->spec.instrument = invoke.source_symbol;
  if (invoke.total_epochs > 0) {
    out->execution.epochs = invoke.total_epochs;
  }
  if (out->spec.instrument.empty()) {
    if (error) {
      *error = "empty instrument in invoke payload; use symbol in command or "
               "wave metadata key symbol: " +
               effective_invoke_payload;
    }
    return false;
  }

  std::unordered_map<std::string, std::unique_ptr<Tsi>> nodes_by_alias;
  nodes_by_alias.reserve(pending_decls.size());
  const auto emplace_node_for_decl =
      [&](const pending_decl_t &pending) -> bool {
    bool made_dataloader = false;
    std::string made_runtime_component_name{};
    std::unique_ptr<Tsi> node = make_tsi_for_decl<Datatype_t, Sampler_t>(
        next_id++, contract_hash, pending.type_id, *pending.decl, &out->spec,
        observation_instruction, jkimyei_specs, jkimyei_specs_dsl_text,
        parsed.name, device, source_batch_size_override, dataloader_workers,
        dataloader_force_rebuild_cache, dataloader_range_warn_batches,
        first_dataloader, pending.resolved_wikimyei_hashimyei,
        pending.wave_wikimyei_decl, wave_mode_train_enabled, &made_dataloader,
        &made_runtime_component_name, error);

    if (!node) {
      if (error) {
        if (!error->empty()) {
          /* preserve richer error from make_tsi_for_decl */
        } else if ((pending.type_id ==
                        TsiTypeId::WikimyeiRepresentationVicreg ||
                    pending.type_id == TsiTypeId::WikimyeiInferenceMdn) &&
                   !first_dataloader) {
          *error = "wikimyei requires a source dataloader in the same circuit";
        } else {
          *error = "unsupported tsi_type: " + pending.canonical_identity;
        }
      }
      return false;
    }

    auto [ins_it, inserted] =
        nodes_by_alias.emplace(pending.decl->alias, std::move(node));
    if (!inserted) {
      if (error)
        *error = "duplicated instance alias: " + pending.decl->alias;
      return false;
    }

    if (made_dataloader && !first_dataloader) {
      first_dataloader = dynamic_cast<DataloaderT<Datatype_t, Sampler_t> *>(
          ins_it->second.get());
      if (first_dataloader) {
        out->spec.channels = first_dataloader->C();
        out->spec.timesteps = first_dataloader->T();
        out->spec.features = first_dataloader->D();
        out->spec.batch_size_hint = first_dataloader->batch_size_hint();
        if (out->execution.batch_size == 0 && out->spec.batch_size_hint > 0) {
          out->execution.batch_size =
              static_cast<std::uint64_t>(out->spec.batch_size_hint);
        }
      }
    }
    if (pending.type_desc->domain == TsiDomain::Wikimyei) {
      RuntimeBindingContract::WikimyeiBindingIdentity binding{};
      binding.binding_id = trim_ascii_copy(pending.decl->alias);
      binding.canonical_type = std::string(pending.type_desc->canonical);
      binding.hashimyei = trim_ascii_copy(pending.resolved_wikimyei_hashimyei);
      binding.runtime_component_name = std::move(made_runtime_component_name);
      out->spec.wikimyei_bindings.push_back(std::move(binding));
    }
    return true;
  };

  // Build source dataloaders first to remove declaration-order coupling for
  // downstream wikimyei components that depend on discovered C/T/D hints.
  for (const auto &pending : pending_decls) {
    if (pending.type_desc->domain != TsiDomain::Source)
      continue;
    if (!emplace_node_for_decl(pending))
      return false;
  }
  for (const auto &pending : pending_decls) {
    if (pending.type_desc->domain == TsiDomain::Source)
      continue;
    if (!emplace_node_for_decl(pending))
      return false;
  }

  alias_to_tsi.clear();
  alias_to_tsi.reserve(pending_decls.size());
  for (const auto &decl : parsed.instances) {
    const auto node_it = nodes_by_alias.find(decl.alias);
    if (node_it == nodes_by_alias.end() || !node_it->second) {
      if (error) {
        *error = "internal runtime builder error: missing node for alias '" +
                 decl.alias + "'";
      }
      return false;
    }
    alias_to_tsi.emplace(decl.alias, node_it->second.get());
    out->nodes.push_back(std::move(node_it->second));
  }

  if (first_dataloader) {
    out->spec.channels = first_dataloader->C();
    out->spec.timesteps = first_dataloader->T();
    out->spec.features = first_dataloader->D();
    out->spec.batch_size_hint = first_dataloader->batch_size_hint();
    if (out->execution.batch_size == 0 && out->spec.batch_size_hint > 0) {
      out->execution.batch_size =
          static_cast<std::uint64_t>(out->spec.batch_size_hint);
    }
  }
  auto observation_instruction_copy = observation_instruction;
  out->spec.future_timesteps =
      observation_instruction_copy.max_future_sequence_length();

  if (!cuwacunu::camahjucunu::resolve_hops(parsed, &resolved_hops, error))
    return false;

  out->hops.reserve(resolved_hops.size());
  for (const auto &h : resolved_hops) {
    const auto it_from = alias_to_tsi.find(h.from.instance);
    const auto it_to = alias_to_tsi.find(h.to.instance);
    if (it_from == alias_to_tsi.end() || it_to == alias_to_tsi.end()) {
      if (error)
        *error = "hop references unknown instance alias: " + h.from.instance +
                 " -> " + h.to.instance;
      return false;
    }

    const auto *out_spec =
        find_directive(*it_from->second, h.from.directive, DirectiveDir::Out);
    const auto *in_spec =
        find_directive(*it_to->second, h.to.directive, DirectiveDir::In);
    if (!out_spec || !in_spec) {
      if (error) {
        *error =
            "hop directive not found on tsi declarations: " + h.from.instance +
            "@" + std::string(h.from.directive) + " -> " + h.to.instance + "@" +
            std::string(h.to.directive);
      }
      return false;
    }

    if (out_spec->kind.kind != h.from.kind) {
      if (error) {
        *error = "hop source kind mismatch against tsi declarations: " +
                 h.from.instance + "@" + std::string(h.from.directive);
      }
      return false;
    }
    if (!it_to->second->is_compatible(h.to.directive, out_spec->kind.kind)) {
      if (error) {
        *error = "hop target is not compatible with source kind: " +
                 h.from.instance + "@" + std::string(h.from.directive) +
                 " -> " + h.to.instance + "@" + std::string(h.to.directive);
      }
      return false;
    }
    // Do not enforce a single static input kind per directive here.
    // Some components (for example sink log) intentionally accept
    // multiple incoming payload kinds on the same directive.

    out->hops.push_back(hop(ep(*it_from->second, h.from.directive),
                            ep(*it_to->second, h.to.directive)));
  }

  out->seed_wave = normalize_wave_span(Wave{
      .cursor =
          WaveCursor{
              .id = 0,
              .i = invoke.wave_i,
              .episode = invoke.episode,
              .batch = invoke.batch,
          },
      .max_batches_per_epoch = invoke.max_batches_per_epoch,
      .span_begin_ms = invoke.span_begin_ms,
      .span_end_ms = invoke.span_end_ms,
      .has_time_span = invoke.has_time_span,
  });
  out->seed_ingress = Ingress{.directive = pick_start_directive(out->view()),
                              .signal = string_signal(invoke.source_command)};
  return true;
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
bool build_runtime_binding_from_instruction(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t &inst,
    torch::Device device,
    const cuwacunu::iitepi::contract_hash_t &contract_hash,
    const std::shared_ptr<const cuwacunu::iitepi::contract_record_t>
        &contract_record,
    const cuwacunu::iitepi::wave_hash_t &wave_hash,
    const std::shared_ptr<const cuwacunu::iitepi::wave_record_t> &wave_record,
    std::string wave_id, RuntimeBinding *out, std::string *error = nullptr) {
  if (!out)
    return false;
  if (!contract_record) {
    if (error)
      *error = "missing contract record";
    return false;
  }
  if (!wave_record) {
    if (error)
      *error = "missing wave record";
    return false;
  }
  (void)wave_hash;

  const auto contract_report =
      validate_contract_definition(inst, contract_hash);
  if (!contract_report.ok) {
    if (error) {
      *error = contract_report.indicators.empty()
                   ? "contract validation failed"
                   : contract_report.indicators.front().message;
    }
    return false;
  }

  std::string observation_sources_dsl;
  std::string observation_channels_dsl;
  std::string jkimyei_specs_dsl;
  std::string wave_dsl;
  if (!load_required_dsl_text(kRuntimeBindingContractWaveDslKey,
                              wave_record->wave.dsl, &wave_dsl, error)) {
    return false;
  }

  cuwacunu::camahjucunu::observation_spec_t observation_instruction{};
  cuwacunu::camahjucunu::jkimyei_specs_t jkimyei_specs{};
  cuwacunu::camahjucunu::iitepi_wave_set_t wave_set{};
  try {
    wave_set = wave_record->wave.decoded();
  } catch (const std::exception &e) {
    if (error)
      *error = std::string("failed to decode wave DSL payloads: ") + e.what();
    return false;
  }

  const auto *selected_wave =
      select_wave_by_id(wave_set, std::move(wave_id), error);
  if (!selected_wave)
    return false;
  const auto wave_report =
      validate_wave_definition(*selected_wave, contract_hash);
  if (!wave_report.ok) {
    if (error) {
      *error = wave_report.indicators.empty()
                   ? "wave validation failed"
                   : wave_report.indicators.front().message;
    }
    return false;
  }

  cuwacunu::camahjucunu::tsiemene_circuit_instruction_t effective_inst{};
  if (!select_contract_circuit(inst, selected_wave->circuit_name,
                               &effective_inst, error)) {
    return false;
  }

  out->contract_hash = contract_hash;
  out->wave_hash = wave_hash;
  out->contracts.clear();
  out->contracts.reserve(effective_inst.circuits.size());

  if (!load_contract_jkimyei_payloads(contract_record, &jkimyei_specs_dsl,
                                      &jkimyei_specs, error)) {
    return false;
  }
  if (!load_wave_dataloader_observation_payloads(
          contract_record, wave_record, *selected_wave,
          &observation_sources_dsl, &observation_channels_dsl,
          &observation_instruction, error)) {
    return false;
  }
  const auto compat_report = validate_wave_contract_compatibility(
      inst, *selected_wave, contract_hash, &jkimyei_specs, contract_hash,
      selected_wave->name);
  if (!compat_report.ok) {
    if (error) {
      *error = compat_report.indicators.empty()
                   ? "wave/contract compatibility validation failed"
                   : compat_report.indicators.front().message;
    }
    return false;
  }

  for (std::size_t i = 0; i < effective_inst.circuits.size(); ++i) {
    RuntimeBindingContract c{};
    std::string local_error;
    if (!build_runtime_circuit_from_decl<Datatype_t, Sampler_t>(
            effective_inst.circuits[i], contract_hash, observation_instruction,
            jkimyei_specs, jkimyei_specs_dsl, selected_wave, device, &c,
            &local_error)) {
      if (error) {
        *error = "contract[" + std::to_string(i) + "] " + local_error;
      }
      return false;
    }
    c.spec.component_dsl_fingerprints.clear();
    c.spec.component_dsl_fingerprints.reserve(
        contract_record->signature.bindings.size());
    for (const auto &binding : contract_record->signature.bindings) {
      RuntimeBindingContract::ComponentDslFingerprint fp{};
      fp.canonical_path = binding.canonical_path;
      fp.tsi_type = binding.tsi_type;
      fp.hashimyei = binding.hashimyei;
      fp.tsi_dsl_path = binding.tsi_dsl_path;
      fp.tsi_dsl_sha256_hex = binding.tsi_dsl_sha256_hex;
      c.spec.component_dsl_fingerprints.push_back(std::move(fp));
    }
    std::string circuit_dsl;
    if (!render_contract_circuit_dsl(contract_hash, effective_inst.circuits[i],
                                     &circuit_dsl, &local_error)) {
      if (error)
        *error = "contract[" + std::to_string(i) + "] " + local_error;
      return false;
    }
    if (is_blank_ascii(circuit_dsl)) {
      if (error) {
        *error = "contract[" + std::to_string(i) +
                 "] missing required DSL text for key: " +
                 std::string(kRuntimeBindingContractCircuitDslKey);
      }
      return false;
    }

    c.set_dsl_segment(kRuntimeBindingContractCircuitDslKey,
                      std::move(circuit_dsl));
    c.set_dsl_segment(kRuntimeBindingContractWaveDslKey, wave_dsl);

    std::string_view missing_key{};
    if (!c.has_required_dsl_segments(&missing_key)) {
      if (error) {
        *error =
            "contract[" + std::to_string(i) +
            "] missing required DSL text for key: " + std::string(missing_key);
      }
      return false;
    }

    c.seed_wave.cursor.id = static_cast<WaveId>(i);
    out->contracts.push_back(std::move(c));
  }
  return true;
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
bool build_runtime_binding_from_instruction(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t &inst,
    torch::Device device,
    const cuwacunu::iitepi::contract_hash_t &contract_hash,
    const cuwacunu::iitepi::wave_hash_t &wave_hash, std::string wave_id,
    RuntimeBinding *out, std::string *error = nullptr) {
  if (!out)
    return false;
  try {
    cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(
        contract_hash);
    cuwacunu::iitepi::wave_space_t::assert_intact_or_fail_fast(wave_hash);
    const auto contract_itself =
        cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
    const auto wave_itself =
        cuwacunu::iitepi::wave_space_t::wave_itself(wave_hash);
    return build_runtime_binding_from_instruction<Datatype_t, Sampler_t>(
        inst, device, contract_hash, contract_itself, wave_hash, wave_itself,
        std::move(wave_id), out, error);
  } catch (const std::exception &e) {
    if (error)
      *error = std::string("failed to load required DSL text from config: ") +
               e.what();
    return false;
  }
}

} // namespace runtime_binding_builder
} // namespace tsiemene
