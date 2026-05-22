// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

#include "jkimyei/api/training_spec.h"
#include "kikijyeba/protocol/source_dock.h"
#include "kikijyeba/settings/wave.h"
#include "kikijyeba/topology/dock_binding.h"
#include "kikijyeba/topology/node_value_chain.h"
#include "kikijyeba/topology/wikimyei_registry.h"
#include "piaabo/parse/simple_kv_block.h"
#include "ujcamei/source/contract/runtime/decode.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "wikimyei/expression/nodelift/srl/assembly.h"
#include "wikimyei/expression/nodelift/srl/nodelift_spec.h"
#include "wikimyei/inference/expected_value/mdn/assembly.h"
#include "wikimyei/inference/expected_value/mdn/mdn_spec.h"
#include "wikimyei/representation/encoding/vicreg/assembly.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_spec.h"

namespace cuwacunu::kikijyeba::protocol {

/*
 * Compiled graph-first protocol contract.
 *
 * A protocol contract is the validated artifact that a wave runs against. It
 * is compiled from authored Ujcamei, Kikijyeba, Wikimyei, and Jkimyei config
 * files. It contains source availability, the active topology/source dock,
 * worker assemblies, dock bindings, and training/inference specs.
 *
 * Wave settings are decoded alongside the contract in v1 because the current
 * launcher reads one `.config` bundle, but contract identity intentionally
 * excludes the wave range/mode: the same compiled contract can run many waves.
 *
 * DEV_WARNING: this v1 contract identity still includes a small set of
 * Jkimyei training selectors such as training id, component id, and batch size.
 * Runtime model-state choices such as loaded checkpoint paths are intentionally
 * excluded and must be represented by job reports, exposure facts, and
 * checkpoint lineage.
 */
struct graph_first_protocol_contract_t {
  std::string config_path{};
  cuwacunu::ujcamei::source::contract::source_registry_config_paths_t
      source_paths{};
  graph_first_source_dock_paths_t source_dock_paths{};
  ujcamei_source_universe_t source_universe{};
  graph_first_source_dock_t source_dock{};
  graph_first_source_resolution_report_t source_resolution_report{};
  resolved_graph_first_source_plan_t source_plan{};
  // Transitional compatibility shape for Ujcamei storage/validation surfaces
  // that still accept the historical merged source_spec_t.
  cuwacunu::ujcamei::source::contract::source_spec_t source_spec{};
  std::string wikimyei_expression_nodelift_srl_dsl_bnf_path{};
  std::string wikimyei_expression_nodelift_srl_dsl_path{};
  std::string wikimyei_representation_vicreg_dsl_bnf_path{};
  std::string wikimyei_representation_vicreg_dsl_path{};
  std::string wikimyei_representation_vicreg_net_bnf_path{};
  std::string wikimyei_representation_vicreg_net_path{};
  std::string wikimyei_inference_expected_value_mdn_dsl_bnf_path{};
  std::string wikimyei_inference_expected_value_mdn_dsl_path{};
  std::string wikimyei_inference_expected_value_mdn_net_bnf_path{};
  std::string wikimyei_inference_expected_value_mdn_net_path{};
  std::string wikimyei_representation_vicreg_jkimyei_bnf_path{};
  std::string wikimyei_representation_vicreg_jkimyei_path{};
  std::string wikimyei_inference_expected_value_mdn_jkimyei_bnf_path{};
  std::string wikimyei_inference_expected_value_mdn_jkimyei_path{};
  std::string kikijyeba_settings_wave_dsl_bnf_path{};
  std::string kikijyeba_settings_wave_dsl_path{};

