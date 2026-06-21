// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <unordered_map>
#include <vector>

#include "hero/runtime_hero/runtime/wave_settings.h"
#include "jkimyei/api/training_spec.h"
#include "kikijyeba/environment/replay/spec.h"
#include "kikijyeba/protocol/protocol_variant.h"
#include "kikijyeba/protocol/source_dock.h"
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
#include "wikimyei/observer/belief/assembly.h"
#include "wikimyei/observer/belief/spec.h"
#include "wikimyei/policy/portfolio/graph_node_allocation/assembly.h"
#include "wikimyei/policy/portfolio/graph_node_allocation/spec.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/assembly.h"
#include "wikimyei/policy/portfolio/spot_distributional_utility/spec.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/assembly.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg_spec.h"
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
struct channel_graph_first_protocol_contract_t {
  std::string config_path{};
  cuwacunu::ujcamei::source::contract::source_registry_config_paths_t
      source_paths{};
  graph_first_source_dock_paths_t source_dock_paths{};
  ujcamei_source_universe_t source_universe{};
  graph_first_source_dock_t source_dock{};
  graph_first_source_resolution_report_t source_resolution_report{};
  resolved_graph_first_source_plan_t source_plan{};
  cuwacunu::ujcamei::source::contract::source_spec_t source_spec{};
  std::string wikimyei_expression_nodelift_srl_dsl_bnf_path{};
  std::string wikimyei_expression_nodelift_srl_dsl_path{};
  std::string wikimyei_representation_vicreg_dsl_bnf_path{};
  std::string wikimyei_representation_vicreg_dsl_path{};
  std::string wikimyei_representation_vicreg_net_bnf_path{};
  std::string wikimyei_representation_vicreg_net_path{};
  std::string wikimyei_representation_mtf_jepa_mae_vicreg_dsl_bnf_path{};
  std::string wikimyei_representation_mtf_jepa_mae_vicreg_dsl_path{};
  std::string wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path{};
  std::string wikimyei_representation_mtf_jepa_mae_vicreg_net_path{};
  std::string wikimyei_inference_expected_value_mdn_dsl_bnf_path{};
  std::string wikimyei_inference_expected_value_mdn_dsl_path{};
  std::string wikimyei_inference_expected_value_mdn_net_bnf_path{};
  std::string wikimyei_inference_expected_value_mdn_net_path{};
  std::string wikimyei_observer_belief_dsl_bnf_path{};
  std::string wikimyei_observer_belief_dsl_path{};
  std::string
      wikimyei_policy_portfolio_spot_distributional_utility_dsl_bnf_path{};
  std::string wikimyei_policy_portfolio_spot_distributional_utility_dsl_path{};
  std::string wikimyei_policy_portfolio_graph_node_allocation_dsl_bnf_path{};
  std::string wikimyei_policy_portfolio_graph_node_allocation_dsl_path{};
  std::string wikimyei_policy_portfolio_graph_node_allocation_net_bnf_path{};
  std::string wikimyei_policy_portfolio_graph_node_allocation_net_path{};
  std::string
      wikimyei_policy_portfolio_graph_node_allocation_features_bnf_path{};
  std::string wikimyei_policy_portfolio_graph_node_allocation_features_path{};
  std::string wikimyei_representation_vicreg_jkimyei_bnf_path{};
  std::string wikimyei_representation_vicreg_jkimyei_path{};
  std::string wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_bnf_path{};
  std::string wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path{};
  std::string wikimyei_inference_expected_value_mdn_jkimyei_bnf_path{};
  std::string wikimyei_inference_expected_value_mdn_jkimyei_path{};
  std::string
      wikimyei_policy_portfolio_graph_node_allocation_jkimyei_bnf_path{};
  std::string wikimyei_policy_portfolio_graph_node_allocation_jkimyei_path{};
  std::string runtime_wave_dsl_bnf_path{};
  std::string runtime_wave_dsl_path{};
  std::string runtime_wave_id{};
  std::string ujcamei_source_cursor_dsl_bnf_path{};
  std::string ujcamei_source_cursor_dsl_path{};
  std::string kikijyeba_protocol_dsl_bnf_path{};
  std::string kikijyeba_protocol_dsl_path{};
  std::string kikijyeba_environment_replay_dsl_bnf_path{};
  std::string kikijyeba_environment_replay_dsl_path{};

  cuwacunu::kikijyeba::protocol::protocol_variant_t protocol_variant{};
  cuwacunu::hero::runtime::settings::wave_settings_t wave_settings{};
  cuwacunu::kikijyeba::environment::replay_environment_spec_t
      replay_environment{};
  cuwacunu::wikimyei::expression::nodelift::srl::nodelift_srl_spec_t nodelift{};
  cuwacunu::wikimyei::representation::encoding::vicreg::vicreg_spec_t vicreg{};
  cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg::
      mtf_jepa_mae_vicreg_spec_t mtf_jepa_mae_vicreg{};
  cuwacunu::wikimyei::inference::expected_value::mdn::channel_mdn_spec_t
      channel_mdn{};
  cuwacunu::wikimyei::observer::belief::belief_observer_spec_t
      belief_observer{};
  cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::
      spot_distributional_utility_spec_t spot_distributional_utility{};
  cuwacunu::wikimyei::policy::portfolio::graph_node_allocation::
      graph_node_allocation_spec_t graph_node_allocation{};
  cuwacunu::wikimyei::policy::portfolio::graph_node_allocation::
      graph_node_allocation_net_spec_t graph_node_allocation_net{};
  cuwacunu::wikimyei::policy::portfolio::graph_node_allocation::
      graph_node_allocation_feature_manifest_spec_t
          graph_node_allocation_features{};
  cuwacunu::wikimyei::assembly::wikimyei_assembly_t nodelift_assembly{};
  cuwacunu::wikimyei::assembly::wikimyei_assembly_t vicreg_assembly{};
  cuwacunu::wikimyei::assembly::wikimyei_assembly_t
      mtf_jepa_mae_vicreg_assembly{};
  cuwacunu::wikimyei::assembly::wikimyei_assembly_t channel_mdn_assembly{};
  cuwacunu::wikimyei::assembly::wikimyei_assembly_t belief_observer_assembly{};
  cuwacunu::wikimyei::assembly::wikimyei_assembly_t
      spot_distributional_utility_assembly{};
  cuwacunu::wikimyei::assembly::wikimyei_assembly_t
      graph_node_allocation_assembly{};
  cuwacunu::kikijyeba::topology::wikimyei_registry_t wikimyei_registry{};
  cuwacunu::kikijyeba::topology::dock_binding_t dock_binding{};
  cuwacunu::kikijyeba::topology::dock_binding_validation_report_t
      dock_binding_report{};
  cuwacunu::jkimyei::training::training_run_spec_t vicreg_training{};
  cuwacunu::jkimyei::training::training_run_spec_t
      mtf_jepa_mae_vicreg_training{};
  cuwacunu::jkimyei::training::training_run_spec_t channel_mdn_training{};
  cuwacunu::jkimyei::training::training_run_spec_t
      graph_node_allocation_training{};
};

using channel_graph_first_config_bundle_t =
    channel_graph_first_protocol_contract_t;

[[nodiscard]] inline bool active_protocol_uses_mtf_jepa_mae_vicreg(
    const channel_graph_first_config_bundle_t &bundle) {
  return bundle.protocol_variant.representation_family ==
         protocol_representation_family_t::mtf_jepa_mae_vicreg;
}

[[nodiscard]] inline const cuwacunu::wikimyei::assembly::wikimyei_assembly_t &
active_representation_assembly(
    const channel_graph_first_config_bundle_t &bundle) {
  return active_protocol_uses_mtf_jepa_mae_vicreg(bundle)
             ? bundle.mtf_jepa_mae_vicreg_assembly
             : bundle.vicreg_assembly;
}

[[nodiscard]] inline std::string active_representation_family_id(
    const channel_graph_first_config_bundle_t &bundle) {
  return protocol_representation_family_name(
      bundle.protocol_variant.representation_family);
}

[[nodiscard]] inline std::string active_representation_component_assembly_id(
    const channel_graph_first_config_bundle_t &bundle) {
  return active_representation_assembly(bundle).component_assembly_id;
}

[[nodiscard]] inline std::int64_t active_representation_encoding_dim(
    const channel_graph_first_config_bundle_t &bundle) {
  return active_protocol_uses_mtf_jepa_mae_vicreg(bundle)
             ? bundle.mtf_jepa_mae_vicreg.config.latent_dim
             : bundle.vicreg.encoding_dim;
}

[[nodiscard]] inline cuwacunu::hero::runtime::settings::wave_protocol_bindings_t
runtime_wave_protocol_bindings(const protocol_variant_t &variant) {
  cuwacunu::hero::runtime::settings::wave_protocol_bindings_t out{};
  out.protocol_id = variant.protocol_id;
  out.representation_family =
      protocol_representation_family_name(variant.representation_family);
  out.inference_family = variant.inference_family;
  out.allocation_policy_family = variant.allocation_policy_family;
  out.policy_component_family = variant.policy_component_family;
  return out;
}

inline void populate_channel_graph_first_source_plan(
    channel_graph_first_config_bundle_t &bundle) {
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
make_channel_graph_first_dock_binding(
    const channel_graph_first_config_bundle_t &bundle) {
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
      topo::make_runtime_variable("B", "hero.runtime.wave.batch_pulse"),
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
          "De", "wikimyei.representation.active.encoding_dim",
          active_representation_encoding_dim(bundle)),
      topo::make_static_i64_variable(
          "Df", "wikimyei.inference.expected_value.mdn.target_coord_count",
          static_cast<std::int64_t>(bundle.channel_mdn.target_coords.size())),
      topo::make_static_i64_variable(
          "K", "wikimyei.inference.expected_value.mdn.mixture_count",
          bundle.channel_mdn.mixture_count),
      topo::make_static_i64_variable("G", "kikijyeba.topology.graph.node_count",
                                     node_count),
      topo::make_static_i64_variable(
          "Kc", "wikimyei.observer.belief.channel_consensus_component_count",
          channel_count * bundle.channel_mdn.mixture_count),
      topo::make_static_i64_variable("S",
                                     "wikimyei.observer.belief.scenario_count",
                                     bundle.belief_observer.scenario_count),
      topo::make_runtime_variable(
          "A", "wikimyei.observer.belief.allocatable_asset_count"),
  };
  out.constraints.push_back(
      active_protocol_uses_mtf_jepa_mae_vicreg(bundle)
          ? "NodeLift.node_lifted_state -> MTF-JEPA-MAE-VICReg.adapter"
          : "NodeLift.node_lifted_state -> VICReg.node_lifted_state");
  out.constraints.push_back(
      active_protocol_uses_mtf_jepa_mae_vicreg(bundle)
          ? "MTF-JEPA-MAE-VICReg.graph_restored_channel_representation -> "
            "ChannelMDN.channel_node_representation"
          : "VICReg.channel_node_representation -> "
            "ChannelMDN.channel_node_representation");
  out.constraints.push_back("NodeLift.future_node_lifted_state -> "
                            "ChannelMDN.future_node_lifted_state");
  out.constraints.push_back(
      "ChannelMDN.future_node_distribution -> ObserverBelief."
      "allocation_belief");
  out.constraints.push_back("ObserverBelief.allocation_belief -> "
                            "SpotDistributionalUtility.allocation_target");
  out.constraints.push_back("ObserverBelief.allocation_belief -> "
                            "GraphNodeAllocation.policy_input");
  out.constraints.push_back("B is runtime-bound per wave pulse");
  out.constraints.push_back(
      "A is runtime-bound by the allocatable graph-node universe");
  topo::validate_dock_binding(out);
  return out;
}

