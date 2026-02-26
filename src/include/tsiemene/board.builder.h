// ./include/tsiemene/board.builder.h
// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <exception>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <torch/torch.h>

#include "camahjucunu/dsl/canonical_path/canonical_path.h"
#include "camahjucunu/dsl/observation_pipeline/observation_pipeline.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "camahjucunu/dsl/tsiemene_circuit/tsiemene_circuit_runtime.h"

#include "jkimyei/training_setup/jk_setup.h"
#include "piaabo/dconfig.h"
#include "tsiemene/board.h"
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
    const std::string& raw_tsi_type,
    std::string* out,
    std::string* error) {
  if (!out) return false;

  const auto type_path = cuwacunu::camahjucunu::decode_canonical_path(raw_tsi_type);
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
    const cuwacunu::camahjucunu::tsiemene_circuit_decl_t& parsed,
    std::string* out,
    std::string* error) {
  if (!out) return false;

  std::ostringstream oss;
  oss << parsed.name << " = {\n";
  for (const auto& decl : parsed.instances) {
    std::string canonical_tsi_type;
    std::string local_error;
    if (!canonical_tsi_type_for_contract(decl.tsi_type, &canonical_tsi_type, &local_error)) {
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
  oss << parsed.invoke_name << "(" << parsed.invoke_payload << ");\n";
  *out = oss.str();
  return true;
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
    TsiTypeId type_id,
    const cuwacunu::camahjucunu::tsiemene_instance_decl_t& decl,
    BoardContract::Spec* spec,
    const cuwacunu::camahjucunu::observation_instruction_t& observation_instruction,
    const cuwacunu::camahjucunu::jkimyei_specs_t& jkimyei_specs,
    std::string_view jkimyei_specs_dsl_text,
    std::string_view circuit_name,
    torch::Device device,
    const DataloaderT<Datatype_t, Sampler_t>* first_dataloader,
    bool* made_dataloader) {
  if (made_dataloader) *made_dataloader = false;
  if (!spec) return nullptr;

  switch (type_id) {
    case TsiTypeId::SourceDataloader:
      if (made_dataloader) *made_dataloader = true;
      return std::make_unique<DataloaderT<Datatype_t, Sampler_t>>(
          id,
          spec->instrument,
          observation_instruction,
          device);
    case TsiTypeId::WikimyeiRepresentationVicreg: {
      if (!first_dataloader) return nullptr;
      const std::string lookup_component_name =
          resolve_vicreg_component_lookup_name(*spec, jkimyei_specs);
      const auto* component_row =
          find_jkimyei_row_by_id(jkimyei_specs, "components_table", lookup_component_name);
      apply_vicreg_flag_overrides_from_component_row(spec, component_row);

      const std::string runtime_component_name =
          compose_vicreg_runtime_component_name(lookup_component_name, circuit_name, decl.alias);
      spec->representation_component_name = runtime_component_name;
      if (!jkimyei_specs_dsl_text.empty()) {
        cuwacunu::jkimyei::jk_setup_t::registry.set_component_instruction_override(
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
    const cuwacunu::camahjucunu::observation_instruction_t& observation_instruction,
    const cuwacunu::camahjucunu::jkimyei_specs_t& jkimyei_specs,
    std::string_view jkimyei_specs_dsl_text,
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

  out->name = parsed.name;
  out->invoke_name = parsed.invoke_name;
  out->invoke_payload = parsed.invoke_payload;
  out->invoke_source_command.clear();
  out->nodes.clear();
  out->hops.clear();
  out->invalidate_compiled_runtime();

  cuwacunu::camahjucunu::tsiemene_wave_invoke_t invoke{};
  if (!cuwacunu::camahjucunu::parse_circuit_invoke_wave(parsed, &invoke, error)) return false;

  const std::string instrument = invoke.source_symbol;
  if (instrument.empty()) {
    if (error) {
      *error =
          "empty instrument in invoke payload; use symbol in command or wave metadata key symbol: " +
          parsed.invoke_payload;
    }
    return false;
  }

  out->invoke_source_command = invoke.source_command;
  out->spec = BoardContract::Spec{};
  out->spec.instrument = instrument;
  out->spec.sample_type = contract_sample_type_name<Datatype_t>();
  out->spec.sourced_from_config = true;

  std::unordered_map<std::string, Tsi*> alias_to_tsi;
  std::vector<cuwacunu::camahjucunu::tsiemene_resolved_hop_t> resolved_hops;
  DataloaderT<Datatype_t, Sampler_t>* first_dataloader = nullptr;
  std::uint64_t next_id = 1;

  for (const auto& decl : parsed.instances) {
    const auto type_path = cuwacunu::camahjucunu::decode_canonical_path(decl.tsi_type);
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

    bool made_dataloader = false;
    std::unique_ptr<Tsi> node = make_tsi_for_decl<Datatype_t, Sampler_t>(
        next_id++,
        *type_id,
        decl,
        &out->spec,
        observation_instruction,
        jkimyei_specs,
        jkimyei_specs_dsl_text,
        parsed.name,
        device,
        first_dataloader,
        &made_dataloader);

    if (!node) {
      if (error) {
        if (*type_id == TsiTypeId::WikimyeiRepresentationVicreg && !first_dataloader) {
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
      }
    }

    out->nodes.push_back(std::move(node));
  }

  if (first_dataloader) {
    out->spec.channels = first_dataloader->C();
    out->spec.timesteps = first_dataloader->T();
    out->spec.features = first_dataloader->D();
    out->spec.batch_size_hint = first_dataloader->batch_size_hint();
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

  out->wave0 = normalize_wave_span(Wave{
      .id = 0,
      .i = invoke.wave_i,
      .episode = invoke.episode,
      .batch = invoke.batch,
      .span_begin_ms = invoke.span_begin_ms,
      .span_end_ms = invoke.span_end_ms,
      .has_time_span = invoke.has_time_span,
  });
  out->ingress0 = Ingress{
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
    const cuwacunu::piaabo::dconfig::contract_snapshot_t& contract_snapshot,
    Board* out,
    std::string* error = nullptr) {
  if (!out) return false;

  const auto& dsl_sections = contract_snapshot.contract_instruction_sections;

  {
    std::string semantic_error;
    if (!cuwacunu::camahjucunu::validate_circuit_instruction(inst, &semantic_error)) {
      if (error) *error = semantic_error;
      return false;
    }
  }

  out->contracts.clear();
  out->contracts.reserve(inst.circuits.size());

  std::string observation_sources_dsl;
  std::string observation_channels_dsl;
  std::string jkimyei_specs_dsl;
  if (!load_required_dsl_text(
          kBoardContractObservationSourcesDslKey,
          dsl_sections.observation_sources_dsl,
          &observation_sources_dsl,
          error)) {
    return false;
  }
  if (!load_required_dsl_text(
          kBoardContractObservationChannelsDslKey,
          dsl_sections.observation_channels_dsl,
          &observation_channels_dsl,
          error)) {
    return false;
  }
  if (!load_required_dsl_text(
          kBoardContractJkimyeiSpecsDslKey,
          dsl_sections.jkimyei_specs_dsl,
          &jkimyei_specs_dsl,
          error)) {
    return false;
  }

  cuwacunu::camahjucunu::observation_instruction_t observation_instruction{};
  cuwacunu::camahjucunu::jkimyei_specs_t jkimyei_specs{};
  try {
    const auto read_snapshot_dsl_asset = [&](const char* key) -> std::string {
      const auto it = contract_snapshot.dsl_asset_text_by_key.find(key);
      if (it == contract_snapshot.dsl_asset_text_by_key.end() || is_blank_ascii(it->second)) {
        throw std::runtime_error(
            std::string("missing contract snapshot DSL/grammar asset for key: ") + key);
      }
      return it->second;
    };

    const std::string observation_sources_grammar =
        read_snapshot_dsl_asset("observation_sources_grammar_filename");
    const std::string observation_channels_grammar =
        read_snapshot_dsl_asset("observation_channels_grammar_filename");
    const std::string jkimyei_specs_grammar =
        read_snapshot_dsl_asset("jkimyei_specs_grammar_filename");

    observation_instruction =
        cuwacunu::camahjucunu::decode_observation_instruction_from_split_dsl(
            observation_sources_grammar,
            observation_sources_dsl,
            observation_channels_grammar,
            observation_channels_dsl);
    jkimyei_specs = cuwacunu::camahjucunu::dsl::decode_jkimyei_specs_from_dsl(
        jkimyei_specs_grammar,
        jkimyei_specs_dsl);
  } catch (const std::exception& e) {
    if (error) *error = std::string("failed to decode contract DSL payloads: ") + e.what();
    return false;
  }

  for (std::size_t i = 0; i < inst.circuits.size(); ++i) {
    BoardContract c{};
    std::string local_error;
    if (!build_runtime_circuit_from_decl<Datatype_t, Sampler_t>(
            inst.circuits[i],
            observation_instruction,
            jkimyei_specs,
            jkimyei_specs_dsl,
            device,
            &c,
            &local_error)) {
      if (error) {
        *error = "contract[" + std::to_string(i) + "] " + local_error;
      }
      return false;
    }
    std::string circuit_dsl;
    if (!render_contract_circuit_dsl(inst.circuits[i], &circuit_dsl, &local_error)) {
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

    std::string_view missing_key{};
    if (!c.has_required_dsl_segments(&missing_key)) {
      if (error) {
        *error = "contract[" + std::to_string(i) + "] missing required DSL text for key: " +
                 std::string(missing_key);
      }
      return false;
    }

    c.wave0.id = static_cast<WaveId>(i);
    out->contracts.push_back(std::move(c));
  }
  return true;
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
bool build_runtime_board_from_instruction(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t& inst,
    torch::Device device,
    const cuwacunu::piaabo::dconfig::contract_space_t::contract_instruction_sections_t& dsl_sections,
    Board* out,
    std::string* error = nullptr) {
  const auto snapshot = cuwacunu::piaabo::dconfig::contract_runtime_t::active();
  if (!snapshot) {
    if (error) *error = "failed to load contract snapshot from config";
    return false;
  }
  cuwacunu::piaabo::dconfig::contract_snapshot_t snapshot_override = *snapshot;
  snapshot_override.contract_instruction_sections = dsl_sections;
  return build_runtime_board_from_instruction<Datatype_t, Sampler_t>(
      inst, device, snapshot_override, out, error);
}

template <typename Datatype_t,
          typename Sampler_t = torch::data::samplers::SequentialSampler>
bool build_runtime_board_from_instruction(
    const cuwacunu::camahjucunu::tsiemene_circuit_instruction_t& inst,
    torch::Device device,
    Board* out,
    std::string* error = nullptr) {
  if (!out) return false;
  try {
    cuwacunu::piaabo::dconfig::contract_runtime_t::assert_intact_or_fail_fast();
    const auto snapshot = cuwacunu::piaabo::dconfig::contract_runtime_t::active();
    if (!snapshot) {
      if (error) *error = "failed to load contract snapshot from config";
      return false;
    }
    return build_runtime_board_from_instruction<Datatype_t, Sampler_t>(
        inst, device, *snapshot, out, error);
  } catch (const std::exception& e) {
    if (error) *error = std::string("failed to load required DSL text from config: ") + e.what();
    return false;
  }
}

} // namespace board_builder
} // namespace tsiemene
