// ./include/iitepi/board.builder.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <exception>
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
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"
#include "camahjucunu/dsl/tsiemene_wave/tsiemene_wave.h"

#include "jkimyei/training_setup/jk_setup.h"
#include "piaabo/dconfig.h"
#include "iitepi/board/board.h"
#include "iitepi/board/board.validation.h"
#include "tsiemene/tsi.type.registry.h"
#include "tsiemene/tsi.source.dataloader.h"
#include "tsiemene/tsi.wikimyei.representation.vicreg.h"
#include "tsiemene/tsi.sink.null.h"
#include "tsiemene/tsi.sink.log.sys.h"

namespace tsiemene {
namespace board_builder {

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
using DataloaderT = TsiSourceDataloader<Datatype_t, Sampler_t>;

[[nodiscard]] inline bool is_blank_ascii(std::string_view text) {
  for (char ch : text) {
    if (!std::isspace(static_cast<unsigned char>(ch))) return false;
  }
  return true;
}

[[nodiscard]] inline bool load_required_dsl_text(std::string_view key,
                                                 std::string text,
                                                 std::string* out,
                                                 std::string* error) {
  if (!out) return false;
  if (is_blank_ascii(text)) {
    if (error) *error = "missing required DSL text for key: " + std::string(key);
    return false;
  }
  *out = std::move(text);
  return true;
}

[[nodiscard]] inline std::string trim_ascii_copy(std::string s) {
  std::size_t b = 0;
  while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
  std::size_t e = s.size();
  while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
  return s.substr(b, e - b);
}

[[nodiscard]] inline std::string lower_ascii_copy(std::string s) {
  for (char& ch : s) {
    if (ch >= 'A' && ch <= 'Z') ch = static_cast<char>(ch - 'A' + 'a');
  }
  return s;
}

[[nodiscard]] inline std::optional<bool> parse_bool_ascii(std::string value) {
  value = lower_ascii_copy(trim_ascii_copy(std::move(value)));
  if (value == "1" || value == "true" || value == "yes" || value == "on") return true;
  if (value == "0" || value == "false" || value == "no" || value == "off") return false;
  return std::nullopt;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::jkimyei_specs_t::row_t*
find_jkimyei_row_by_id(const cuwacunu::camahjucunu::jkimyei_specs_t& specs,
                       std::string_view table_name,
                       std::string_view row_id) {
  const auto table_it = specs.tables.find(std::string(table_name));
  if (table_it == specs.tables.end()) return nullptr;
  for (const auto& row : table_it->second) {
    const auto rid_it = row.find(ROW_ID_COLUMN_HEADER);
    if (rid_it == row.end()) continue;
    if (trim_ascii_copy(rid_it->second) == row_id) return &row;
  }
  return nullptr;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::jkimyei_specs_t::row_t*
find_jkimyei_component_profile_row(
    const cuwacunu::camahjucunu::jkimyei_specs_t& specs,
    std::string_view component_id,
    std::string_view profile_id) {
  const auto table_it = specs.tables.find("component_profiles_table");
  if (table_it == specs.tables.end()) return nullptr;
  for (const auto& row : table_it->second) {
    const auto cid_it = row.find("component_id");
    const auto pid_it = row.find("profile_id");
    if (cid_it == row.end() || pid_it == row.end()) continue;
    if (trim_ascii_copy(cid_it->second) != component_id) continue;
    if (trim_ascii_copy(pid_it->second) != profile_id) continue;
    return &row;
  }
  return nullptr;
}

[[nodiscard]] inline std::string resolve_vicreg_component_lookup_name(
    const BoardContract::Spec& spec,
    const cuwacunu::camahjucunu::jkimyei_specs_t& jkimyei_specs) {
  constexpr std::string_view kBase = "VICReg_representation";
  if (spec.representation_hashimyei.empty()) return std::string(kBase);

  const std::string dot_name = std::string(kBase) + "." + spec.representation_hashimyei;
  if (find_jkimyei_row_by_id(jkimyei_specs, "components_table", dot_name)) {
    return dot_name;
  }

  const std::string underscore_name = std::string(kBase) + "_" + spec.representation_hashimyei;
  if (find_jkimyei_row_by_id(jkimyei_specs, "components_table", underscore_name)) {
    return underscore_name;
  }

  return std::string(kBase);
}

inline void apply_vicreg_flag_overrides_from_component_row(
    BoardContract::Spec* spec,
    const cuwacunu::camahjucunu::jkimyei_specs_t::row_t* row) {
  if (!spec || !row) return;

  const auto assign_if_present = [&](const char* key, bool* out_value) {
    const auto it = row->find(key);
    if (it == row->end() || !out_value) return;
    const auto parsed = parse_bool_ascii(it->second);
    if (parsed.has_value()) *out_value = *parsed;
  };

  assign_if_present("vicreg_train", &spec->vicreg_train);
  assign_if_present("vicreg_use_swa", &spec->vicreg_use_swa);
  assign_if_present("vicreg_detach_to_cpu", &spec->vicreg_detach_to_cpu);
}

[[nodiscard]] inline std::string compose_vicreg_runtime_component_name(
    std::string_view lookup_component_name,
    std::string_view circuit_name,
    std::string_view alias) {
  std::string out(lookup_component_name);
  out.append("@");
  out.append(circuit_name);
  out.append(".");
  out.append(alias);
  return out;
}

[[nodiscard]] inline bool canonical_tsi_type_for_contract(
    const std::string& contract_hash,
    const std::string& raw_tsi_type,
    std::string* out,
    std::string* error) {
  if (!out) return false;

  const auto type_path = cuwacunu::camahjucunu::decode_canonical_path(
      raw_tsi_type, contract_hash);
  if (!type_path.ok) {
    if (error) *error = "invalid tsi_type canonical path: " + type_path.error;
    return false;
  }
  if (type_path.path_kind != cuwacunu::camahjucunu::canonical_path_kind_e::Node) {
    if (error) *error = "tsi_type must be a canonical node path: " + type_path.canonical;
    return false;
  }

  const auto type_id = parse_tsi_type_id(type_path.canonical_identity);
  if (!type_id) {
    if (error) *error = "unsupported tsi_type: " + type_path.canonical_identity;
    return false;
  }

  const auto* type_desc = find_tsi_type(*type_id);
  if (!type_desc) {
    if (error) *error = "missing tsi type descriptor in manifest for: " + type_path.canonical_identity;
    return false;
  }

  *out = std::string(type_desc->canonical);
  if (!type_path.hashimyei.empty()) {
    out->append(".");
    out->append(type_path.hashimyei);
  }
  return true;
}

[[nodiscard]] inline bool render_contract_circuit_dsl(
    const std::string& contract_hash,
    const cuwacunu::camahjucunu::tsiemene_circuit_decl_t& parsed,
    std::string* out,
    std::string* error) {
  if (!out) return false;

  std::ostringstream oss;
  oss << parsed.name << " = {\n";
  for (const auto& decl : parsed.instances) {
    std::string canonical_tsi_type;
    std::string local_error;
    if (!canonical_tsi_type_for_contract(
            contract_hash, decl.tsi_type, &canonical_tsi_type, &local_error)) {
      if (error) {
        *error = "unable to canonicalize tsi_type for alias " + decl.alias + ": " + local_error;
      }
      return false;
    }
    oss << "  " << decl.alias << " = " << canonical_tsi_type << "\n";
  }
  for (const auto& h : parsed.hops) {
    if (h.to.directive.empty()) {
      if (error) {
        *error = "hop target directive is empty while rendering canonical circuit DSL: " +
                 h.from.instance + " -> " + h.to.instance;
      }
      return false;
    }
    oss << "  " << h.from.instance << h.from.directive << ":" << h.from.kind
        << " -> "
        << h.to.instance << h.to.directive
        << "\n";
  }
  oss << "}\n";
  *out = oss.str();
  return true;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::tsiemene_wave_t*
select_wave_by_id(
    const cuwacunu::camahjucunu::tsiemene_wave_set_t& instruction,
    std::string wave_id,
    std::string* error = nullptr) {
  wave_id = trim_ascii_copy(std::move(wave_id));
  if (wave_id.empty()) {
    if (error) *error = "missing required wave id";
    return nullptr;
  }
  const cuwacunu::camahjucunu::tsiemene_wave_t* chosen = nullptr;
  for (const auto& wave : instruction.waves) {
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
  if (chosen) return chosen;
  if (error) {
    *error = "no WAVE matches requested wave id '" + wave_id + "'";
  }
  return nullptr;
}

[[nodiscard]] inline std::string canonical_runtime_node_path(
    const std::string& raw_path,
    const std::string& contract_hash,
    std::string* error = nullptr) {
  const auto parsed = cuwacunu::camahjucunu::decode_canonical_path(raw_path, contract_hash);
  if (!parsed.ok) {
    if (error) *error = parsed.error;
    return {};
  }
  if (parsed.path_kind != cuwacunu::camahjucunu::canonical_path_kind_e::Node) {
    if (error) *error = "path must resolve to canonical node";
    return {};
  }
  std::string out = parsed.canonical_identity;
  if (!parsed.hashimyei.empty()) {
    const std::string suffix = "." + parsed.hashimyei;
    if (out.size() < suffix.size() ||
        out.compare(out.size() - suffix.size(), suffix.size(), suffix) != 0) {
      out.append(suffix);
    }
  }
  return out;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::tsiemene_wave_wikimyei_decl_t*
find_wave_wikimyei_decl_by_path(
    const cuwacunu::camahjucunu::tsiemene_wave_t& wave,
    std::string_view canonical_path,
    const std::string& contract_hash) {
  for (const auto& w : wave.wikimyeis) {
    if (canonical_runtime_node_path(w.wikimyei_path, contract_hash, nullptr) ==
        canonical_path) {
      return &w;
    }
  }
  return nullptr;
}

[[nodiscard]] inline const cuwacunu::camahjucunu::tsiemene_wave_source_decl_t*
find_wave_source_decl_by_path(
    const cuwacunu::camahjucunu::tsiemene_wave_t& wave,
    std::string_view canonical_path,
    const std::string& contract_hash) {
  for (const auto& s : wave.sources) {
    if (canonical_runtime_node_path(s.source_path, contract_hash, nullptr) ==
        canonical_path) {
      return &s;
    }
  }
  return nullptr;
}

[[nodiscard]] inline std::string compose_source_range_command(
    const cuwacunu::camahjucunu::tsiemene_wave_source_decl_t& source) {
  return source.symbol + "[" + source.from + "," + source.to + "]";
}

[[nodiscard]] inline std::string compose_invoke_payload_from_wave_source(
    const cuwacunu::camahjucunu::tsiemene_wave_source_decl_t& source,
    const cuwacunu::camahjucunu::tsiemene_wave_t& wave) {
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
  const char* record =
      cuwacunu::camahjucunu::data::detail::record_type_name_for_datatype<Datatype_t>();
  if (!record) return {};
  return std::string(record);
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
std::unique_ptr<Tsi> make_tsi_for_decl(
    TsiId id,
    const cuwacunu::iitepi::contract_hash_t& contract_hash,
    TsiTypeId type_id,
    const cuwacunu::camahjucunu::tsiemene_instance_decl_t& decl,
    BoardContract::Spec* spec,
    const cuwacunu::camahjucunu::observation_spec_t& observation_instruction,
    const cuwacunu::camahjucunu::jkimyei_specs_t& jkimyei_specs,
    std::string_view jkimyei_specs_dsl_text,
    std::string_view circuit_name,
    torch::Device device,
    std::size_t source_batch_size_override,
    const DataloaderT<Datatype_t, Sampler_t>* first_dataloader,
    const cuwacunu::camahjucunu::tsiemene_wave_wikimyei_decl_t* wave_wikimyei_decl,
    bool* made_dataloader,
    std::string* error) {
  if (made_dataloader) *made_dataloader = false;
  if (!spec) return nullptr;

  switch (type_id) {
    case TsiTypeId::SourceDataloader:
      if (made_dataloader) *made_dataloader = true;
      return std::make_unique<DataloaderT<Datatype_t, Sampler_t>>(
          id,
          spec->instrument,
          observation_instruction,
          device,
          source_batch_size_override);
    case TsiTypeId::WikimyeiRepresentationVicreg: {
      if (!first_dataloader) return nullptr;

      const std::string base_component_lookup_name =
          resolve_vicreg_component_lookup_name(*spec, jkimyei_specs);
      std::string lookup_component_name = base_component_lookup_name;
      const cuwacunu::camahjucunu::jkimyei_specs_t::row_t* selected_profile_row =
          nullptr;

      if (wave_wikimyei_decl) {
        spec->vicreg_train = wave_wikimyei_decl->train;
        selected_profile_row = find_jkimyei_component_profile_row(
            jkimyei_specs,
            base_component_lookup_name,
            wave_wikimyei_decl->profile_id);
        if (!selected_profile_row) {
          if (error) {
            *error = "wave profile_id '" + wave_wikimyei_decl->profile_id +
                     "' not found for component '" + base_component_lookup_name + "'";
          }
          return nullptr;
        }
        lookup_component_name =
            base_component_lookup_name + "@" + wave_wikimyei_decl->profile_id;
      }

      const auto* component_row =
          find_jkimyei_row_by_id(jkimyei_specs, "components_table", base_component_lookup_name);
      if (selected_profile_row) {
        apply_vicreg_flag_overrides_from_component_row(spec, selected_profile_row);
      } else {
        apply_vicreg_flag_overrides_from_component_row(spec, component_row);
      }
      if (wave_wikimyei_decl) {
        spec->vicreg_train = wave_wikimyei_decl->train;
      }

      const std::string runtime_component_name =
          compose_vicreg_runtime_component_name(lookup_component_name, circuit_name, decl.alias);
      spec->representation_component_name = runtime_component_name;
      if (!jkimyei_specs_dsl_text.empty()) {
        cuwacunu::jkimyei::jk_setup_t::registry.set_component_instruction_override(
            contract_hash,
            runtime_component_name,
            lookup_component_name,
            std::string(jkimyei_specs_dsl_text));
      }

      // Runtime ctor settings are derived from contract spec.
      const int C = (spec->channels > 0) ? static_cast<int>(spec->channels)
                                        : static_cast<int>(first_dataloader->C());
      const int T = (spec->timesteps > 0) ? static_cast<int>(spec->timesteps)
                                         : static_cast<int>(first_dataloader->T());
      const int D = (spec->features > 0) ? static_cast<int>(spec->features)
                                        : static_cast<int>(first_dataloader->D());
      return std::make_unique<TsiWikimyeiRepresentationVicreg>(
          id,
          decl.alias,
          contract_hash,
          runtime_component_name,
          C,
          T,
          D,
          /*train=*/spec->vicreg_train,
          /*use_swa=*/spec->vicreg_use_swa,
          /*detach_to_cpu=*/spec->vicreg_detach_to_cpu);
    }
    case TsiTypeId::SinkNull:
      return std::make_unique<TsiSinkNull>(id, decl.alias);
    case TsiTypeId::SinkLogSys:
      return std::make_unique<TsiSinkLogSys>(id, decl.alias);
  }
  return nullptr;
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
bool build_runtime_circuit_from_decl(
    const cuwacunu::camahjucunu::tsiemene_circuit_decl_t& parsed,
    const cuwacunu::iitepi::contract_hash_t& contract_hash,
    const cuwacunu::camahjucunu::observation_spec_t& observation_instruction,
    const cuwacunu::camahjucunu::jkimyei_specs_t& jkimyei_specs,
    std::string_view jkimyei_specs_dsl_text,
    const cuwacunu::camahjucunu::tsiemene_wave_t* wave,
    torch::Device device,
    BoardContract* out,
    std::string* error = nullptr) {
  if (!out) return false;

  {
    std::string semantic_error;
    if (!cuwacunu::camahjucunu::validate_circuit_decl(parsed, &semantic_error)) {
      if (error) *error = semantic_error;
      return false;
    }
  }

  std::string effective_invoke_name = parsed.invoke_name;
  if (is_blank_ascii(effective_invoke_name)) effective_invoke_name = parsed.name;
  std::string effective_invoke_payload = trim_ascii_copy(parsed.invoke_payload);

  if (is_blank_ascii(effective_invoke_name)) {
    if (error) *error = "empty circuit invoke name";
    return false;
  }

  out->name = parsed.name;
  out->invoke_name = effective_invoke_name;
  out->invoke_payload.clear();
  out->invoke_source_command.clear();
  out->nodes.clear();
  out->hops.clear();
  out->invalidate_compiled_runtime();
  out->spec = BoardContract::Spec{};
  out->execution = BoardContract::Execution{};
  out->spec.sample_type = contract_sample_type_name<Datatype_t>();
  out->spec.sourced_from_config = true;
  if (wave) {
    out->execution.epochs = wave->epochs;
    out->execution.batch_size = wave->batch_size;
  }
  if (wave &&
      wave->batch_size >
          static_cast<std::uint64_t>(std::numeric_limits<std::size_t>::max())) {
    if (error) {
      *error = "wave BATCH_SIZE exceeds runtime supported range";
    }
    return false;
  }
  const std::size_t source_batch_size_override =
      (out->execution.batch_size > 0)
          ? static_cast<std::size_t>(out->execution.batch_size)
          : 0;

  std::unordered_map<std::string, Tsi*> alias_to_tsi;
  std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t> resolved_hops;
  DataloaderT<Datatype_t, Sampler_t>* first_dataloader = nullptr;
  std::uint64_t next_id = 1;
  std::unordered_set<std::string> circuit_wikimyei_paths;
  std::unordered_set<std::string> circuit_source_paths;
  const cuwacunu::camahjucunu::tsiemene_wave_source_decl_t* selected_wave_source =
      nullptr;

  for (const auto& decl : parsed.instances) {
    const auto type_path = cuwacunu::camahjucunu::decode_canonical_path(
        decl.tsi_type, contract_hash);
    if (!type_path.ok) {
      if (error) *error = "invalid tsi_type canonical path for alias " + decl.alias + ": " + type_path.error;
      return false;
    }
    if (type_path.path_kind != cuwacunu::camahjucunu::canonical_path_kind_e::Node) {
      if (error) *error = "tsi_type must be a canonical node path for alias " + decl.alias + ": " + type_path.canonical;
      return false;
    }

    const auto type_id = parse_tsi_type_id(type_path.canonical_identity);
    if (!type_id) {
      if (error) *error = "unsupported tsi_type: " + type_path.canonical_identity;
      return false;
    }
    const auto* type_desc = find_tsi_type(*type_id);
    if (!type_desc) {
      if (error) *error = "missing tsi type descriptor in manifest for: " + type_path.canonical_identity;
      return false;
    }

    const std::string canonical_type(type_desc->canonical);
    if (std::find(out->spec.component_types.begin(),
                  out->spec.component_types.end(),
                  canonical_type) == out->spec.component_types.end()) {
      out->spec.component_types.push_back(canonical_type);
    }
    if (type_desc->domain == TsiDomain::Source && out->spec.source_type.empty()) {
      out->spec.source_type = canonical_type;
    }
    if (type_desc->domain == TsiDomain::Wikimyei && out->spec.representation_type.empty()) {
      out->spec.representation_type = canonical_type;
    }
    if (type_desc->domain == TsiDomain::Wikimyei &&
        out->spec.representation_hashimyei.empty() &&
        !type_path.hashimyei.empty()) {
      out->spec.representation_hashimyei = type_path.hashimyei;
    }
    const std::string decl_path = canonical_runtime_node_path(
        decl.tsi_type, contract_hash, error);
    if (decl_path.empty()) {
      if (error && error->empty()) {
        *error = "invalid canonical runtime path for alias '" + decl.alias + "'";
      }
      return false;
    }

    const cuwacunu::camahjucunu::tsiemene_wave_wikimyei_decl_t* wave_wikimyei_decl =
        nullptr;
    if (wave && type_desc->domain == TsiDomain::Wikimyei) {
      circuit_wikimyei_paths.insert(decl_path);
      wave_wikimyei_decl = find_wave_wikimyei_decl_by_path(*wave, decl_path, contract_hash);
      if (!wave_wikimyei_decl) {
        if (error) {
          *error = "missing WIKIMYEI wave block for path '" + decl_path + "'";
        }
        return false;
      }
    }
    if (wave && type_desc->domain == TsiDomain::Source) {
      circuit_source_paths.insert(decl_path);
      const auto* wave_source_decl =
          find_wave_source_decl_by_path(*wave, decl_path, contract_hash);
      if (!wave_source_decl) {
        if (error) {
          *error = "missing SOURCE wave block for path '" + decl_path + "'";
        }
        return false;
      }
      if (!selected_wave_source) selected_wave_source = wave_source_decl;
      // Source dataloader construction requires instrument up-front.
      if (out->spec.instrument.empty()) {
        out->spec.instrument = trim_ascii_copy(wave_source_decl->symbol);
      }
    }

    bool made_dataloader = false;
    std::unique_ptr<Tsi> node = make_tsi_for_decl<Datatype_t, Sampler_t>(
        next_id++,
        contract_hash,
        *type_id,
        decl,
        &out->spec,
        observation_instruction,
        jkimyei_specs,
        jkimyei_specs_dsl_text,
        parsed.name,
        device,
        source_batch_size_override,
        first_dataloader,
        wave_wikimyei_decl,
        &made_dataloader,
        error);

    if (!node) {
      if (error) {
        if (!error->empty()) {
          /* preserve richer error from make_tsi_for_decl */
        } else if (*type_id == TsiTypeId::WikimyeiRepresentationVicreg &&
                   !first_dataloader) {
          *error = "vicreg requires a dataloader declared earlier in the same circuit";
        } else {
          *error = "unsupported tsi_type: " + type_path.canonical_identity;
        }
      }
      return false;
    }

    auto [it, inserted] = alias_to_tsi.emplace(decl.alias, node.get());
    if (!inserted) {
      if (error) *error = "duplicated instance alias: " + decl.alias;
      return false;
    }
    (void)it;

    if (made_dataloader && !first_dataloader) {
      first_dataloader = dynamic_cast<DataloaderT<Datatype_t, Sampler_t>*>(node.get());
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

    out->nodes.push_back(std::move(node));
  }

  if (wave) {
    for (const auto& w : wave->wikimyeis) {
      const std::string wave_path =
          canonical_runtime_node_path(w.wikimyei_path, contract_hash, nullptr);
      if (wave_path.empty() ||
          circuit_wikimyei_paths.find(wave_path) == circuit_wikimyei_paths.end()) {
        if (error) {
          *error = "wave '" + wave->name +
                   "' contains unknown WIKIMYEI PATH not present in circuit: " +
                   w.wikimyei_path;
        }
        return false;
      }
    }
    for (const auto& s : wave->sources) {
      const std::string wave_path =
          canonical_runtime_node_path(s.source_path, contract_hash, nullptr);
      if (wave_path.empty() ||
          circuit_source_paths.find(wave_path) == circuit_source_paths.end()) {
        if (error) {
          *error = "wave '" + wave->name +
                   "' contains unknown SOURCE PATH not present in circuit: " +
                   s.source_path;
        }
        return false;
      }
    }

    if (circuit_source_paths.size() != 1) {
      if (error) {
        *error =
            "runtime currently supports exactly one SOURCE path per circuit when wave is enabled";
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
    effective_invoke_payload =
        compose_invoke_payload_from_wave_source(*selected_wave_source, *wave);
  } else if (is_blank_ascii(effective_invoke_payload)) {
    if (error) *error = "empty circuit invoke payload";
    return false;
  }

  out->invoke_payload = effective_invoke_payload;
  cuwacunu::camahjucunu::tsiemene_circuit_decl_t invoke_decl = parsed;
  invoke_decl.invoke_name = effective_invoke_name;
  invoke_decl.invoke_payload = effective_invoke_payload;

  cuwacunu::camahjucunu::tsiemene_wave_invoke_t invoke{};
  if (!cuwacunu::camahjucunu::parse_circuit_invoke_wave(
          invoke_decl, &invoke, error)) {
    return false;
  }
  out->invoke_source_command = invoke.source_command;
  out->spec.instrument = invoke.source_symbol;
  if (invoke.total_epochs > 0) {
    out->execution.epochs = invoke.total_epochs;
  }
  if (out->spec.instrument.empty()) {
    if (error) {
      *error =
          "empty instrument in invoke payload; use symbol in command or wave metadata key symbol: " +
          effective_invoke_payload;
    }
    return false;
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
  out->spec.future_timesteps = observation_instruction_copy.max_future_sequence_length();

  if (!cuwacunu::camahjucunu::resolve_hops(parsed, &resolved_hops, error)) return false;

  out->hops.reserve(resolved_hops.size());
  for (const auto& h : resolved_hops) {
    const auto it_from = alias_to_tsi.find(h.from.instance);
    const auto it_to = alias_to_tsi.find(h.to.instance);
    if (it_from == alias_to_tsi.end() || it_to == alias_to_tsi.end()) {
      if (error) *error = "hop references unknown instance alias: " + h.from.instance + " -> " + h.to.instance;
      return false;
    }

    const auto* out_spec = find_directive(*it_from->second, h.from.directive, DirectiveDir::Out);
    const auto* in_spec = find_directive(*it_to->second, h.to.directive, DirectiveDir::In);
    if (!out_spec || !in_spec) {
      if (error) {
        *error = "hop directive not found on tsi declarations: " +
                 h.from.instance + "@" + std::string(h.from.directive) +
                 " -> " +
                 h.to.instance + "@" + std::string(h.to.directive);
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
                 " -> " +
                 h.to.instance + "@" + std::string(h.to.directive);
      }
      return false;
    }
    if (in_spec->kind.kind != h.to.kind) {
      if (error) {
        *error = "hop target kind mismatch against tsi declarations: " +
                 h.to.instance + "@" + std::string(h.to.directive);
      }
      return false;
    }

    out->hops.push_back(hop(
        ep(*it_from->second, h.from.directive),
        ep(*it_to->second, h.to.directive),
        query("")));
  }

  out->seed_wave = normalize_wave_span(Wave{
      .cursor = WaveCursor{
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
  out->seed_ingress = Ingress{
    .directive = pick_start_directive(out->view()),
    .signal = string_signal(invoke.source_command)
  };
  return true;
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
bool build_runtime_board_from_instruction(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t& inst,
    torch::Device device,
    const cuwacunu::iitepi::contract_hash_t& contract_hash,
    const std::shared_ptr<const cuwacunu::iitepi::contract_record_t>&
        contract_record,
    const cuwacunu::iitepi::wave_hash_t& wave_hash,
    const std::shared_ptr<const cuwacunu::iitepi::wave_record_t>&
        wave_record,
    std::string wave_id,
    Board* out,
    std::string* error = nullptr) {
  if (!out) return false;
  if (!contract_record) {
    if (error) *error = "missing contract record";
    return false;
  }
  if (!wave_record) {
    if (error) *error = "missing wave record";
    return false;
  }
  (void)wave_hash;

  const auto contract_report = validate_contract_definition(inst, contract_hash);
  if (!contract_report.ok) {
    if (error) {
      *error = contract_report.indicators.empty()
                   ? "contract validation failed"
                   : contract_report.indicators.front().message;
    }
    return false;
  }

  out->contract_hash = contract_hash;
  out->wave_hash = wave_hash;
  out->contracts.clear();
  out->contracts.reserve(inst.circuits.size());

  std::string observation_sources_dsl;
  std::string observation_channels_dsl;
  std::string jkimyei_specs_dsl;
  std::string wave_dsl;
  if (!load_required_dsl_text(
          kBoardContractObservationSourcesDslKey,
          contract_record->observation.sources.dsl,
          &observation_sources_dsl,
          error)) {
    return false;
  }
  if (!load_required_dsl_text(
          kBoardContractObservationChannelsDslKey,
          contract_record->observation.channels.dsl,
          &observation_channels_dsl,
          error)) {
    return false;
  }
  if (!load_required_dsl_text(
          kBoardContractJkimyeiSpecsDslKey,
          contract_record->jkimyei.dsl,
          &jkimyei_specs_dsl,
          error)) {
    return false;
  }
  if (!load_required_dsl_text(
          kBoardContractWaveDslKey,
          wave_record->wave.dsl,
          &wave_dsl,
          error)) {
    return false;
  }

  cuwacunu::camahjucunu::observation_spec_t observation_instruction{};
  cuwacunu::camahjucunu::jkimyei_specs_t jkimyei_specs{};
  cuwacunu::camahjucunu::tsiemene_wave_set_t wave_set{};
  try {
    observation_instruction = contract_record->observation.decoded();
    jkimyei_specs = contract_record->jkimyei.decoded();
    wave_set = wave_record->wave.decoded();
  } catch (const std::exception& e) {
    if (error) *error = std::string("failed to decode contract/wave DSL payloads: ") + e.what();
    return false;
  }

  const auto* selected_wave = select_wave_by_id(wave_set, std::move(wave_id), error);
  if (!selected_wave) return false;
  const auto wave_report = validate_wave_definition(*selected_wave, contract_hash);
  if (!wave_report.ok) {
    if (error) {
      *error = wave_report.indicators.empty()
                   ? "wave validation failed"
                   : wave_report.indicators.front().message;
    }
    return false;
  }
  const auto compat_report = validate_wave_contract_compatibility(
      inst,
      *selected_wave,
      contract_hash,
      &jkimyei_specs,
      contract_hash,
      selected_wave->name);
  if (!compat_report.ok) {
    if (error) {
      *error = compat_report.indicators.empty()
                   ? "wave/contract compatibility validation failed"
                   : compat_report.indicators.front().message;
    }
    return false;
  }

  for (std::size_t i = 0; i < inst.circuits.size(); ++i) {
    BoardContract c{};
    std::string local_error;
    if (!build_runtime_circuit_from_decl<Datatype_t, Sampler_t>(
            inst.circuits[i],
            contract_hash,
            observation_instruction,
            jkimyei_specs,
            jkimyei_specs_dsl,
            selected_wave,
            device,
            &c,
            &local_error)) {
      if (error) {
        *error = "contract[" + std::to_string(i) + "] " + local_error;
      }
      return false;
    }
    std::string circuit_dsl;
    if (!render_contract_circuit_dsl(
            contract_hash, inst.circuits[i], &circuit_dsl, &local_error)) {
      if (error) *error = "contract[" + std::to_string(i) + "] " + local_error;
      return false;
    }
    if (is_blank_ascii(circuit_dsl)) {
      if (error) {
        *error = "contract[" + std::to_string(i) + "] missing required DSL text for key: " +
                 std::string(kBoardContractCircuitDslKey);
      }
      return false;
    }

    c.set_dsl_segment(kBoardContractCircuitDslKey, std::move(circuit_dsl));
    c.set_dsl_segment(kBoardContractObservationSourcesDslKey, observation_sources_dsl);
    c.set_dsl_segment(kBoardContractObservationChannelsDslKey, observation_channels_dsl);
    c.set_dsl_segment(kBoardContractJkimyeiSpecsDslKey, jkimyei_specs_dsl);
    c.set_dsl_segment(kBoardContractWaveDslKey, wave_dsl);

    std::string_view missing_key{};
    if (!c.has_required_dsl_segments(&missing_key)) {
      if (error) {
        *error = "contract[" + std::to_string(i) + "] missing required DSL text for key: " +
                 std::string(missing_key);
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
bool build_runtime_board_from_instruction(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t& inst,
    torch::Device device,
    const cuwacunu::iitepi::contract_hash_t& contract_hash,
    const cuwacunu::iitepi::wave_hash_t& wave_hash,
    std::string wave_id,
    Board* out,
    std::string* error = nullptr) {
  if (!out) return false;
  try {
    cuwacunu::iitepi::contract_space_t::assert_intact_or_fail_fast(
        contract_hash);
    cuwacunu::iitepi::wave_space_t::assert_intact_or_fail_fast(
        wave_hash);
    const auto contract_itself =
        cuwacunu::iitepi::contract_space_t::contract_itself(contract_hash);
    const auto wave_itself =
        cuwacunu::iitepi::wave_space_t::wave_itself(wave_hash);
    return build_runtime_board_from_instruction<Datatype_t, Sampler_t>(
        inst,
        device,
        contract_hash,
        contract_itself,
        wave_hash,
        wave_itself,
        std::move(wave_id),
        out,
        error);
  } catch (const std::exception& e) {
    if (error) *error = std::string("failed to load required DSL text from config: ") + e.what();
    return false;
  }
}

} // namespace board_builder
} // namespace tsiemene