[[nodiscard]] inline std::vector<
    cuwacunu::wikimyei::assembly::wikimyei_assembly_t>
channel_graph_first_wikimyei_assemblies(
    const channel_graph_first_config_bundle_t &bundle) {
  return {bundle.nodelift_assembly,
          bundle.vicreg_assembly,
          bundle.mtf_jepa_mae_vicreg_assembly,
          bundle.channel_mdn_assembly,
          bundle.belief_observer_assembly,
          bundle.spot_distributional_utility_assembly,
          bundle.graph_node_allocation_assembly};
}

namespace config_bundle_detail {

inline void append_i64_list(std::ostringstream &out,
                            const std::vector<int64_t> &values) {
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << values.at(i);
  }
}

inline void append_string_list(std::ostringstream &out,
                               const std::vector<std::string> &values) {
  for (std::size_t i = 0; i < values.size(); ++i) {
    if (i != 0) {
      out << ",";
    }
    out << values.at(i);
  }
}

[[nodiscard]] inline const char *channel_input_route_name(
    cuwacunu::wikimyei::representation::encoding::vicreg::vicreg_input_route_t
        route) {
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  switch (route) {
  case vicreg::vicreg_input_route_t::channel_node_stream:
    return "channel_node_stream";
  }
  return "unknown";
}

[[nodiscard]] inline const char *cell_valid_policy_name(
    cuwacunu::wikimyei::representation::encoding::vicreg::cell_valid_policy_t
        policy) {
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  switch (policy) {
  case vicreg::cell_valid_policy_t::any_feature:
    return "any_feature";
  case vicreg::cell_valid_policy_t::all_features:
    return "all_features";
  case vicreg::cell_valid_policy_t::required_features:
    return "required_features";
  case vicreg::cell_valid_policy_t::min_valid_fraction:
    return "min_valid_fraction";
  }
  return "unknown";
}

[[nodiscard]] inline const char *
channel_context_mode_name(cuwacunu::wikimyei::inference::expected_value::mdn::
                              channel_mdn_context_mode_t mode) {
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  switch (mode) {
  case mdn::channel_mdn_context_mode_t::channel_context_strict:
    return "channel_context_strict";
  case mdn::channel_mdn_context_mode_t::channel_context_plus_global:
    return "channel_context_plus_global";
  }
  return "unknown";
}

[[nodiscard]] inline const char *
channel_target_domain_name(cuwacunu::wikimyei::inference::expected_value::mdn::
                               channel_mdn_target_domain_t domain) {
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  switch (domain) {
  case mdn::channel_mdn_target_domain_t::channel_node_future:
    return "channel_node_future";
  }
  return "unknown";
}

[[nodiscard]] inline const char *channel_target_mask_policy_name(
    cuwacunu::wikimyei::inference::expected_value::mdn::stream::
        channel_target_mask_policy_t policy) {
  namespace stream = cuwacunu::wikimyei::inference::expected_value::mdn::stream;
  switch (policy) {
  case stream::channel_target_mask_policy_t::per_target_feature_valid:
    return "per_target_feature_valid";
  }
  return "unknown";
}

[[nodiscard]] inline const char *
channel_activity_target_name(cuwacunu::wikimyei::inference::expected_value::
                                 mdn::channel_mdn_activity_target_t target) {
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  switch (target) {
  case mdn::channel_mdn_activity_target_t::node_feature_support_mean:
    return "node_feature_support_mean";
  }
  return "unknown";
}

[[nodiscard]] inline const char *
training_task_name(cuwacunu::jkimyei::training::training_task_t task) {
  namespace training = cuwacunu::jkimyei::training;
  switch (task) {
  case training::training_task_t::mdn_expected_value_inference:
    return "mdn_expected_value_inference";
  case training::training_task_t::vicreg_representation:
    return "vicreg_representation";
  case training::training_task_t::mtf_jepa_mae_vicreg_representation:
    return "mtf_jepa_mae_vicreg_representation";
  case training::training_task_t::policy_graph_node_allocation_ppo_v0:
    return "policy_graph_node_allocation_ppo_v0";
  }
  return "unknown";
}

[[nodiscard]] inline const char *
optimizer_name(cuwacunu::jkimyei::training::training_optimizer_t optimizer) {
  namespace training = cuwacunu::jkimyei::training;
  switch (optimizer) {
  case training::training_optimizer_t::adam:
    return "adam";
  case training::training_optimizer_t::ppo_clip:
    return "ppo_clip";
  }
  return "unknown";
}

inline void append_training_contract_fields(
    std::ostringstream &out, const char *prefix,
    const cuwacunu::jkimyei::training::training_run_spec_t &training) {
  out << prefix << "_version=" << training.version_token << "\n";
  out << prefix << "_id=" << training.training_id << "\n";
  out << prefix << "_task=" << training_task_name(training.task) << "\n";
  out << prefix << "_component_assembly_id=" << training.component_assembly_id
      << "\n";
  out << prefix
      << "_no_lookahead_contract_id=" << training.no_lookahead_contract_id
      << "\n";
  out << prefix << "_no_lookahead_contract_digest="
      << training.no_lookahead_contract_digest << "\n";
  out << prefix
      << "_component_order_contract_id=" << training.component_order_contract_id
      << "\n";
  out << prefix << "_component_order_contract_digest="
      << training.component_order_contract_digest << "\n";
  out << prefix << "_component_role=" << training.component_role << "\n";
  out << prefix << "_serving_order_index=" << training.serving_order_index
      << "\n";
  out << prefix << "_optimizer=" << optimizer_name(training.optimizer) << "\n";
  out << prefix << "_learning_rate=" << training.learning_rate << "\n";
  out << prefix << "_max_steps=" << training.max_steps << "\n";
  out << prefix << "_batch_size=" << training.batch_size << "\n";
  out << prefix << "_grad_clip_norm=" << training.grad_clip_norm << "\n";
  out << prefix << "_checkpoint_every=" << training.checkpoint_every << "\n";
  out << prefix << "_report_every=" << training.report_every << "\n";
  out << prefix << "_validation_every=" << training.validation_every << "\n";
  out << prefix << "_seed=" << training.seed << "\n";
  out << prefix << "_freeze_representation=" << training.freeze_representation
      << "\n";
  out << prefix << "_training_schedule_mode=" << training.training_schedule_mode
      << "\n";
  out << prefix
      << "_require_causal_schedule=" << training.require_causal_schedule
      << "\n";
  out << prefix << "_ppo_execution_allowed=" << training.ppo_execution_allowed
      << "\n";
  out << prefix << "_checkpoint_kind=" << training.checkpoint_kind << "\n";
  out << prefix
      << "_training_visibility_policy=" << training.training_visibility_policy
      << "\n";
  out << prefix << "_generation_lane_policy=" << training.generation_lane_policy
      << "\n";
  out << prefix << "_valid_from_policy=" << training.valid_from_policy << "\n";
  out << prefix
      << "_artifact_provenance_policy=" << training.artifact_provenance_policy
      << "\n";
}

[[nodiscard]] inline cuwacunu::jkimyei::training::training_contract_defaults_t
training_contract_defaults_from_protocol(const protocol_variant_t &variant) {
  namespace training = cuwacunu::jkimyei::training;
  auto defaults = training::canonical_training_contract_defaults();
  if (protocol_no_lookahead_contract_declared(variant.no_lookahead_contract)) {
    defaults.no_lookahead_contract_id =
        variant.no_lookahead_contract.contract_id;
    defaults.no_lookahead_contract_digest =
        variant.no_lookahead_contract.contract_digest;
    defaults.component_order_contract_id =
        variant.no_lookahead_contract.contract_id;
    defaults.component_order_contract_digest =
        variant.no_lookahead_contract.contract_digest;
  }
  return defaults;
}

[[nodiscard]] inline cuwacunu::jkimyei::training::training_contract_defaults_t
training_contract_defaults_for_component(
    const protocol_variant_t &variant,
    const std::string &component_assembly_id) {
  auto defaults = training_contract_defaults_from_protocol(variant);
  defaults.component_assembly_id = component_assembly_id;
  return defaults;
}

} // namespace config_bundle_detail

