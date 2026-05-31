// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::wikimyei::observer::belief {

struct belief_observer_spec_t {
  std::string version_token{
      "wikimyei.observer.belief.nodelift_allocation_belief.v1"};
  std::string component_assembly_id{"nodelift_allocation_belief_v1"};
  std::string input_mdn_assembly_id{"mdn_v1"};
  std::string feature_semantics{"ujcamei_kline_registry"};
  std::string feature_semantics_source{"dock_coordinate_space"};
  std::string graph_node_axis_policy{"graph_node_axis_binding"};
  std::string batch_policy{"single_anchor"};
  std::string channel_consensus{"uniform_valid_channels"};
  std::string potential_semantics{"edge_log_return_lifted_potential"};
  std::string return_projection{"base_relative_nodelift_projection"};
  std::string scenario_unit{"arithmetic_return"};
  std::string reserve_asset_policy{"graph_node_from_base_policy"};
  std::string covariance_coupler{"gaussian_copula_shrinkage"};
  std::int64_t scenario_count{1024};
  bool projection_validation_required{true};
  bool live_capital_allowed{false};
  double eps{1.0e-12};
};

inline void validate_belief_observer_spec(const belief_observer_spec_t &spec) {
  if (spec.version_token !=
      "wikimyei.observer.belief.nodelift_allocation_belief.v1") {
    throw std::runtime_error("[belief_observer_spec] unsupported VERSION");
  }
  if (spec.component_assembly_id.empty()) {
    throw std::runtime_error(
        "[belief_observer_spec] COMPONENT_ASSEMBLY_ID is required");
  }
  if (spec.input_mdn_assembly_id.empty()) {
    throw std::runtime_error(
        "[belief_observer_spec] INPUT_MDN_ASSEMBLY_ID is required");
  }
  if (spec.feature_semantics != "ujcamei_kline_registry" ||
      spec.feature_semantics_source != "dock_coordinate_space") {
    throw std::runtime_error(
        "[belief_observer_spec] feature semantics must come from docks and "
        "the Ujcamei kline registry");
  }
  if (spec.graph_node_axis_policy != "graph_node_axis_binding") {
    throw std::runtime_error(
        "[belief_observer_spec] GRAPH_NODE_AXIS_POLICY must be "
        "graph_node_axis_binding");
  }
  if (spec.batch_policy != "single_anchor") {
    throw std::runtime_error(
        "[belief_observer_spec] BATCH_POLICY must be single_anchor");
  }
  if (spec.channel_consensus != "uniform_valid_channels") {
    throw std::runtime_error("[belief_observer_spec] CHANNEL_CONSENSUS must be "
                             "uniform_valid_channels");
  }
  if (spec.potential_semantics != "edge_log_return_lifted_potential" ||
      spec.return_projection != "base_relative_nodelift_projection" ||
      spec.scenario_unit != "arithmetic_return") {
    throw std::runtime_error(
        "[belief_observer_spec] v1 requires NodeLift potential projection into "
        "base-relative arithmetic-return scenarios");
  }
  if (spec.reserve_asset_policy != "graph_node_from_base_policy") {
    throw std::runtime_error(
        "[belief_observer_spec] reserve asset must be a graph node supplied by "
        "BasePolicy");
  }
  if (spec.covariance_coupler != "gaussian_copula_shrinkage") {
    throw std::runtime_error(
        "[belief_observer_spec] COVARIANCE_COUPLER must be "
        "gaussian_copula_shrinkage");
  }
  if (spec.scenario_count <= 0) {
    throw std::runtime_error(
        "[belief_observer_spec] SCENARIO_COUNT must be positive");
  }
  if (!spec.projection_validation_required) {
    throw std::runtime_error(
        "[belief_observer_spec] PROJECTION_VALIDATION_REQUIRED must be true "
        "for V1");
  }
  if (spec.live_capital_allowed) {
    throw std::runtime_error(
        "[belief_observer_spec] LIVE_CAPITAL_ALLOWED must be false for V1");
  }
  if (spec.eps <= 0.0) {
    throw std::runtime_error("[belief_observer_spec] EPS must be positive");
  }
}

[[nodiscard]] inline belief_observer_spec_t
decode_belief_observer_spec_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(dsl_text, "OBSERVER_BELIEF");
  belief_observer_spec_t spec{};
  spec.version_token = kv::required(block, "VERSION");
  spec.component_assembly_id = kv::required(block, "COMPONENT_ASSEMBLY_ID");
  spec.input_mdn_assembly_id = kv::required(block, "INPUT_MDN_ASSEMBLY_ID");
  spec.feature_semantics = kv::required(block, "FEATURE_SEMANTICS");
  spec.feature_semantics_source =
      kv::required(block, "FEATURE_SEMANTICS_SOURCE");
  spec.graph_node_axis_policy = kv::required(block, "GRAPH_NODE_AXIS_POLICY");
  spec.batch_policy = kv::required(block, "BATCH_POLICY");
  spec.channel_consensus = kv::required(block, "CHANNEL_CONSENSUS");
  spec.potential_semantics = kv::required(block, "POTENTIAL_SEMANTICS");
  spec.return_projection = kv::required(block, "RETURN_PROJECTION");
  spec.scenario_unit = kv::required(block, "SCENARIO_UNIT");
  spec.reserve_asset_policy = kv::required(block, "RESERVE_ASSET_POLICY");
  spec.covariance_coupler = kv::required(block, "COVARIANCE_COUPLER");
  spec.scenario_count = kv::parse_i64(kv::required(block, "SCENARIO_COUNT"));
  spec.projection_validation_required =
      kv::parse_bool(kv::required(block, "PROJECTION_VALIDATION_REQUIRED"));
  spec.live_capital_allowed =
      kv::parse_bool(kv::required(block, "LIVE_CAPITAL_ALLOWED"));
  spec.eps = kv::parse_double(kv::required(block, "EPS"));
  validate_belief_observer_spec(spec);
  return spec;
}

} // namespace cuwacunu::wikimyei::observer::belief
