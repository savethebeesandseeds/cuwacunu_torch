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
#include "iitepi/contract_space_t.h"
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
#include "tsiemene/tsi.wikimyei.inference.expected_value.mdn.h"
#include "tsiemene/tsi.wikimyei.representation.encoding.vicreg.h"

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

  out_jkimyei_dsl->clear();
  *out_jkimyei = cuwacunu::camahjucunu::jkimyei_specs_t{};
  std::unordered_set<std::string> loaded_paths{};
  std::vector<std::string> merged_paths{};

  const auto merge_specs =
      [](cuwacunu::camahjucunu::jkimyei_specs_t *dst,
         const cuwacunu::camahjucunu::jkimyei_specs_t &src) {
        for (const auto &[table_name, rows] : src.tables) {
          auto &dst_rows = dst->tables[table_name];
          dst_rows.insert(dst_rows.end(), rows.begin(), rows.end());
        }
        dst->raw.insert(dst->raw.end(), src.raw.begin(), src.raw.end());
      };

  for (const char *module_section : {"VICReg", "EXPECTED_VALUE"}) {
    const auto sec_it = contract_record->module_sections.find(module_section);
    if (sec_it == contract_record->module_sections.end())
      continue;
    const auto key_it = sec_it->second.find("jkimyei_dsl_file");
    if (key_it == sec_it->second.end()) {
      if (error) {
        *error = std::string("missing component-local jkimyei payload in ") +
                 module_section + " module (expected key jkimyei_dsl_file)";
      }
      return false;
    }

    std::string module_path{};
    const auto module_it =
        contract_record->module_section_paths.find(module_section);
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
        *error = std::string("invalid ") + module_section +
                 ".jkimyei_dsl_file path: " + dsl_path;
      }
      return false;
    }
    if (!loaded_paths.insert(dsl_path).second)
      continue;

    const std::string jkimyei_dsl =
        cuwacunu::piaabo::dfiles::readFileToString(dsl_path);
    if (is_blank_ascii(jkimyei_dsl)) {
      if (error)
        *error = "empty jkimyei DSL payload: " + dsl_path;
      return false;
    }

    try {
      const auto decoded =
          cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(
              grammar_text, jkimyei_dsl, dsl_path);
      merge_specs(out_jkimyei, decoded);
      if (!out_jkimyei_dsl->empty())
        *out_jkimyei_dsl += "\n";
      *out_jkimyei_dsl += "/* " + std::string(module_section) + ": " +
                          dsl_path + " */\n" + jkimyei_dsl;
      merged_paths.push_back(dsl_path);
    } catch (const std::exception &e) {
      if (error) {
        *error = "failed to decode component-local jkimyei DSL " + dsl_path +
                 ": " + e.what();
      }
      return false;
    }
  }

  if (merged_paths.empty()) {
    if (error)
      *error = "missing jkimyei payloads in contract module sections";
    return false;
  }
  std::ostringstream merged_label;
  for (std::size_t i = 0; i < merged_paths.size(); ++i) {
    if (i)
      merged_label << ";";
    merged_label << merged_paths[i];
  }
  out_jkimyei->instruction_filepath = merged_label.str();
  return true;
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

[[nodiscard]] inline bool resolve_wikimyei_component_lookup_name(
    TsiTypeId wikimyei_type_id, std::string_view wikimyei_hashimyei,
    const cuwacunu::camahjucunu::jkimyei_specs_t &jkimyei_specs,
    std::string *out, std::string *error) {
  if (!out)
    return false;
  out->clear();
  const std::string canonical_type =
      std::string(tsi_type_token(wikimyei_type_id));
  const auto table_it = jkimyei_specs.tables.find("components_table");
  if (table_it == jkimyei_specs.tables.end()) {
    if (error) {
      *error = "jkimyei specs missing components_table for component type '" +
               canonical_type + "'";
    }
    return false;
  }

  const std::string normalized_hashimyei =
      trim_ascii_copy(std::string(wikimyei_hashimyei));
  const std::string dot_name = canonical_type + "." + normalized_hashimyei;
  const std::string underscore_name =
      canonical_type + "_" + normalized_hashimyei;
  const std::string dot_suffix = "." + normalized_hashimyei;
  const std::string underscore_suffix = "_" + normalized_hashimyei;
  const auto ends_with = [](std::string_view text, std::string_view suffix) {
    return text.size() >= suffix.size() &&
           text.substr(text.size() - suffix.size()) == suffix;
  };
  std::vector<std::string> compatible_row_ids{};
  std::vector<std::string> exact_hashimyei_row_ids{};

  for (const auto &row : table_it->second) {
    const auto ctype_it = row.find("component_type");
    if (ctype_it == row.end() ||
        trim_ascii_copy(ctype_it->second) != canonical_type)
      continue;

    const auto rid_it = row.find(ROW_ID_COLUMN_HEADER);
    if (rid_it == row.end() || trim_ascii_copy(rid_it->second).empty()) {
      if (error) {
        *error = "jkimyei components_table row for component type '" +
                 canonical_type + "' has empty row_id";
      }
      return false;
    }

    const std::string row_id = trim_ascii_copy(rid_it->second);
    if (!normalized_hashimyei.empty() &&
        (row_id == dot_name || row_id == underscore_name ||
         ends_with(row_id, dot_suffix) ||
         ends_with(row_id, underscore_suffix))) {
      exact_hashimyei_row_ids.push_back(row_id);
    }
    compatible_row_ids.push_back(row_id);
  }

  if (exact_hashimyei_row_ids.size() == 1) {
    *out = exact_hashimyei_row_ids.front();
    return true;
  }
  if (exact_hashimyei_row_ids.size() > 1) {
    if (error) {
      std::ostringstream oss;
      for (std::size_t i = 0; i < exact_hashimyei_row_ids.size(); ++i) {
        if (i != 0)
          oss << ", ";
        oss << exact_hashimyei_row_ids[i];
      }
      *error =
          "jkimyei components_table has ambiguous hashimyei-specific rows for "
          "component type '" +
          canonical_type + "': [" + oss.str() + "]";
    }
    return false;
  }

  if (compatible_row_ids.empty()) {
    if (error) {
      *error = "jkimyei components_table has no row for component type '" +
               canonical_type + "'";
    }
    return false;
  }

  if (compatible_row_ids.size() != 1) {
    if (error) {
      std::ostringstream oss;
      for (std::size_t i = 0; i < compatible_row_ids.size(); ++i) {
        if (i != 0)
          oss << ", ";
        oss << compatible_row_ids[i];
      }
      *error =
          "jkimyei components_table has ambiguous rows for component type '" +
          canonical_type + "': [" + oss.str() + "]";
    }
    return false;
  }

  *out = compatible_row_ids.front();
  return true;
}