[[nodiscard]] inline std::string
canonical_channel_graph_first_protocol_contract_text(
    const channel_graph_first_protocol_contract_t &contract) {
  namespace assembly = cuwacunu::wikimyei::assembly;
  namespace types = cuwacunu::ujcamei::source::registry::types;

  std::ostringstream out;
  out << "schema=kikijyeba.protocol.channel_graph_first.contract.v1\n";
  out << "protocol_id=" << contract.protocol_variant.protocol_id << "\n";
  out << "protocol_kind=" << contract.protocol_variant.protocol_kind << "\n";
  out << "protocol_graph_topology=" << contract.protocol_variant.graph_topology
      << "\n";
  out << "protocol_nodelift=" << contract.protocol_variant.nodelift_family
      << "\n";
  out << "protocol_representation="
      << protocol_representation_family_name(
             contract.protocol_variant.representation_family)
      << "\n";
  out << "protocol_inference=" << contract.protocol_variant.inference_family
      << "\n";
  out << "protocol_observer=" << contract.protocol_variant.observer_family
      << "\n";
  out << "protocol_allocation_policy="
      << contract.protocol_variant.allocation_policy_family << "\n";
  out << "protocol_policy_component="
      << contract.protocol_variant.policy_component_family << "\n";
  out << "protocol_representation_contract="
      << contract.protocol_variant.representation_contract << "\n";
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
  out << "active_representation_family="
      << active_representation_family_id(contract) << "\n";
  out << "active_representation_component_assembly_id="
      << active_representation_component_assembly_id(contract) << "\n";
  out << "active_representation_assembly="
      << assembly::assembly_fingerprint(
             active_representation_assembly(contract))
      << "\n";
  out << "vicreg_assembly="
      << assembly::assembly_fingerprint(contract.vicreg_assembly) << "\n";
  out << "mtf_jepa_mae_vicreg_assembly="
      << assembly::assembly_fingerprint(contract.mtf_jepa_mae_vicreg_assembly)
      << "\n";
  out << "channel_mdn_assembly="
      << assembly::assembly_fingerprint(contract.channel_mdn_assembly) << "\n";
  out << "belief_observer_assembly="
      << assembly::assembly_fingerprint(contract.belief_observer_assembly)
      << "\n";
  out << "spot_distributional_utility_assembly="
      << assembly::assembly_fingerprint(
             contract.spot_distributional_utility_assembly)
      << "\n";
  out << "graph_node_allocation_assembly="
      << assembly::assembly_fingerprint(contract.graph_node_allocation_assembly)
      << "\n";
  out << "dock_binding="
      << cuwacunu::kikijyeba::topology::dock_binding_fingerprint(
             contract.dock_binding)
      << "\n";
  out << "vicreg_version=" << contract.vicreg.version_token << "\n";
  out << "vicreg_component_assembly_id="
      << contract.vicreg.component_assembly_id << "\n";
  out << "vicreg_input_route="
      << config_bundle_detail::channel_input_route_name(
             contract.vicreg.input_route)
      << "\n";
  out << "vicreg_channel_count=" << contract.vicreg.channel_count << "\n";
  out << "vicreg_history_length=" << contract.vicreg.history_length << "\n";
  out << "vicreg_input_width=" << contract.vicreg.input_width << "\n";
  out << "vicreg_cell_valid_policy="
      << config_bundle_detail::cell_valid_policy_name(
             contract.vicreg.cell_valid_policy)
      << "\n";
  out << "vicreg_required_feature_coords=";
  config_bundle_detail::append_i64_list(
      out, contract.vicreg.required_feature_coords);
  out << "\n";
  out << "vicreg_min_valid_fraction=" << contract.vicreg.min_valid_fraction
      << "\n";
  out << "vicreg_use_missingness_indicators="
      << contract.vicreg.use_missingness_indicators << "\n";
  out << "vicreg_encoding_dim=" << contract.vicreg.encoding_dim << "\n";
  out << "vicreg_feature_hidden_dim=" << contract.vicreg.feature_hidden_dim
      << "\n";
  out << "vicreg_temporal_depth=" << contract.vicreg.temporal_depth << "\n";
  out << "vicreg_recency_decay=" << contract.vicreg.recency_decay << "\n";
  out << "vicreg_projector_dim=" << contract.vicreg.vicreg_projector_dim
      << "\n";
  out << "vicreg_projector_hidden_dim="
      << contract.vicreg.vicreg_projector_hidden_dim << "\n";
  out << "vicreg_projector_depth=" << contract.vicreg.vicreg_projector_depth
      << "\n";
  out << "vicreg_invariance_weight=" << contract.vicreg.vicreg_invariance_weight
      << "\n";
  out << "vicreg_variance_weight=" << contract.vicreg.vicreg_variance_weight
      << "\n";
  out << "vicreg_covariance_weight=" << contract.vicreg.vicreg_covariance_weight
      << "\n";
  out << "vicreg_variance_floor=" << contract.vicreg.vicreg_variance_floor
      << "\n";
  out << "vicreg_eps=" << contract.vicreg.vicreg_eps << "\n";
  out << "vicreg_global_aux_weight=" << contract.vicreg.global_aux_weight
      << "\n";
  out << "vicreg_min_valid_rows=" << contract.vicreg.min_valid_rows << "\n";
  out << "vicreg_skip_non_finite_loss=" << contract.vicreg.skip_non_finite_loss
      << "\n";
  out << "vicreg_jitter_std=" << contract.vicreg.jitter_std << "\n";
  out << "vicreg_feature_dropout_prob=" << contract.vicreg.feature_dropout_prob
      << "\n";
  out << "vicreg_history_dropout_prob=" << contract.vicreg.history_dropout_prob
      << "\n";
  out << "vicreg_dtype=" << contract.vicreg.dtype << "\n";
  out << "vicreg_device=" << contract.vicreg.device << "\n";
  const auto &mtf_cfg = contract.mtf_jepa_mae_vicreg.config;
  out << "mtf_jepa_mae_vicreg_version="
      << contract.mtf_jepa_mae_vicreg.version_token << "\n";
  out << "mtf_jepa_mae_vicreg_component_assembly_id="
      << contract.mtf_jepa_mae_vicreg.component_assembly_id << "\n";
  out << "mtf_jepa_mae_vicreg_channel_count=" << mtf_cfg.channel_count << "\n";
  out << "mtf_jepa_mae_vicreg_history_length=" << mtf_cfg.history_length
      << "\n";
  out << "mtf_jepa_mae_vicreg_input_width=" << mtf_cfg.input_width << "\n";
  out << "mtf_jepa_mae_vicreg_d_model=" << mtf_cfg.d_model << "\n";
  out << "mtf_jepa_mae_vicreg_latent_dim=" << mtf_cfg.latent_dim << "\n";
  out << "mtf_jepa_mae_vicreg_projector_dim=" << mtf_cfg.projector_dim << "\n";
  out << "mtf_jepa_mae_vicreg_predictor_hidden_dim="
      << mtf_cfg.predictor_hidden_dim << "\n";
  out << "mtf_jepa_mae_vicreg_num_encoder_layers=" << mtf_cfg.num_encoder_layers
      << "\n";
  out << "mtf_jepa_mae_vicreg_num_predictor_layers="
      << mtf_cfg.num_predictor_layers << "\n";
  out << "mtf_jepa_mae_vicreg_num_decoder_layers=" << mtf_cfg.num_decoder_layers
      << "\n";
  out << "mtf_jepa_mae_vicreg_num_heads=" << mtf_cfg.num_heads << "\n";
  out << "mtf_jepa_mae_vicreg_dropout=" << mtf_cfg.dropout << "\n";
  out << "mtf_jepa_mae_vicreg_time_scales=";
  config_bundle_detail::append_i64_list(out, mtf_cfg.time_scales);
  out << "\n";
  out << "mtf_jepa_mae_vicreg_scale_strides=";
  config_bundle_detail::append_i64_list(out, mtf_cfg.scale_strides);
  out << "\n";
  out << "mtf_jepa_mae_vicreg_frequency_num_bins=" << mtf_cfg.frequency_num_bins
      << "\n";
  out << "mtf_jepa_mae_vicreg_mask_ratio_time=" << mtf_cfg.mask_ratio_time
      << "\n";
  out << "mtf_jepa_mae_vicreg_mask_ratio_frequency="
      << mtf_cfg.mask_ratio_frequency << "\n";
  out << "mtf_jepa_mae_vicreg_mask_ratio_channel=" << mtf_cfg.mask_ratio_channel
      << "\n";
  out << "mtf_jepa_mae_vicreg_min_context_ratio=" << mtf_cfg.min_context_ratio
      << "\n";
  out << "mtf_jepa_mae_vicreg_lambda_jepa=" << mtf_cfg.lambda_jepa << "\n";
  out << "mtf_jepa_mae_vicreg_lambda_mae=" << mtf_cfg.lambda_mae << "\n";
  out << "mtf_jepa_mae_vicreg_lambda_tf_align=" << mtf_cfg.lambda_tf_align
      << "\n";
  out << "mtf_jepa_mae_vicreg_lambda_vicreg=" << mtf_cfg.lambda_vicreg << "\n";
  out << "mtf_jepa_mae_vicreg_target_ema_tau=" << mtf_cfg.target_ema_tau
      << "\n";
  out << "channel_mdn_version=" << contract.channel_mdn.version_token << "\n";
  out << "channel_mdn_component_assembly_id="
      << contract.channel_mdn.component_assembly_id << "\n";
  out << "channel_mdn_input_representation_assembly_id="
      << contract.channel_mdn.input_representation_assembly_id << "\n";
  out << "channel_mdn_context_mode="
      << config_bundle_detail::channel_context_mode_name(
             contract.channel_mdn.context_mode)
      << "\n";
  out << "channel_mdn_target_domain="
      << config_bundle_detail::channel_target_domain_name(
             contract.channel_mdn.target_domain)
      << "\n";
  out << "channel_mdn_target_coords=";
  for (std::size_t i = 0; i < contract.channel_mdn.target_coords.size(); ++i) {
    if (i != 0)
      out << ",";
    out << contract.channel_mdn.target_coords.at(i);
  }
  out << "\n";
  out << "channel_mdn_target_mask_policy="
      << config_bundle_detail::channel_target_mask_policy_name(
             contract.channel_mdn.target_mask_policy)
      << "\n";
  out << "channel_mdn_activity_target="
      << config_bundle_detail::channel_activity_target_name(
             contract.channel_mdn.activity_target)
      << "\n";
  out << "channel_mdn_channel_count=" << contract.channel_mdn.channel_count
      << "\n";
  out << "channel_mdn_future_horizon=" << contract.channel_mdn.future_horizon
      << "\n";
  out << "channel_mdn_mixture_count=" << contract.channel_mdn.mixture_count
      << "\n";
  out << "channel_mdn_hidden_width=" << contract.channel_mdn.hidden_width
      << "\n";
  out << "channel_mdn_residual_depth=" << contract.channel_mdn.residual_depth
      << "\n";
  out << "channel_mdn_feature_embedding_dim="
      << contract.channel_mdn.feature_embedding_dim << "\n";
  out << "channel_mdn_channel_adapter_rank="
      << contract.channel_mdn.channel_adapter_rank << "\n";
  out << "channel_mdn_global_context_dim="
      << contract.channel_mdn.global_context_dim << "\n";
  out << "channel_mdn_sigma_min=" << contract.channel_mdn.sigma_min << "\n";
  out << "channel_mdn_sigma_max=" << contract.channel_mdn.sigma_max << "\n";
  out << "channel_mdn_eps=" << contract.channel_mdn.eps << "\n";
  out << "belief_observer_version=" << contract.belief_observer.version_token
      << "\n";
  out << "belief_observer_component_assembly_id="
      << contract.belief_observer.component_assembly_id << "\n";
  out << "belief_observer_input_mdn_assembly_id="
      << contract.belief_observer.input_mdn_assembly_id << "\n";
  out << "belief_observer_feature_semantics="
      << contract.belief_observer.feature_semantics << "\n";
  out << "belief_observer_feature_semantics_source="
      << contract.belief_observer.feature_semantics_source << "\n";
  out << "belief_observer_graph_node_axis_policy="
      << contract.belief_observer.graph_node_axis_policy << "\n";
  out << "belief_observer_batch_policy="
      << contract.belief_observer.batch_policy << "\n";
  out << "belief_observer_channel_consensus="
      << contract.belief_observer.channel_consensus << "\n";
  out << "belief_observer_potential_semantics="
      << contract.belief_observer.potential_semantics << "\n";
  out << "belief_observer_return_projection="
      << contract.belief_observer.return_projection << "\n";
  out << "belief_observer_scenario_unit="
      << contract.belief_observer.scenario_unit << "\n";
  out << "belief_observer_accounting_numeraire_policy="
      << contract.belief_observer.accounting_numeraire_policy << "\n";
  out << "belief_observer_covariance_coupler="
      << contract.belief_observer.covariance_coupler << "\n";
  out << "belief_observer_scenario_count="
      << contract.belief_observer.scenario_count << "\n";
  out << "belief_observer_projection_validation_required="
      << contract.belief_observer.projection_validation_required << "\n";
  out << "belief_observer_live_capital_allowed="
      << contract.belief_observer.live_capital_allowed << "\n";
  out << "belief_observer_eps=" << contract.belief_observer.eps << "\n";
  out << "spot_distributional_utility_version="
      << contract.spot_distributional_utility.version_token << "\n";
  out << "spot_distributional_utility_component_assembly_id="
      << contract.spot_distributional_utility.component_assembly_id << "\n";
  out << "spot_distributional_utility_input_belief_assembly_id="
      << contract.spot_distributional_utility.input_belief_assembly_id << "\n";
  out << "spot_distributional_utility_optimizer="
      << contract.spot_distributional_utility.optimizer << "\n";
  out << "spot_distributional_utility_objective="
      << contract.spot_distributional_utility.objective << "\n";
  out << "spot_distributional_utility_scenario_unit="
      << contract.spot_distributional_utility.scenario_unit << "\n";
  out << "spot_distributional_utility_iterations="
      << contract.spot_distributional_utility.iterations << "\n";
  out << "spot_distributional_utility_learning_rate="
      << contract.spot_distributional_utility.learning_rate << "\n";
  out << "spot_distributional_utility_cvar_alpha="
      << contract.spot_distributional_utility.cvar_alpha << "\n";
  out << "spot_distributional_utility_scenario_growth_floor="
      << contract.spot_distributional_utility.scenario_growth_floor << "\n";
  out << "spot_distributional_utility_long_only="
      << contract.spot_distributional_utility.long_only << "\n";
  out << "spot_distributional_utility_spot_only="
      << contract.spot_distributional_utility.spot_only << "\n";
  out << "spot_distributional_utility_require_accounting_numeraire_node="
      << contract.spot_distributional_utility.require_accounting_numeraire_node
      << "\n";
  out << "spot_distributional_utility_projection_validation_required="
      << contract.spot_distributional_utility.projection_validation_required
      << "\n";
  out << "spot_distributional_utility_live_capital_allowed="
      << contract.spot_distributional_utility.live_capital_allowed << "\n";
  out << "graph_node_allocation_version="
      << contract.graph_node_allocation.version_token << "\n";
  out << "graph_node_allocation_component_assembly_id="
      << contract.graph_node_allocation.component_assembly_id << "\n";
  out << "graph_node_allocation_policy_kind="
      << contract.graph_node_allocation.policy_kind << "\n";
  out << "graph_node_allocation_policy_input_schema="
      << contract.graph_node_allocation.policy_input_schema << "\n";
  out << "graph_node_allocation_action_adapter="
      << contract.graph_node_allocation.action_adapter << "\n";
  out << "graph_node_allocation_action_distribution="
      << contract.graph_node_allocation.action_distribution << "\n";
  out << "graph_node_allocation_action_schema="
      << contract.graph_node_allocation.action_schema << "\n";
  out << "graph_node_allocation_reward_contract="
      << contract.graph_node_allocation.reward_contract << "\n";
  out << "graph_node_allocation_graph_node_universe_policy="
      << contract.graph_node_allocation.graph_node_universe_policy << "\n";
  out << "graph_node_allocation_scenario_input_policy="
      << contract.graph_node_allocation.scenario_input_policy << "\n";
  out << "graph_node_allocation_raw_mdn_input_allowed="
      << contract.graph_node_allocation.raw_mdn_input_allowed << "\n";
  out << "graph_node_allocation_ppo_implemented="
      << contract.graph_node_allocation.ppo_implemented << "\n";
  out << "graph_node_allocation_live_capital_allowed="
      << contract.graph_node_allocation.live_capital_allowed << "\n";
  out << "graph_node_allocation_net_version="
      << contract.graph_node_allocation_net.version_token << "\n";
  out << "graph_node_allocation_net_policy_input_schema="
      << contract.graph_node_allocation_net.policy_input_schema << "\n";
  out << "graph_node_allocation_net_policy_input_feature_manifest="
      << contract.graph_node_allocation_net.policy_input_feature_manifest
      << "\n";
  out << "graph_node_allocation_net_input_node_feature_dim="
      << contract.graph_node_allocation_net.input_node_feature_dim << "\n";
  out << "graph_node_allocation_net_input_global_feature_dim="
      << contract.graph_node_allocation_net.input_global_feature_dim << "\n";
  out << "graph_node_allocation_net_input_risk_feature_dim="
      << contract.graph_node_allocation_net.input_risk_feature_dim << "\n";
  out << "graph_node_allocation_net_node_encoder_layers="
      << contract.graph_node_allocation_net.node_encoder_layers << "\n";
  out << "graph_node_allocation_net_node_encoder_hidden_dim="
      << contract.graph_node_allocation_net.node_encoder_hidden_dim << "\n";
  out << "graph_node_allocation_net_global_encoder_layers="
      << contract.graph_node_allocation_net.global_encoder_layers << "\n";
  out << "graph_node_allocation_net_global_encoder_hidden_dim="
      << contract.graph_node_allocation_net.global_encoder_hidden_dim << "\n";
  out << "graph_node_allocation_net_risk_encoder_layers="
      << contract.graph_node_allocation_net.risk_encoder_layers << "\n";
  out << "graph_node_allocation_net_risk_encoder_hidden_dim="
      << contract.graph_node_allocation_net.risk_encoder_hidden_dim << "\n";
  out << "graph_node_allocation_net_pooling="
      << contract.graph_node_allocation_net.pooling << "\n";
  out << "graph_node_allocation_net_fusion_hidden_dim="
      << contract.graph_node_allocation_net.fusion_hidden_dim << "\n";
  out << "graph_node_allocation_net_fusion_layers="
      << contract.graph_node_allocation_net.fusion_layers << "\n";
  out << "graph_node_allocation_net_activation="
      << contract.graph_node_allocation_net.activation << "\n";
  out << "graph_node_allocation_net_normalization="
      << contract.graph_node_allocation_net.normalization << "\n";
  out << "graph_node_allocation_net_dropout="
      << contract.graph_node_allocation_net.dropout << "\n";
  out << "graph_node_allocation_net_share_encoder="
      << contract.graph_node_allocation_net.share_encoder << "\n";
  out << "graph_node_allocation_net_separate_actor_critic_heads="
      << contract.graph_node_allocation_net.separate_actor_critic_heads << "\n";
  out << "graph_node_allocation_net_policy_head_hidden_dim="
      << contract.graph_node_allocation_net.policy_head_hidden_dim << "\n";
  out << "graph_node_allocation_net_value_head_hidden_dim="
      << contract.graph_node_allocation_net.value_head_hidden_dim << "\n";
  out << "graph_node_allocation_net_output_head="
      << contract.graph_node_allocation_net.output_head << "\n";
  out << "graph_node_allocation_net_value_head="
      << contract.graph_node_allocation_net.value_head << "\n";
  out << "graph_node_allocation_net_action_adapter="
      << contract.graph_node_allocation_net.action_adapter << "\n";
  out << "graph_node_allocation_net_action_distribution="
      << contract.graph_node_allocation_net.action_distribution << "\n";
  out << "graph_node_allocation_net_dirichlet_concentration_head="
      << contract.graph_node_allocation_net.dirichlet_concentration_head
      << "\n";
  out << "graph_node_allocation_net_dirichlet_alpha_floor="
      << contract.graph_node_allocation_net.dirichlet_alpha_floor << "\n";
  out << "graph_node_allocation_net_dirichlet_total_concentration_min="
      << contract.graph_node_allocation_net.dirichlet_total_concentration_min
      << "\n";
  out << "graph_node_allocation_net_dirichlet_total_concentration_max="
      << contract.graph_node_allocation_net.dirichlet_total_concentration_max
      << "\n";
  out << "graph_node_allocation_net_logistic_normal_candidate="
      << contract.graph_node_allocation_net.logistic_normal_candidate << "\n";
  out << "graph_node_allocation_net_logistic_normal_std_head="
      << contract.graph_node_allocation_net.logistic_normal_std_head << "\n";
  out << "graph_node_allocation_net_logistic_normal_log_std_min="
      << contract.graph_node_allocation_net.logistic_normal_log_std_min << "\n";
  out << "graph_node_allocation_net_logistic_normal_log_std_max="
      << contract.graph_node_allocation_net.logistic_normal_log_std_max << "\n";
  out << "graph_node_allocation_net_logistic_normal_coordinate_policy="
      << contract.graph_node_allocation_net.logistic_normal_coordinate_policy
      << "\n";
  out << "graph_node_allocation_net_logistic_normal_entropy_kind="
      << contract.graph_node_allocation_net.logistic_normal_entropy_kind
      << "\n";
  out << "graph_node_allocation_net_ppo_execution_allowed="
      << contract.graph_node_allocation_net.ppo_execution_allowed << "\n";
  out << "graph_node_allocation_features_version="
      << contract.graph_node_allocation_features.version_token << "\n";
  out << "graph_node_allocation_features_node_feature_dim="
      << contract.graph_node_allocation_features.node_feature_dim << "\n";
  out << "graph_node_allocation_features_node_feature_names=";
  config_bundle_detail::append_string_list(
      out, contract.graph_node_allocation_features.node_feature_names);
  out << "\n";
  out << "graph_node_allocation_features_global_feature_dim="
      << contract.graph_node_allocation_features.global_feature_dim << "\n";
  out << "graph_node_allocation_features_global_feature_names=";
  config_bundle_detail::append_string_list(
      out, contract.graph_node_allocation_features.global_feature_names);
  out << "\n";
  out << "graph_node_allocation_features_risk_feature_block="
      << contract.graph_node_allocation_features.risk_feature_block << "\n";
  out << "graph_node_allocation_features_risk_feature_dim="
      << contract.graph_node_allocation_features.risk_feature_dim << "\n";
  out << "graph_node_allocation_features_risk_feature_names=";
  config_bundle_detail::append_string_list(
      out, contract.graph_node_allocation_features.risk_feature_names);
  out << "\n";
  out << "graph_node_allocation_features_raw_mdn_input_allowed="
      << contract.graph_node_allocation_features.raw_mdn_input_allowed << "\n";
  out << "graph_node_allocation_features_full_scenario_bank_input_allowed="
      << contract.graph_node_allocation_features
             .full_scenario_bank_input_allowed
      << "\n";
  out << "replay_environment_version="
      << contract.replay_environment.version_token << "\n";
  out << "replay_environment_component_assembly_id="
      << contract.replay_environment.component_assembly_id << "\n";
  out << "replay_environment_world_mode="
      << contract.replay_environment.world_mode << "\n";
  out << "replay_environment_api_contract="
      << contract.replay_environment.api_contract << "\n";
  out << "replay_environment_spawn_model="
      << contract.replay_environment.spawn_model << "\n";
  out << "replay_environment_range_source="
      << contract.replay_environment.range_source << "\n";
  out << "replay_environment_source_range_policy="
      << contract.replay_environment.source_range_policy << "\n";
  out << "replay_environment_source_order_policy="
      << contract.replay_environment.source_order_policy << "\n";
  out << "replay_environment_range_resolution="
      << contract.replay_environment.range_resolution << "\n";
  out << "replay_environment_observation_time_law="
      << contract.replay_environment.observation_time_law << "\n";
  out << "replay_environment_realization_reveal="
      << contract.replay_environment.realization_reveal << "\n";
  out << "replay_environment_realization_key_policy="
      << contract.replay_environment.realization_key_policy << "\n";
  out << "replay_environment_action_kind="
      << contract.replay_environment.action_kind << "\n";
  out << "replay_environment_action_time_policy="
      << contract.replay_environment.action_time_policy << "\n";
  out << "replay_environment_graph_node_universe_policy="
      << contract.replay_environment.graph_node_universe_policy << "\n";
  out << "replay_environment_reward_policy="
      << contract.replay_environment.reward_policy << "\n";
  out << "replay_environment_projection_validation="
      << contract.replay_environment.projection_validation << "\n";
  out << "replay_environment_policy_surface="
      << contract.replay_environment.policy_surface << "\n";
  out << "replay_environment_action_policy_identity="
      << contract.replay_environment.action_policy_identity << "\n";
  out << "replay_environment_initial_policy_kind="
      << contract.replay_environment.initial_policy_kind << "\n";
  out << "replay_environment_experiment_task_identity="
      << contract.replay_environment.experiment_task_identity << "\n";
  out << "replay_environment_experiment_run_identity="
      << contract.replay_environment.experiment_run_identity << "\n";
  out << "replay_environment_step_artifact_identity="
      << contract.replay_environment.step_artifact_identity << "\n";
  out << "replay_environment_experiment_report_count_policy="
      << contract.replay_environment.experiment_report_count_policy << "\n";
  out << "replay_environment_artifact_schema="
      << contract.replay_environment.artifact_schema << "\n";
  out << "replay_environment_require_resolved_cursor="
      << contract.replay_environment.require_resolved_cursor << "\n";
  out << "replay_environment_require_no_future_leakage="
      << contract.replay_environment.require_no_future_leakage << "\n";
  out << "replay_environment_require_projection_validation="
      << contract.replay_environment.require_projection_validation << "\n";
  out << "replay_environment_replay_executor="
      << contract.replay_environment.replay_executor << "\n";
  out << "replay_environment_allocation_authority="
      << contract.replay_environment.allocation_authority << "\n";
  out << "replay_environment_execution_authority="
      << contract.replay_environment.execution_authority << "\n";
  out << "replay_environment_live_capital_allowed="
      << contract.replay_environment.live_capital_allowed << "\n";
  out << "replay_environment_default_max_parallel_jobs="
      << contract.replay_environment.default_max_parallel_jobs << "\n";
  config_bundle_detail::append_training_contract_fields(
      out, "vicreg_training", contract.vicreg_training);
  config_bundle_detail::append_training_contract_fields(
      out, "mtf_jepa_mae_vicreg_training",
      contract.mtf_jepa_mae_vicreg_training);
  config_bundle_detail::append_training_contract_fields(
      out, "channel_mdn_training", contract.channel_mdn_training);
  config_bundle_detail::append_training_contract_fields(
      out, "graph_node_allocation_training",
      contract.graph_node_allocation_training);
  return out.str();
}