  cuwacunu::kikijyeba::settings::wave_settings_t wave_settings{};
  cuwacunu::wikimyei::expression::nodelift::srl::nodelift_srl_spec_t nodelift{};
  cuwacunu::wikimyei::representation::encoding::vicreg::
      vicreg_node_representation_spec_t vicreg{};
  cuwacunu::wikimyei::inference::expected_value::mdn::mdn_spec_t mdn{};
  cuwacunu::wikimyei::assembly::wikimyei_assembly_t nodelift_assembly{};
  cuwacunu::wikimyei::assembly::wikimyei_assembly_t vicreg_assembly{};
  cuwacunu::wikimyei::assembly::wikimyei_assembly_t mdn_assembly{};
  cuwacunu::kikijyeba::topology::wikimyei_registry_t wikimyei_registry{};
  cuwacunu::kikijyeba::topology::dock_binding_t dock_binding{};
  cuwacunu::kikijyeba::topology::dock_binding_validation_report_t
      dock_binding_report{};
  cuwacunu::jkimyei::training::training_run_spec_t vicreg_training{};
  cuwacunu::jkimyei::training::training_run_spec_t mdn_training{};
};

// Compatibility name retained while older call sites migrate to contract
// vocabulary.
using graph_first_config_bundle_t = graph_first_protocol_contract_t;

inline void
populate_graph_first_source_plan(graph_first_config_bundle_t &bundle) {
  if (bundle.source_universe.empty() &&
      !bundle.source_spec.source_forms.empty()) {
    bundle.source_universe = make_ujcamei_source_universe(bundle.source_spec);
  }
  if (bundle.source_dock.empty() &&
      (!bundle.source_spec.channel_forms.empty() ||
       !bundle.source_spec.graph_node_forms.empty() ||
       !bundle.source_spec.graph_edge_forms.empty())) {
    bundle.source_dock = make_graph_first_source_dock(bundle.source_spec);
  }
  if (bundle.source_plan.compat_source_spec.source_forms.empty() &&
      !bundle.source_universe.empty() && !bundle.source_dock.empty()) {
    bundle.source_dock = resolve_graph_first_source_dock_edges(
        bundle.source_universe, bundle.source_dock);
    bundle.source_resolution_report = analyze_graph_first_source_resolution(
        bundle.source_universe, bundle.source_dock);
    bundle.source_plan = resolve_graph_first_source_plan(bundle.source_universe,
                                                         bundle.source_dock);
  }
  if (bundle.source_spec.source_forms.empty() &&
      !bundle.source_plan.compat_source_spec.source_forms.empty()) {
    bundle.source_spec = bundle.source_plan.compat_source_spec;
  }
}

[[nodiscard]] inline cuwacunu::kikijyeba::topology::dock_binding_t
make_graph_first_dock_binding(const graph_first_config_bundle_t &bundle) {
  namespace topo = cuwacunu::kikijyeba::topology;

  const auto node_count = static_cast<std::int64_t>(
      bundle.source_plan.market_graph.node_ids.size());
  const auto edge_count = static_cast<std::int64_t>(
      bundle.source_plan.market_graph.edge_ids.size());
  const auto channel_count = active_channel_count(bundle.source_dock);
  const auto input_length = max_input_length(bundle.source_dock);
  const auto future_length = max_future_length(bundle.source_dock);

  topo::dock_binding_t out{};
  out.binding_id = topo::kGraphTopologyDockBindingId;
  out.variables = {
      topo::make_runtime_variable("B", "kikijyeba.settings.wave.batch_pulse"),
      topo::make_static_i64_variable("N", "kikijyeba.topology.graph.node_count",
                                     node_count),
      topo::make_static_i64_variable("L", "kikijyeba.topology.graph.edge_count",
                                     edge_count),
      topo::make_static_i64_variable(
          "C", "ujcamei.source.retrieval.active_channel_count", channel_count),
      topo::make_static_i64_variable(
          "Hx", "ujcamei.source.retrieval.input_length", input_length),
      topo::make_static_i64_variable(
          "Hf", "ujcamei.source.retrieval.future_length", future_length),
      topo::make_static_i64_variable(
          "F", "ujcamei.source.registry.kline_feature_width",
          cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth),
      topo::make_static_i64_variable(
          "De", "wikimyei.representation.encoding.vicreg.encoding_dim",
          bundle.vicreg.encoding_dim),
      topo::make_static_i64_variable(
          "Df", "wikimyei.inference.expected_value.mdn.target_coord_count",
          static_cast<std::int64_t>(bundle.mdn.target_coords.size())),
      topo::make_static_i64_variable(
          "K", "wikimyei.inference.expected_value.mdn.mixture_count",
          bundle.mdn.mixture_count),
  };
  out.constraints.push_back(
      "NodeLift.node_lifted_state -> VICReg.node_lifted_state");
  out.constraints.push_back(
      "VICReg.node_representation -> MDN.node_representation");
  out.constraints.push_back(
      "NodeLift.future_node_lifted_state -> MDN.future_node_lifted_state");
  out.constraints.push_back("B is runtime-bound per wave pulse");
  topo::validate_dock_binding(out);
  return out;
}

[[nodiscard]] inline std::vector<
    cuwacunu::wikimyei::assembly::wikimyei_assembly_t>
graph_first_wikimyei_assemblies(const graph_first_config_bundle_t &bundle) {
  return {bundle.nodelift_assembly, bundle.vicreg_assembly,
          bundle.mdn_assembly};
}

[[nodiscard]] inline std::string canonical_graph_first_protocol_contract_text(
    const graph_first_protocol_contract_t &contract) {
  namespace assembly = cuwacunu::wikimyei::assembly;
  namespace types = cuwacunu::ujcamei::source::registry::types;

  std::ostringstream out;
  out << "schema=kikijyeba.protocol.graph_first.contract.v1\n";
  out << "source_count=" << contract.source_universe.source_forms.size()
      << "\n";
  for (const auto &source : contract.source_universe.source_forms) {
    out << "source=" << source.instrument << "|"
        << types::enum_to_string(source.interval) << "|" << source.record_type
        << "|" << source.market_type << "|" << source.venue << "|"
        << source.base_asset << "|" << source.quote_asset << "|"
        << source.source_kind << "|" << source.source << "\n";
  }
  out << "retrieval_channel_count=" << contract.source_dock.channel_forms.size()
      << "\n";
  for (const auto &channel : contract.source_dock.channel_forms) {
    out << "channel=" << types::enum_to_string(channel.interval) << "|"
        << channel.active << "|" << channel.record_type << "|"
        << channel.input_length << "|" << channel.future_length << "|"
        << channel.channel_weight << "|" << channel.normalization_policy
        << "\n";
  }
  out << "edge_resolution_policy="
      << contract.source_resolution_report.edge_resolution_policy << "\n";
  out << "edge_source_kind="
      << contract.source_resolution_report.edge_source_kind << "\n";
  out << "fetch_mode=" << contract.source_resolution_report.fetch_mode << "\n";
  out << "node_count=" << contract.source_plan.market_graph.node_ids.size()
      << "\n";
  for (const auto &node : contract.source_plan.market_graph.node_ids) {
    out << "node=" << node << "\n";
  }
  out << "edge_count=" << contract.source_plan.market_graph.edge_ids.size()
      << "\n";
  for (std::size_t i = 0; i < contract.source_plan.market_graph.edge_ids.size();
       ++i) {
    out << "edge=" << contract.source_plan.market_graph.edge_ids.at(i) << "|"
        << contract.source_plan.market_graph.base_index.at(i) << "|"
        << contract.source_plan.market_graph.quote_index.at(i) << "\n";
  }
  out << "graph_order_fingerprint="
      << contract.source_plan.market_graph.computed_graph_order_fingerprint()
      << "\n";
  out << "nodelift_assembly="
      << assembly::assembly_fingerprint(contract.nodelift_assembly) << "\n";
  out << "vicreg_assembly="
      << assembly::assembly_fingerprint(contract.vicreg_assembly) << "\n";
  out << "mdn_assembly="
      << assembly::assembly_fingerprint(contract.mdn_assembly) << "\n";
  out << "dock_binding="
      << cuwacunu::kikijyeba::topology::dock_binding_fingerprint(
             contract.dock_binding)
      << "\n";
  out << "vicreg_training_id=" << contract.vicreg_training.training_id << "\n";
  out << "vicreg_training_component=" << contract.vicreg_training.component_id
      << "\n";
  out << "vicreg_training_batch_size=" << contract.vicreg_training.batch_size
      << "\n";
  out << "mdn_training_id=" << contract.mdn_training.training_id << "\n";
  out << "mdn_training_component=" << contract.mdn_training.component_id
      << "\n";
  out << "mdn_training_batch_size=" << contract.mdn_training.batch_size << "\n";
  out << "mdn_target_coords=";
  for (std::size_t i = 0; i < contract.mdn.target_coords.size(); ++i) {
    if (i != 0)
      out << ",";
    out << contract.mdn.target_coords.at(i);
  }
  out << "\n";
  return out.str();
}

[[nodiscard]] inline std::string graph_first_protocol_contract_fingerprint(
    const graph_first_protocol_contract_t &contract) {
  std::uint64_t hash =
      cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, "cuwacunu.kikijyeba.protocol.graph_first.contract.v1");
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, canonical_graph_first_protocol_contract_text(contract));
  return cuwacunu::wikimyei::assembly::assembly_detail::hash_hex(hash);
}