[[nodiscard]] inline bool resolve_vicreg_component_lookup_name(
    std::string_view vicreg_hashimyei,
    const cuwacunu::camahjucunu::jkimyei_specs_t &jkimyei_specs,
    std::string *out, std::string *error) {
  return resolve_wikimyei_component_lookup_name(
      TsiTypeId::WikimyeiRepresentationEncodingVicreg, vicreg_hashimyei,
      jkimyei_specs, out, error);
}

[[nodiscard]] inline bool resolve_expected_value_component_lookup_name(
    std::string_view expected_value_hashimyei,
    const cuwacunu::camahjucunu::jkimyei_specs_t &jkimyei_specs,
    std::string *out, std::string *error) {
  return resolve_wikimyei_component_lookup_name(
      TsiTypeId::WikimyeiInferenceExpectedValueMdn, expected_value_hashimyei,
      jkimyei_specs, out, error);
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

[[nodiscard]] inline const cuwacunu::iitepi::
    contract_instrument_signature_assignment_t *
    find_contract_instrument_signature(
        const std::shared_ptr<const cuwacunu::iitepi::contract_record_t>
            &contract_record,
        std::string_view binding_id) {
  if (!contract_record)
    return nullptr;
  const std::string id = trim_ascii_copy(std::string(binding_id));
  for (const auto &assignment : contract_record->instrument_signatures) {
    if (trim_ascii_copy(assignment.binding_id) == id)
      return &assignment;
  }
  return nullptr;
}

[[nodiscard]] inline bool active_observation_sources_match_runtime(
    const cuwacunu::camahjucunu::observation_spec_t &observation,
    const cuwacunu::camahjucunu::instrument_signature_t &runtime_signature,
    std::string *error = nullptr) {
  return observation.active_sources_match_runtime_signature(runtime_signature,
                                                            error);
}

[[nodiscard]] inline bool validate_selected_wave_instrument_signatures(
    const std::shared_ptr<const cuwacunu::iitepi::contract_record_t>
        &contract_record,
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t
        &effective_instruction,
    const cuwacunu::camahjucunu::iitepi_wave_t &selected_wave,
    const cuwacunu::camahjucunu::observation_spec_t &observation,
    std::string *error = nullptr) {
  if (error)
    error->clear();
  if (!contract_record) {
    if (error)
      *error = "missing contract record for instrument signature validation";
    return false;
  }
  if (selected_wave.sources.size() != 1U) {
    if (error) {
      *error = "instrument signature validation requires exactly one SOURCE "
               "block in selected wave";
    }
    return false;
  }
  const auto &runtime_signature =
      selected_wave.sources.front().runtime_instrument_signature;
  std::string validation_error{};
  if (!cuwacunu::camahjucunu::instrument_signature_validate(
          runtime_signature, /*allow_any=*/false,
          "RUNTIME_INSTRUMENT_SIGNATURE", &validation_error)) {
    if (error)
      *error = validation_error;
    return false;
  }
  if (!active_observation_sources_match_runtime(observation, runtime_signature,
                                                error)) {
    return false;
  }

  for (const auto &circuit : effective_instruction.circuits) {
    for (const auto &instance : circuit.instances) {
      const std::string alias = trim_ascii_copy(instance.alias);
      if (alias.empty()) {
        if (error)
          *error = "empty circuit alias during instrument signature validation";
        return false;
      }
      const auto *assignment =
          find_contract_instrument_signature(contract_record, alias);
      if (!assignment) {
        if (error) {
          *error = "missing DOCK.INSTRUMENT_SIGNATURE for circuit alias '" +
                   alias + "'";
        }
        return false;
      }
      if (!cuwacunu::camahjucunu::instrument_signature_accepts_runtime(
              assignment->signature, runtime_signature,
              "INSTRUMENT_SIGNATURE <" + alias + ">", &validation_error)) {
        if (error)
          *error = validation_error;
        return false;
      }
    }
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
  return source.runtime_instrument_signature.symbol + "[" + source.from + "," +
         source.to + "]";
}

[[nodiscard]] inline std::string compose_invoke_payload_from_wave_source(
    const cuwacunu::camahjucunu::iitepi_wave_source_decl_t &source,
    const cuwacunu::camahjucunu::iitepi_wave_t &wave) {
  const std::string source_command = compose_source_range_command(source);
  std::string payload =
      "wave@symbol:" + source.runtime_instrument_signature.symbol +
      ",epochs:" + std::to_string(wave.epochs) +
      ",episode:0,batch:0,i:0,from:" + source.from + ",to:" + source.to;
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