[[nodiscard]] inline std::string
channel_graph_first_protocol_contract_fingerprint(
    const channel_graph_first_protocol_contract_t &contract) {
  std::uint64_t hash =
      cuwacunu::wikimyei::assembly::assembly_detail::kFnvOffsetBasis;
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, "cuwacunu.kikijyeba.protocol.channel_graph_first.contract.v1");
  cuwacunu::wikimyei::assembly::assembly_detail::mix_hash_string(
      hash, canonical_channel_graph_first_protocol_contract_text(contract));
  return cuwacunu::wikimyei::assembly::assembly_detail::hash_hex(hash);
}

[[nodiscard]] inline std::string channel_graph_first_protocol_contract_token(
    const channel_graph_first_protocol_contract_t &contract) {
  return std::string("kikijyeba.protocol.channel_graph_first.contract.v1/") +
         channel_graph_first_protocol_contract_fingerprint(contract);
}

[[nodiscard]] inline cuwacunu::kikijyeba::topology::
    dock_binding_validation_report_t
    make_channel_graph_first_dock_binding_report(
        const channel_graph_first_config_bundle_t &bundle) {
  return cuwacunu::kikijyeba::topology::analyze_dock_binding_against_assemblies(
      bundle.dock_binding, channel_graph_first_wikimyei_assemblies(bundle));
}