[[nodiscard]] inline std::string graph_first_protocol_contract_token(
    const graph_first_protocol_contract_t &contract) {
  return std::string("kikijyeba.protocol.graph_first.contract.v1/") +
         graph_first_protocol_contract_fingerprint(contract);
}

[[nodiscard]] inline cuwacunu::kikijyeba::topology::
    dock_binding_validation_report_t
    make_graph_first_dock_binding_report(
        const graph_first_config_bundle_t &bundle) {
  return cuwacunu::kikijyeba::topology::analyze_dock_binding_against_assemblies(
      bundle.dock_binding, graph_first_wikimyei_assemblies(bundle));
}

inline void
validate_graph_first_dock_binding(const graph_first_config_bundle_t &bundle) {
  namespace topo = cuwacunu::kikijyeba::topology;
  topo::validate_dock_binding(bundle.dock_binding);
  topo::require_runtime_binding(bundle.dock_binding, "B", "wave pulse batch");
  topo::require_static_i64_binding_value(
      bundle.dock_binding, "N",
      static_cast<std::int64_t>(
          bundle.source_plan.market_graph.node_ids.size()),
      "graph node count");
  topo::require_static_i64_binding_value(
      bundle.dock_binding, "L",
      static_cast<std::int64_t>(
          bundle.source_plan.market_graph.edge_ids.size()),
      "graph edge count");
  topo::require_static_i64_binding_value(
      bundle.dock_binding, "C", active_channel_count(bundle.source_dock),
      "active channel count");
  topo::require_static_i64_binding_value(bundle.dock_binding, "Hx",
                                         max_input_length(bundle.source_dock),
                                         "input window length");
  topo::require_static_i64_binding_value(bundle.dock_binding, "Hf",
                                         max_future_length(bundle.source_dock),
                                         "future window length");
  topo::require_static_i64_binding_value(
      bundle.dock_binding, "F",
      cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth,
      "kline feature width");
  topo::require_static_i64_binding_value(bundle.dock_binding, "De",
                                         bundle.vicreg.encoding_dim,
                                         "VICReg encoding dimension");
  topo::require_static_i64_binding_value(
      bundle.dock_binding, "Df",
      static_cast<std::int64_t>(bundle.mdn.target_coords.size()),
      "MDN target coordinate count");
  topo::require_static_i64_binding_value(
      bundle.dock_binding, "K", bundle.mdn.mixture_count, "MDN mixture count");
  (void)make_graph_first_dock_binding_report(bundle);
}