inline void validate_channel_graph_first_dock_binding(
    const channel_graph_first_config_bundle_t &bundle) {
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
  topo::require_static_i64_binding_value(
      bundle.dock_binding, "De", active_representation_encoding_dim(bundle),
      "active representation encoding dimension");
  topo::require_static_i64_binding_value(
      bundle.dock_binding, "Df",
      static_cast<std::int64_t>(bundle.channel_mdn.target_coords.size()),
      "channel MDN target coordinate count");
  topo::require_static_i64_binding_value(bundle.dock_binding, "K",
                                         bundle.channel_mdn.mixture_count,
                                         "channel MDN mixture count");
  topo::require_static_i64_binding_value(
      bundle.dock_binding, "G",
      static_cast<std::int64_t>(
          bundle.source_plan.market_graph.node_ids.size()),
      "graph node count for NodeLift potential belief");
  topo::require_static_i64_binding_value(
      bundle.dock_binding, "Kc",
      active_channel_count(bundle.source_dock) *
          bundle.channel_mdn.mixture_count,
      "channel-consensus mixture component count");
  topo::require_static_i64_binding_value(bundle.dock_binding, "S",
                                         bundle.belief_observer.scenario_count,
                                         "observer scenario count");
  topo::require_runtime_binding(bundle.dock_binding, "A",
                                "allocatable risky graph-node count");
  (void)make_channel_graph_first_dock_binding_report(bundle);
}

inline void validate_channel_graph_first_protocol_contract(
    const channel_graph_first_protocol_contract_t &bundle) {
  namespace mdn = cuwacunu::wikimyei::inference::expected_value::mdn;
  namespace mtf =
      cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;
  namespace vicreg = cuwacunu::wikimyei::representation::encoding::vicreg;
  namespace nodelift = cuwacunu::wikimyei::expression::nodelift::srl;
  namespace observer_belief = cuwacunu::wikimyei::observer::belief;
  namespace spot_policy =
      cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility;
  namespace graph_allocation_policy =
      cuwacunu::wikimyei::policy::portfolio::graph_node_allocation;
  namespace replay_env = cuwacunu::kikijyeba::environment;
  namespace training = cuwacunu::jkimyei::training;

  validate_protocol_variant(bundle.protocol_variant);
  nodelift::validate_nodelift_srl_spec(bundle.nodelift);
  vicreg::validate_vicreg_spec(bundle.vicreg);
  mtf::validate_mtf_jepa_mae_vicreg_spec(bundle.mtf_jepa_mae_vicreg);
  mdn::validate_channel_mdn_spec(bundle.channel_mdn);
  observer_belief::validate_belief_observer_spec(bundle.belief_observer);
  spot_policy::validate_spot_distributional_utility_spec(
      bundle.spot_distributional_utility);
  graph_allocation_policy::validate_graph_node_allocation_spec(
      bundle.graph_node_allocation);
  graph_allocation_policy::validate_graph_node_allocation_net_spec(
      bundle.graph_node_allocation_net);
  graph_allocation_policy::validate_graph_node_allocation_feature_manifest_spec(
      bundle.graph_node_allocation_features);
  if (bundle.graph_node_allocation.action_distribution !=
      bundle.graph_node_allocation_net.action_distribution) {
    throw std::runtime_error(
        "[channel_graph_first_config] graph-node allocation action "
        "distribution mismatch");
  }
  if (bundle.graph_node_allocation_net.policy_input_feature_manifest !=
      bundle.graph_node_allocation_features.version_token) {
    throw std::runtime_error("[channel_graph_first_config] graph-node "
                             "allocation net feature manifest mismatch");
  }
  if (bundle.graph_node_allocation_net.input_node_feature_dim !=
          bundle.graph_node_allocation_features.node_feature_dim ||
      bundle.graph_node_allocation_net.input_global_feature_dim !=
          bundle.graph_node_allocation_features.global_feature_dim ||
      bundle.graph_node_allocation_net.input_risk_feature_dim !=
          bundle.graph_node_allocation_features.risk_feature_dim) {
    throw std::runtime_error("[channel_graph_first_config] graph-node "
                             "allocation net/feature dimensions mismatch");
  }
  replay_env::validate_replay_environment_spec(bundle.replay_environment);
  if (bundle.nodelift_assembly.component_assembly_id !=
          bundle.nodelift.component_assembly_id ||
      bundle.nodelift_assembly.version_token != bundle.nodelift.version_token) {
    throw std::runtime_error(
        "[channel_graph_first_config] NodeLift assembly does not match "
        "NodeLift spec");
  }
  if (bundle.vicreg_assembly.component_assembly_id !=
          bundle.vicreg.component_assembly_id ||
      bundle.vicreg_assembly.version_token != bundle.vicreg.version_token) {
    throw std::runtime_error(
        "[channel_graph_first_config] VICReg assembly does not match "
        "VICReg spec");
  }
  if (bundle.mtf_jepa_mae_vicreg_assembly.component_assembly_id !=
          bundle.mtf_jepa_mae_vicreg.component_assembly_id ||
      bundle.mtf_jepa_mae_vicreg_assembly.version_token !=
          bundle.mtf_jepa_mae_vicreg.version_token) {
    throw std::runtime_error(
        "[channel_graph_first_config] MTF-JEPA-MAE-VICReg assembly does not "
        "match MTF spec");
  }
  if (bundle.channel_mdn_assembly.component_assembly_id !=
          bundle.channel_mdn.component_assembly_id ||
      bundle.channel_mdn_assembly.version_token !=
          bundle.channel_mdn.version_token) {
    throw std::runtime_error(
        "[channel_graph_first_config] Channel MDN assembly does not match "
        "Channel MDN spec");
  }
  if (bundle.belief_observer_assembly.component_assembly_id !=
          bundle.belief_observer.component_assembly_id ||
      bundle.belief_observer_assembly.version_token !=
          bundle.belief_observer.version_token) {
    throw std::runtime_error(
        "[channel_graph_first_config] belief observer assembly does not match "
        "belief observer spec");
  }
  if (bundle.spot_distributional_utility_assembly.component_assembly_id !=
          bundle.spot_distributional_utility.component_assembly_id ||
      bundle.spot_distributional_utility_assembly.version_token !=
          bundle.spot_distributional_utility.version_token) {
    throw std::runtime_error(
        "[channel_graph_first_config] spot distributional utility assembly "
        "does not match policy spec");
  }
  if (bundle.graph_node_allocation_assembly.component_assembly_id !=
          bundle.graph_node_allocation.component_assembly_id ||
      bundle.graph_node_allocation_assembly.version_token !=
          bundle.graph_node_allocation.version_token) {
    throw std::runtime_error("[channel_graph_first_config] graph node "
                             "allocation policy assembly does "
                             "not match policy spec");
  }
  if (bundle.belief_observer.input_mdn_assembly_id !=
      bundle.channel_mdn.component_assembly_id) {
    throw std::runtime_error(
        "[channel_graph_first_config] belief observer input MDN id does not "
        "match Channel MDN component id");
  }
  if (bundle.spot_distributional_utility.input_belief_assembly_id !=
      bundle.belief_observer.component_assembly_id) {
    throw std::runtime_error(
        "[channel_graph_first_config] spot distributional utility input "
        "belief id does not match belief observer component id");
  }
  if (!active_protocol_uses_mtf_jepa_mae_vicreg(bundle)) {
    cuwacunu::kikijyeba::topology::validate_node_value_assembly_chain(
        bundle.nodelift_assembly, bundle.vicreg_assembly,
        bundle.channel_mdn_assembly);
  } else {
    cuwacunu::wikimyei::assembly::validate_wikimyei_assembly(
        bundle.nodelift_assembly);
    cuwacunu::wikimyei::assembly::validate_wikimyei_assembly(
        bundle.mtf_jepa_mae_vicreg_assembly);
    cuwacunu::wikimyei::assembly::validate_wikimyei_assembly(
        bundle.channel_mdn_assembly);
  }
  const auto require_dock =
      [](const auto &assembly, const std::string &dock_name,
         cuwacunu::wikimyei::assembly::dock_direction_t direction)
      -> const cuwacunu::wikimyei::assembly::dock_t & {
    for (const auto &dock : assembly.docks) {
      if (dock.name == dock_name && dock.direction == direction) {
        return dock;
      }
    }
    throw std::runtime_error("[channel_graph_first_config] assembly " +
                             assembly.component_assembly_id +
                             " is missing dock: " + dock_name);
  };
  const auto &mdn_output = require_dock(
      bundle.channel_mdn_assembly, "future_channel_node_distribution",
      cuwacunu::wikimyei::assembly::dock_direction_t::produces);
  const auto &belief_input = require_dock(
      bundle.belief_observer_assembly, "future_channel_node_distribution",
      cuwacunu::wikimyei::assembly::dock_direction_t::consumes);
  if (!cuwacunu::wikimyei::assembly::dock_domain_compatible(mdn_output,
                                                            belief_input)) {
    throw std::runtime_error(
        "[channel_graph_first_config] Channel MDN output dock is not "
        "compatible with belief observer input dock");
  }
  const auto &belief_output =
      require_dock(bundle.belief_observer_assembly, "allocation_belief",
                   cuwacunu::wikimyei::assembly::dock_direction_t::produces);
  const auto &engine_input = require_dock(
      bundle.spot_distributional_utility_assembly, "allocation_belief",
      cuwacunu::wikimyei::assembly::dock_direction_t::consumes);
  if (!cuwacunu::wikimyei::assembly::dock_domain_compatible(belief_output,
                                                            engine_input)) {
    throw std::runtime_error(
        "[channel_graph_first_config] belief observer output dock is not "
        "compatible with spot distributional utility input dock");
  }
  if (bundle.wikimyei_registry.assemblies.size() != 7) {
    throw std::runtime_error(
        "[channel_graph_first_config] expected seven Wikimyei assemblies");
  }
  if (cuwacunu::piaabo::parse::simple_kv::lowercase(
          cuwacunu::piaabo::parse::simple_kv::trim(
              bundle.protocol_variant.protocol_status)) != "legacy" &&
      !protocol_no_lookahead_contract_declared(
          bundle.protocol_variant.no_lookahead_contract)) {
    throw std::runtime_error("[channel_graph_first_config] non-legacy "
                             "protocol must define NO_LOOKAHEAD_CONTRACT");
  }
  training::validate_training_run_spec(
      bundle.vicreg_training,
      config_bundle_detail::training_contract_defaults_for_component(
          bundle.protocol_variant, bundle.vicreg.component_assembly_id));
  training::validate_training_run_spec(
      bundle.mtf_jepa_mae_vicreg_training,
      config_bundle_detail::training_contract_defaults_for_component(
          bundle.protocol_variant,
          bundle.mtf_jepa_mae_vicreg.component_assembly_id));
  training::validate_training_run_spec(
      bundle.channel_mdn_training,
      config_bundle_detail::training_contract_defaults_for_component(
          bundle.protocol_variant, bundle.channel_mdn.component_assembly_id));
  training::validate_training_run_spec(
      bundle.graph_node_allocation_training,
      config_bundle_detail::training_contract_defaults_for_component(
          bundle.protocol_variant,
          bundle.graph_node_allocation.component_assembly_id));
  cuwacunu::hero::runtime::settings::validate_wave_settings(
      bundle.wave_settings);
  if (bundle.source_universe.empty()) {
    throw std::runtime_error(
        "[channel_graph_first_config] Ujcamei source universe is empty");
  }
  if (bundle.source_dock.empty()) {
    throw std::runtime_error(
        "[channel_graph_first_config] source dock is empty");
  }
  if (bundle.source_plan.compat_source_spec.source_forms.empty()) {
    throw std::runtime_error(
        "[channel_graph_first_config] resolved source plan is empty");
  }
  validate_channel_graph_first_dock_binding(bundle);

  const auto channel_count = active_channel_count(bundle.source_dock);
  const auto input_length = max_input_length(bundle.source_dock);
  const auto future_length = max_future_length(bundle.source_dock);
  const bool active_mtf = active_protocol_uses_mtf_jepa_mae_vicreg(bundle);
  if (!cuwacunu::hero::runtime::settings::wave_supports_protocol(
          bundle.wave_settings, bundle.protocol_variant.protocol_id)) {
    throw std::runtime_error("[channel_graph_first_config] active protocol " +
                             bundle.protocol_variant.protocol_id +
                             " does not match wave PROTOCOL " +
                             bundle.wave_settings.protocol_id);
  }
  if (bundle.wave_settings.target == cuwacunu::hero::runtime::settings::
                                         wave_target_t::vicreg_representation &&
      active_mtf) {
    throw std::runtime_error(
        "[channel_graph_first_config] active protocol docks "
        "MTF-JEPA-MAE-VICReg but active wave targets VICReg");
  }
  if (bundle.wave_settings.target ==
          cuwacunu::hero::runtime::settings::wave_target_t::
              mtf_jepa_mae_vicreg_representation &&
      !active_mtf) {
    throw std::runtime_error(
        "[channel_graph_first_config] active protocol docks VICReg but active "
        "wave targets MTF-JEPA-MAE-VICReg");
  }
  const auto active_representation_channel_count =
      active_mtf ? bundle.mtf_jepa_mae_vicreg.config.channel_count
                 : bundle.vicreg.channel_count;
  const auto active_representation_history_length =
      active_mtf ? bundle.mtf_jepa_mae_vicreg.config.history_length
                 : bundle.vicreg.history_length;
  if (active_representation_channel_count != channel_count ||
      bundle.channel_mdn.channel_count != channel_count) {
    throw std::runtime_error(
        "[channel_graph_first_config] channel counts do not match active "
        "source channels");
  }
  if (active_representation_history_length != input_length) {
    throw std::runtime_error(
        "[channel_graph_first_config] representation history length does not "
        "match source input length");
  }
  if (bundle.channel_mdn.future_horizon != future_length) {
    throw std::runtime_error(
        "[channel_graph_first_config] channel MDN future horizon does not "
        "match source future length");
  }
  if (!active_protocol_uses_mtf_jepa_mae_vicreg(bundle) &&
      bundle.channel_mdn.input_representation_assembly_id !=
          bundle.vicreg.component_assembly_id) {
    throw std::runtime_error(
        "[channel_graph_first_config] channel MDN input representation id does "
        "not match VICReg component id");
  }
  if (bundle.vicreg_training.task !=
      training::training_task_t::vicreg_representation) {
    throw std::runtime_error(
        "[channel_graph_first_config] VICReg training spec must use "
        "vicreg_representation task");
  }
  if (bundle.vicreg_training.component_assembly_id !=
      bundle.vicreg.component_assembly_id) {
    throw std::runtime_error(
        "[channel_graph_first_config] VICReg training component id "
        "does not match VICReg component id");
  }
  if (bundle.channel_mdn_training.task !=
      training::training_task_t::mdn_expected_value_inference) {
    throw std::runtime_error(
        "[channel_graph_first_config] Channel MDN training spec must use "
        "mdn_expected_value_inference task");
  }
  if (bundle.channel_mdn_training.component_assembly_id !=
      bundle.channel_mdn.component_assembly_id) {
    throw std::runtime_error(
        "[channel_graph_first_config] Channel MDN training component id does "
        "not match Channel MDN component id");
  }
  if (bundle.graph_node_allocation_training.task !=
      training::training_task_t::policy_graph_node_allocation_ppo_v0) {
    throw std::runtime_error(
        "[channel_graph_first_config] graph-node allocation policy training "
        "spec must use policy_graph_node_allocation_ppo_v0 task");
  }
  if (bundle.graph_node_allocation_training.component_assembly_id !=
      bundle.graph_node_allocation.component_assembly_id) {
    throw std::runtime_error(
        "[channel_graph_first_config] graph-node allocation policy training "
        "component id "
        "does not match graph-node allocation policy component id");
  }
}