inline void validate_graph_first_protocol_contract(
    const graph_first_protocol_contract_t &bundle) {
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  namespace nodelift = cuwacunu::wikimyei::expression::nodelift::srl;

  nodelift::validate_nodelift_srl_spec(bundle.nodelift);
  vicreg::validate_vicreg_node_representation_spec(bundle.vicreg);
  mdn::validate_mdn_spec(bundle.mdn);
  if (bundle.nodelift_assembly.component_id != bundle.nodelift.component_id ||
      bundle.nodelift_assembly.version_token != bundle.nodelift.version_token) {
    throw std::runtime_error(
        "[graph_first_config] NodeLift assembly does not match NodeLift spec");
  }
  if (bundle.vicreg_assembly.component_id != bundle.vicreg.component_id ||
      bundle.vicreg_assembly.version_token != bundle.vicreg.version_token) {
    throw std::runtime_error(
        "[graph_first_config] VICReg assembly does not match VICReg spec");
  }
  if (bundle.mdn_assembly.component_id != bundle.mdn.component_id ||
      bundle.mdn_assembly.version_token != bundle.mdn.version_token) {
    throw std::runtime_error(
        "[graph_first_config] MDN assembly does not match MDN spec");
  }
  cuwacunu::kikijyeba::topology::validate_node_value_assembly_chain(
      bundle.nodelift_assembly, bundle.vicreg_assembly, bundle.mdn_assembly);
  if (bundle.wikimyei_registry.assemblies.size() != 3) {
    throw std::runtime_error(
        "[graph_first_config] expected three Wikimyei assemblies");
  }
  cuwacunu::jkimyei::training::validate_training_run_spec(
      bundle.vicreg_training);
  cuwacunu::jkimyei::training::validate_training_run_spec(bundle.mdn_training);
  cuwacunu::kikijyeba::settings::validate_wave_settings(bundle.wave_settings);
  if (bundle.source_universe.empty()) {
    throw std::runtime_error(
        "[graph_first_config] Ujcamei source universe is empty");
  }
  if (bundle.source_dock.empty()) {
    throw std::runtime_error("[graph_first_config] source dock is empty");
  }
  if (bundle.source_plan.compat_source_spec.source_forms.empty()) {
    throw std::runtime_error(
        "[graph_first_config] resolved source plan is empty");
  }
  validate_graph_first_dock_binding(bundle);

  if (bundle.mdn.input_representation_id != bundle.vicreg.component_id) {
    throw std::runtime_error(
        "[graph_first_config] mdn input representation id does not match "
        "VICReg component id");
  }
  if (bundle.vicreg_training.task !=
      cuwacunu::jkimyei::training::training_task_t::
          vicreg_node_representation) {
    throw std::runtime_error(
        "[graph_first_config] VICReg training spec must use "
        "vicreg_node_representation task");
  }
  if (bundle.vicreg_training.component_id != bundle.vicreg.component_id) {
    throw std::runtime_error(
        "[graph_first_config] VICReg training component id does not match "
        "VICReg component id");
  }
  if (bundle.mdn_training.task != cuwacunu::jkimyei::training::training_task_t::
                                      mdn_expected_value_inference) {
    throw std::runtime_error("[graph_first_config] MDN training spec must use "
                             "mdn_expected_value_inference task");
  }
  if (bundle.mdn_training.component_id != bundle.mdn.component_id) {
    throw std::runtime_error(
        "[graph_first_config] MDN training component id does not match MDN "
        "component id");
  }
}