inline void validate_channel_graph_first_config_bundle(
    const channel_graph_first_config_bundle_t &bundle) {
  validate_channel_graph_first_protocol_contract(bundle);
}

namespace graph_first_config_detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline bool ends_with(std::string_view text,
                                    std::string_view suffix) {
  return text.size() >= suffix.size() &&
         text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

[[nodiscard]] inline std::string
resolve_config_relative_path(const std::string &config_path,
                             const std::string &raw_path) {
  std::filesystem::path path(kv::trim(raw_path));
  if (path.empty() || path.is_absolute()) {
    return path.string();
  }
  return (std::filesystem::path(config_path).parent_path() / path)
      .lexically_normal()
      .string();
}

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
    if (ends_with(key, "_path")) {
      value = resolve_config_relative_path(config_path, value);
    }
    out[std::move(key)] = std::move(value);
  }
  return out;
}

[[nodiscard]] inline std::string
required_section_config_value(const std::string &config_path,
                              const std::string &section,
                              const std::string &key) {
  std::istringstream lines(read_text_file_or_throw(config_path));
  std::string current_section;
  std::string line;
  std::size_t line_number = 0;
  while (std::getline(lines, line)) {
    ++line_number;
    const auto comment = line.find('#');
    if (comment != std::string::npos) {
      line.resize(comment);
    }
    line = kv::trim(std::move(line));
    if (line.empty()) {
      continue;
    }
    if (line.front() == '[' && line.back() == ']') {
      current_section = kv::trim(line.substr(1, line.size() - 2));
      continue;
    }
    if (current_section != section) {
      continue;
    }
    const auto eq = line.find('=');
    if (eq == std::string::npos) {
      throw std::runtime_error("[graph_first_config] invalid config line " +
                               std::to_string(line_number) + ": missing '='");
    }
    auto lhs = kv::trim(line.substr(0, eq));
    auto rhs = kv::trim(line.substr(eq + 1));
    if (lhs == key && !rhs.empty()) {
      return rhs;
    }
  }
  throw std::runtime_error("[graph_first_config] missing required key '" +
                           section + "." + key + "' in " + config_path);
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

[[nodiscard]] inline std::string
optional_config_value(const std::unordered_map<std::string, std::string> &cfg,
                      const std::string &key, std::string fallback) {
  const auto it = cfg.find(key);
  if (it == cfg.end() || kv::trim(it->second).empty()) {
    return fallback;
  }
  return it->second;
}

[[nodiscard]] inline std::string optional_config_path_value(
    const std::unordered_map<std::string, std::string> &cfg,
    const std::string &key, const std::string &config_path,
    const std::string &fallback_relative_path) {
  const std::string active_bundle_fallback =
      resolve_config_relative_path(config_path, fallback_relative_path);
  std::error_code ec;
  if (!active_bundle_fallback.empty() &&
      std::filesystem::exists(active_bundle_fallback, ec)) {
    return optional_config_value(cfg, key, active_bundle_fallback);
  }
  const std::string default_config_path =
      cuwacunu::ujcamei::source::contract::default_source_config_path();
  return optional_config_value(
      cfg, key,
      resolve_config_relative_path(default_config_path,
                                   fallback_relative_path));
}

} // namespace graph_first_config_detail

[[nodiscard]] inline std::string
load_accounting_numeraire_node_id_from_config(std::string config_path = {}) {
  if (cuwacunu::piaabo::parse::simple_kv::trim(config_path).empty()) {
    config_path =
        cuwacunu::ujcamei::source::contract::default_source_config_path();
  }
  return graph_first_config_detail::required_section_config_value(
      config_path, "ACCOUNTING", "accounting_numeraire_node_id");
}

[[nodiscard]] inline std::string
resolve_accounting_numeraire_node_id(std::string explicit_node_id,
                                     std::string config_path = {}) {
  explicit_node_id =
      cuwacunu::piaabo::parse::simple_kv::trim(std::move(explicit_node_id));
  if (!explicit_node_id.empty()) {
    return explicit_node_id;
  }
  return load_accounting_numeraire_node_id_from_config(std::move(config_path));
}

[[nodiscard]] inline cuwacunu::hero::runtime::settings::wave_settings_t
load_wave_settings_from_config(std::string config_path = {}) {
  if (cuwacunu::piaabo::parse::simple_kv::trim(config_path).empty()) {
    config_path =
        cuwacunu::ujcamei::source::contract::default_source_config_path();
  }
  const auto cfg =
      graph_first_config_detail::parse_assignment_config(config_path);
  const auto protocol = decode_protocol_variant_from_dsl(
      graph_first_config_detail::read_text_file_or_throw(
          graph_first_config_detail::required_config_value(
              cfg, "kikijyeba_protocol_dsl_path", config_path)));
  return cuwacunu::hero::runtime::settings::decode_wave_settings_from_dsl(
      graph_first_config_detail::read_text_file_or_throw(
          graph_first_config_detail::required_config_value(
              cfg, "runtime_wave_dsl_path", config_path)),
      graph_first_config_detail::optional_config_value(cfg, "runtime_wave_id",
                                                       ""),
      runtime_wave_protocol_bindings(protocol),
      graph_first_config_detail::read_text_file_or_throw(
          graph_first_config_detail::required_config_value(
              cfg, "ujcamei_source_cursor_dsl_path", config_path)));
}

[[nodiscard]] inline channel_graph_first_protocol_contract_t
load_channel_graph_first_protocol_contract_from_config(
    std::string config_path = {}) {
  if (cuwacunu::piaabo::parse::simple_kv::trim(config_path).empty()) {
    config_path =
        cuwacunu::ujcamei::source::contract::default_source_config_path();
  }
  const auto cfg =
      graph_first_config_detail::parse_assignment_config(config_path);

  channel_graph_first_protocol_contract_t out{};
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
  out.wikimyei_representation_mtf_jepa_mae_vicreg_dsl_bnf_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_representation_mtf_jepa_mae_vicreg_dsl_bnf_path",
          config_path,
          "grammar/wikimyei.representation.mtf_jepa_mae_vicreg.dsl.bnf");
  out.wikimyei_representation_mtf_jepa_mae_vicreg_dsl_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_representation_mtf_jepa_mae_vicreg_dsl_path",
          config_path, "wikimyei.representation.mtf_jepa_mae_vicreg.dsl");
  out.wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path",
          config_path,
          "grammar/wikimyei.representation.mtf_jepa_mae_vicreg.net.bnf");
  out.wikimyei_representation_mtf_jepa_mae_vicreg_net_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_representation_mtf_jepa_mae_vicreg_net_path",
          config_path, "wikimyei.representation.mtf_jepa_mae_vicreg.net");
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
  out.wikimyei_observer_belief_dsl_bnf_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_observer_belief_dsl_bnf_path", config_path,
          "grammar/wikimyei.observer.belief.dsl.bnf");
  out.wikimyei_observer_belief_dsl_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_observer_belief_dsl_path", config_path,
          "wikimyei.observer.belief.dsl");
  out.wikimyei_policy_portfolio_spot_distributional_utility_dsl_bnf_path =
      graph_first_config_detail::optional_config_path_value(
          cfg,
          "wikimyei_policy_portfolio_spot_distributional_utility_dsl_bnf_path",
          config_path,
          "grammar/wikimyei.policy.portfolio.spot_distributional_utility.dsl."
          "bnf");
  out.wikimyei_policy_portfolio_spot_distributional_utility_dsl_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_policy_portfolio_spot_distributional_utility_dsl_path",
          config_path,
          "wikimyei.policy.portfolio.spot_distributional_utility.dsl");
  out.wikimyei_policy_portfolio_graph_node_allocation_dsl_bnf_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_policy_portfolio_graph_node_allocation_dsl_bnf_path",
          config_path,
          "grammar/wikimyei.policy.portfolio.graph_node_allocation.dsl.bnf");
  out.wikimyei_policy_portfolio_graph_node_allocation_dsl_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_policy_portfolio_graph_node_allocation_dsl_path",
          config_path, "wikimyei.policy.portfolio.graph_node_allocation.dsl");
  out.wikimyei_policy_portfolio_graph_node_allocation_net_bnf_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_policy_portfolio_graph_node_allocation_net_bnf_path",
          config_path,
          "grammar/wikimyei.policy.portfolio.graph_node_allocation.net.bnf");
  out.wikimyei_policy_portfolio_graph_node_allocation_net_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_policy_portfolio_graph_node_allocation_net_path",
          config_path, "wikimyei.policy.portfolio.graph_node_allocation.net");
  out.wikimyei_policy_portfolio_graph_node_allocation_features_bnf_path =
      graph_first_config_detail::optional_config_path_value(
          cfg,
          "wikimyei_policy_portfolio_graph_node_allocation_features_bnf_path",
          config_path,
          "grammar/"
          "wikimyei.policy.portfolio.graph_node_allocation.features.dsl.bnf");
  out.wikimyei_policy_portfolio_graph_node_allocation_features_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_policy_portfolio_graph_node_allocation_features_path",
          config_path,
          "wikimyei.policy.portfolio.graph_node_allocation.features.dsl");
  out.wikimyei_representation_vicreg_jkimyei_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_representation_vicreg_jkimyei_bnf_path", config_path);
  out.wikimyei_representation_vicreg_jkimyei_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_representation_vicreg_jkimyei_path", config_path);
  out.wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_bnf_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_bnf_path",
          config_path,
          "grammar/wikimyei.representation.mtf_jepa_mae_vicreg.jkimyei.bnf");
  out.wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path",
          config_path, "wikimyei.representation.mtf_jepa_mae_vicreg.jkimyei");
  out.wikimyei_inference_expected_value_mdn_jkimyei_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_inference_expected_value_mdn_jkimyei_bnf_path",
          config_path);
  out.wikimyei_inference_expected_value_mdn_jkimyei_path =
      graph_first_config_detail::required_config_value(
          cfg, "wikimyei_inference_expected_value_mdn_jkimyei_path",
          config_path);
  out.wikimyei_policy_portfolio_graph_node_allocation_jkimyei_bnf_path =
      graph_first_config_detail::optional_config_path_value(
          cfg,
          "wikimyei_policy_portfolio_graph_node_allocation_jkimyei_bnf_"
          "path",
          config_path,
          "grammar/"
          "wikimyei.policy.portfolio.graph_node_allocation.jkimyei.bnf");
  out.wikimyei_policy_portfolio_graph_node_allocation_jkimyei_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "wikimyei_policy_portfolio_graph_node_allocation_jkimyei_path",
          config_path,
          "wikimyei.policy.portfolio.graph_node_allocation.jkimyei");
  out.runtime_wave_dsl_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "runtime_wave_dsl_bnf_path", config_path);
  out.runtime_wave_dsl_path = graph_first_config_detail::required_config_value(
      cfg, "runtime_wave_dsl_path", config_path);
  out.runtime_wave_id = graph_first_config_detail::optional_config_value(
      cfg, "runtime_wave_id", "");
  out.ujcamei_source_cursor_dsl_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "ujcamei_source_cursor_dsl_bnf_path", config_path);
  out.ujcamei_source_cursor_dsl_path =
      graph_first_config_detail::required_config_value(
          cfg, "ujcamei_source_cursor_dsl_path", config_path);
  out.kikijyeba_protocol_dsl_bnf_path =
      graph_first_config_detail::required_config_value(
          cfg, "kikijyeba_protocol_dsl_bnf_path", config_path);
  out.kikijyeba_protocol_dsl_path =
      graph_first_config_detail::required_config_value(
          cfg, "kikijyeba_protocol_dsl_path", config_path);
  out.kikijyeba_environment_replay_dsl_bnf_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "kikijyeba_environment_replay_dsl_bnf_path", config_path,
          "grammar/kikijyeba.environment.replay.dsl.bnf");
  out.kikijyeba_environment_replay_dsl_path =
      graph_first_config_detail::optional_config_path_value(
          cfg, "kikijyeba_environment_replay_dsl_path", config_path,
          "kikijyeba.environment.replay.dsl");

  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_expression_nodelift_srl_dsl_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_representation_vicreg_dsl_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_representation_vicreg_net_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_representation_mtf_jepa_mae_vicreg_dsl_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_representation_mtf_jepa_mae_vicreg_net_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_inference_expected_value_mdn_dsl_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_inference_expected_value_mdn_net_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_observer_belief_dsl_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_policy_portfolio_spot_distributional_utility_dsl_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_policy_portfolio_graph_node_allocation_dsl_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_policy_portfolio_graph_node_allocation_net_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_policy_portfolio_graph_node_allocation_features_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_representation_vicreg_jkimyei_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_inference_expected_value_mdn_jkimyei_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.wikimyei_policy_portfolio_graph_node_allocation_jkimyei_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.runtime_wave_dsl_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.ujcamei_source_cursor_dsl_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.kikijyeba_protocol_dsl_bnf_path);
  (void)graph_first_config_detail::read_text_file_or_throw(
      out.kikijyeba_environment_replay_dsl_bnf_path);

  out.protocol_variant = decode_protocol_variant_from_dsl(
      graph_first_config_detail::read_text_file_or_throw(
          out.kikijyeba_protocol_dsl_path));
  out.wave_settings =
      cuwacunu::hero::runtime::settings::decode_wave_settings_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.runtime_wave_dsl_path),
          out.runtime_wave_id,
          runtime_wave_protocol_bindings(out.protocol_variant),
          graph_first_config_detail::read_text_file_or_throw(
              out.ujcamei_source_cursor_dsl_path));
  out.replay_environment =
      cuwacunu::kikijyeba::environment::decode_replay_environment_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.kikijyeba_environment_replay_dsl_path));
  out.nodelift = cuwacunu::wikimyei::expression::nodelift::srl::
      decode_nodelift_srl_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_expression_nodelift_srl_dsl_path));
  out.vicreg = cuwacunu::wikimyei::representation::encoding::vicreg::
      decode_vicreg_spec_from_split_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_representation_vicreg_dsl_path),
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_representation_vicreg_net_path));
  out.mtf_jepa_mae_vicreg = cuwacunu::wikimyei::representation::encoding::
      mtf_jepa_mae_vicreg::decode_mtf_jepa_mae_vicreg_spec_from_split_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_representation_mtf_jepa_mae_vicreg_dsl_path),
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_representation_mtf_jepa_mae_vicreg_net_path));
  out.channel_mdn = cuwacunu::wikimyei::inference::expected_value::mdn::
      decode_channel_mdn_spec_from_split_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_inference_expected_value_mdn_dsl_path),
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_inference_expected_value_mdn_net_path));
  out.belief_observer = cuwacunu::wikimyei::observer::belief::
      decode_belief_observer_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_observer_belief_dsl_path));
  out.spot_distributional_utility = cuwacunu::wikimyei::policy::portfolio::
      spot_distributional_utility::decode_spot_distributional_utility_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_policy_portfolio_spot_distributional_utility_dsl_path));
  out.graph_node_allocation = cuwacunu::wikimyei::policy::portfolio::
      graph_node_allocation::decode_graph_node_allocation_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_policy_portfolio_graph_node_allocation_dsl_path));
  out.graph_node_allocation_net = cuwacunu::wikimyei::policy::portfolio::
      graph_node_allocation::decode_graph_node_allocation_net_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_policy_portfolio_graph_node_allocation_net_path));
  out.graph_node_allocation_features = cuwacunu::wikimyei::policy::portfolio::
      graph_node_allocation::decode_graph_node_allocation_feature_manifest_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_policy_portfolio_graph_node_allocation_features_path));
  out.nodelift_assembly =
      cuwacunu::wikimyei::expression::nodelift::srl::make_nodelift_srl_assembly(
          out.nodelift);
  out.vicreg_assembly = cuwacunu::wikimyei::representation::encoding::vicreg::
      make_vicreg_assembly(out.vicreg.component_assembly_id,
                           out.vicreg.version_token);
  out.mtf_jepa_mae_vicreg_assembly = cuwacunu::wikimyei::representation::
      encoding::mtf_jepa_mae_vicreg::make_mtf_jepa_mae_vicreg_assembly(
          out.mtf_jepa_mae_vicreg.component_assembly_id,
          out.mtf_jepa_mae_vicreg.version_token);
  out.channel_mdn_assembly = cuwacunu::wikimyei::inference::expected_value::
      mdn::make_channel_context_mdn_assembly(
          out.channel_mdn.component_assembly_id, out.channel_mdn.version_token);
  out.belief_observer_assembly = cuwacunu::wikimyei::observer::belief::
      make_nodelift_allocation_belief_assembly(
          out.belief_observer.component_assembly_id,
          out.belief_observer.version_token);
  out.spot_distributional_utility_assembly =
      cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility::
          make_spot_distributional_utility_assembly(
              out.spot_distributional_utility.component_assembly_id,
              out.spot_distributional_utility.version_token);
  out.graph_node_allocation_assembly = cuwacunu::wikimyei::policy::portfolio::
      graph_node_allocation::make_graph_node_allocation_assembly(
          out.graph_node_allocation.component_assembly_id,
          out.graph_node_allocation.version_token);
  out.wikimyei_registry.add(out.nodelift_assembly);
  out.wikimyei_registry.add(out.vicreg_assembly);
  out.wikimyei_registry.add(out.mtf_jepa_mae_vicreg_assembly);
  out.wikimyei_registry.add(out.channel_mdn_assembly);
  out.wikimyei_registry.add(out.belief_observer_assembly);
  out.wikimyei_registry.add(out.spot_distributional_utility_assembly);
  out.wikimyei_registry.add(out.graph_node_allocation_assembly);
  out.vicreg_training =
      cuwacunu::jkimyei::training::decode_training_run_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_representation_vicreg_jkimyei_path),
          config_bundle_detail::training_contract_defaults_for_component(
              out.protocol_variant, out.vicreg.component_assembly_id));
  out.mtf_jepa_mae_vicreg_training =
      cuwacunu::jkimyei::training::decode_training_run_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_representation_mtf_jepa_mae_vicreg_jkimyei_path),
          config_bundle_detail::training_contract_defaults_for_component(
              out.protocol_variant,
              out.mtf_jepa_mae_vicreg.component_assembly_id));
  out.channel_mdn_training =
      cuwacunu::jkimyei::training::decode_training_run_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_inference_expected_value_mdn_jkimyei_path),
          config_bundle_detail::training_contract_defaults_for_component(
              out.protocol_variant, out.channel_mdn.component_assembly_id));
  out.graph_node_allocation_training =
      cuwacunu::jkimyei::training::decode_training_run_spec_from_dsl(
          graph_first_config_detail::read_text_file_or_throw(
              out.wikimyei_policy_portfolio_graph_node_allocation_jkimyei_path),
          config_bundle_detail::training_contract_defaults_for_component(
              out.protocol_variant,
              out.graph_node_allocation.component_assembly_id));

  populate_channel_graph_first_source_plan(out);
  out.dock_binding = make_channel_graph_first_dock_binding(out);
  out.dock_binding_report = make_channel_graph_first_dock_binding_report(out);
  validate_channel_graph_first_protocol_contract(out);
  return out;
}

[[nodiscard]] inline channel_graph_first_config_bundle_t
load_channel_graph_first_config_bundle_from_config(
    std::string config_path = {}) {
  return load_channel_graph_first_protocol_contract_from_config(
      std::move(config_path));
}

} // namespace cuwacunu::kikijyeba::protocol