inline void
validate_graph_first_config_bundle(const graph_first_config_bundle_t &bundle) {
  validate_graph_first_protocol_contract(bundle);
}

namespace graph_first_config_detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline std::string
read_text_file_or_throw(const std::string &path) {
  std::ifstream in(path);
  if (!in) {
    throw std::runtime_error("[graph_first_config] unable to open file: " +
                             path);
  }
  return std::string((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
}

[[nodiscard]] inline std::unordered_map<std::string, std::string>
parse_assignment_config(const std::string &config_path) {
  std::unordered_map<std::string, std::string> out;
  std::istringstream lines(read_text_file_or_throw(config_path));
  std::string line;
  std::size_t line_number = 0;
  while (std::getline(lines, line)) {
    ++line_number;
    const auto comment = line.find('#');
    if (comment != std::string::npos) {
      line.resize(comment);
    }
    line = kv::trim(std::move(line));
    if (line.empty() || (line.front() == '[' && line.back() == ']')) {
      continue;
    }
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      throw std::runtime_error("[graph_first_config] invalid config line " +
                               std::to_string(line_number) + ": missing '='");
    }
    auto key = kv::trim(line.substr(0, eq));
    auto value = kv::trim(line.substr(eq + 1));
    if (key.empty() || value.empty()) {
      throw std::runtime_error("[graph_first_config] invalid config line " +
                               std::to_string(line_number) +
                               ": empty key or value");
    }
    out[std::move(key)] = std::move(value);
  }
  return out;
}

[[nodiscard]] inline std::string
required_config_value(const std::unordered_map<std::string, std::string> &cfg,
                      const std::string &key, const std::string &config_path) {
  const auto it = cfg.find(key);
  if (it == cfg.end() || kv::trim(it->second).empty()) {
    throw std::runtime_error("[graph_first_config] missing required key '" +
                             key + "' in " + config_path);
  }
  return it->second;
}

} // namespace graph_first_config_detail

[[nodiscard]] inline graph_first_protocol_contract_t
load_graph_first_protocol_contract_from_config(std::string config_path = {}) {
  if (cuwacunu::piaabo::parse::simple_kv::trim(config_path).empty()) {
    config_path =
        cuwacunu::ujcamei::source::contract::default_source_config_path();
  }
  const auto cfg =
      graph_first_config_detail::parse_assignment_config(config_path);

  graph_first_protocol_contract_t out{};
  out.config_path = config_path;
  out.source_paths = cuwacunu::ujcamei::source::contract::
      load_source_registry_config_paths_from_config(config_path);
  out.source_dock_paths = graph_first_source_dock_paths_t{
      .retrieval_channels_dsl_bnf_path =
          graph_first_config_detail::required_config_value(
              cfg, "ujcamei_source_retrieval_channels_dsl_bnf_path",
              config_path),
      .retrieval_channels_dsl_path =
          graph_first_config_detail::required_config_value(
              cfg, "ujcamei_source_retrieval_channels_dsl_path", config_path),
      .graph_dsl_bnf_path = graph_first_config_detail::required_config_value(
          cfg, "kikijyeba_topology_graph_dsl_bnf_path", config_path),
      .graph_dsl_path = graph_first_config_detail::required_config_value(
          cfg, "kikijyeba_topology_graph_dsl_path", config_path),
  };
  out.source_universe =
      cuwacunu::ujcamei::source::contract::decode_source_universe_from_config(
          config_path);
  out.source_dock = decode_graph_first_source_dock_from_split_dsl(
      graph_first_config_detail::read_text_file_or_throw(
          out.source_dock_paths.retrieval_channels_dsl_bnf_path),
      graph_first_config_detail::read_text_file_or_throw(
          out.source_dock_paths.retrieval_channels_dsl_path),
      graph_first_config_detail::read_text_file_or_throw(
          out.source_dock_paths.graph_dsl_bnf_path),
      graph_first_config_detail::read_text_file_or_throw(
          out.source_dock_paths.graph_dsl_path));
  out.source_dock = resolve_graph_first_source_dock_edges(out.source_universe,
                                                          out.source_dock);
  out.source_resolution_report = analyze_graph_first_source_resolution(
      out.source_universe, out.source_dock);
  out.source_plan =
      resolve_graph_first_source_plan(out.source_universe, out.source_dock);
  out.source_spec = out.source_plan.compat_source_spec;

  out.wikimyei_expression_nodelift_srl_dsl_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_expression_nodelift_srl_dsl_bnf_path", config_path);
  out.wikimyei_expression_nodelift_srl_dsl_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_expression_nodelift_srl_dsl_path", config_path);
  out.wikimyei_representation_vicreg_dsl_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_representation_vicreg_dsl_bnf_path", config_path);
  out.wikimyei_representation_vicreg_dsl_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_representation_vicreg_dsl_path", config_path);
  out.wikimyei_representation_vicreg_net_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_representation_vicreg_net_bnf_path", config_path);
  out.wikimyei_representation_vicreg_net_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_representation_vicreg_net_path", config_path);
  out.wikimyei_inference_expected_value_mdn_dsl_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_inference_expected_value_mdn_dsl_bnf_path",
          config_path);
  out.wikimyei_inference_expected_value_mdn_dsl_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_inference_expected_value_mdn_dsl_path", config_path);
  out.wikimyei_inference_expected_value_mdn_net_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_inference_expected_value_mdn_net_bnf_path",
          config_path);
  out.wikimyei_inference_expected_value_mdn_net_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_inference_expected_value_mdn_net_path", config_path);
  out.wikimyei_representation_vicreg_jkimyei_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_representation_vicreg_jkimyei_bnf_path", config_path);
  out.wikimyei_representation_vicreg_jkimyei_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_representation_vicreg_jkimyei_path", config_path);
  out.wikimyei_inference_expected_value_mdn_jkimyei_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_inference_expected_value_mdn_jkimyei_bnf_path",
          config_path);
  out.wikimyei_inference_expected_value_mdn_jkimyei_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_inference_expected_value_mdn_jkimyei_path",
          config_path);
  out.kikijyeba_settings_wave_dsl_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "kikijyeba_settings_wave_dsl_bnf_path", config_path);
  out.kikijyeba_settings_wave_dsl_path =
      graph_first_config_detail::required_config_value(
          cfg, "kikijyeba_settings_wave_dsl_path", config_path);

  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_expression_nodelift_srl_dsl_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_representation_vicreg_dsl_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_representation_vicreg_net_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_inference_expected_value_mdn_dsl_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_inference_expected_value_mdn_net_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_representation_vicreg_jkimyei_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_inference_expected_value_mdn_jkimyei_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.kikijyeba_settings_wave_dsl_bnf_path);

  out.wave_settings =
      cuwacunu::kikijyeba::settings::decode_wave_settings_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.kikijyeba_settings_wave_dsl_path));
  out.nodelift = cuwacunu::wikimyei::expression::nodelift::srl::
      decode_nodelift_srl_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_expression_nodelift_srl_dsl_path));
  out.vicreg = cuwacunu::wikimyei::representation::encoding::vicreg::
      decode_vicreg_node_representation_spec_from_split_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_representation_vicreg_dsl_path),
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_representation_vicreg_net_path));
  out.mdn = cuwacunu::wikimyei::inference::expected_value::mdn::
      decode_mdn_spec_from_split_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_inference_expected_value_mdn_dsl_path),
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_inference_expected_value_mdn_net_path));
  out.nodelift_assembly =
      cuwacunu::wikimyei::expression::nodelift::srl::make_nodelift_srl_assembly(
          out.nodelift);
  out.vicreg_assembly = cuwacunu::wikimyei::representation::encoding::vicreg::
      make_vicreg_representation_assembly(out.vicreg);
  out.mdn_assembly = cuwacunu::wikimyei::inference::expected_value::mdn::
      make_mdn_expected_value_assembly(out.mdn);
  out.wikimyei_registry.add(out.nodelift_assembly);
  out.wikimyei_registry.add(out.vicreg_assembly);
  out.wikimyei_registry.add(out.mdn_assembly);
  out.vicreg_training =
      cuwacunu::jkimyei::training::decode_training_run_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_representation_vicreg_jkimyei_path));
  out.mdn_training =
      cuwacunu::jkimyei::training::decode_training_run_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_inference_expected_value_mdn_jkimyei_path));

  populate_graph_first_source_plan(out);
  out.dock_binding = make_graph_first_dock_binding(out);
  out.dock_binding_report = make_graph_first_dock_binding_report(out);
  validate_graph_first_protocol_contract(out);
  return out;
}

[[nodiscard]] inline graph_first_config_bundle_t
load_graph_first_config_bundle_from_config(std::string config_path = {}) {
  return load_graph_first_protocol_contract_from_config(std::move(config_path));
}

} // namespace cuwacunu::kikijyeba::protocol
