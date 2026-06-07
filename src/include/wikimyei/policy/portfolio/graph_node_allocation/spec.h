// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>

#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::wikimyei::policy::portfolio::graph_node_allocation {

struct graph_node_allocation_spec_t {
  std::string version_token{
      "wikimyei.policy.portfolio.graph_node_allocation.v1"};
  std::string component_assembly_id{"graph_node_allocation_v1"};
  std::string policy_kind{"trainable_graph_node_allocation"};
  std::string policy_input_schema{"kikijyeba.environment.policy_input.v1"};
  std::string action_adapter{"target_node_weights_simplex.v1"};
  std::string action_schema{
      "kikijyeba.environment.action.target_node_weights.v1"};
  std::string reward_contract{"kikijyeba.environment.reward.post_execution_"
                              "ledger_log_growth_cost_drawdown.v1"};
  std::string graph_node_universe_policy{"fixed_ordered_target_node_universe"};
  std::string scenario_input_policy{
      "allocation_belief_distributional_summaries_v1"};
  bool raw_mdn_input_allowed{false};
  bool ppo_implemented{false};
  bool live_capital_allowed{false};
};

struct graph_node_allocation_net_spec_t {
  std::string version_token{
      "wikimyei.policy.portfolio.graph_node_allocation.net.v1"};
  std::int64_t input_node_feature_dim{27};
  std::int64_t input_global_feature_dim{8};
  std::int64_t node_encoder_hidden_dim{64};
  std::int64_t global_encoder_hidden_dim{32};
  std::int64_t allocation_head_hidden_dim{128};
  std::string output_head{"node_weight_logits"};
  std::string value_head{"reserved_for_ppo"};
  std::string action_adapter{"target_node_weights_simplex.v1"};
  bool ppo_execution_allowed{false};
};

inline void
validate_graph_node_allocation_spec(const graph_node_allocation_spec_t &spec) {
  if (spec.version_token !=
      "wikimyei.policy.portfolio.graph_node_allocation.v1") {
    throw std::runtime_error("[graph_node_allocation_spec] unsupported "
                             "VERSION");
  }
  if (spec.component_assembly_id.empty()) {
    throw std::runtime_error("[graph_node_allocation_spec] "
                             "COMPONENT_ASSEMBLY_ID is required");
  }
  if (spec.policy_kind != "trainable_graph_node_allocation") {
    throw std::runtime_error(
        "[graph_node_allocation_spec] unsupported POLICY_KIND");
  }
  if (spec.policy_input_schema != "kikijyeba.environment.policy_input.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_spec] unsupported POLICY_INPUT_SCHEMA");
  }
  if (spec.action_adapter != "target_node_weights_simplex.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_spec] unsupported ACTION_ADAPTER");
  }
  if (spec.action_schema !=
      "kikijyeba.environment.action.target_node_weights.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_spec] unsupported ACTION_SCHEMA");
  }
  if (spec.reward_contract != "kikijyeba.environment.reward.post_execution_"
                              "ledger_log_growth_cost_drawdown.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_spec] unsupported REWARD_CONTRACT");
  }
  if (spec.graph_node_universe_policy != "fixed_ordered_target_node_universe") {
    throw std::runtime_error(
        "[graph_node_allocation_spec] unsupported graph universe policy");
  }
  if (spec.scenario_input_policy !=
      "allocation_belief_distributional_summaries_v1") {
    throw std::runtime_error(
        "[graph_node_allocation_spec] V1 does not expose raw scenario "
        "banks or raw MDN tensors by default");
  }
  if (spec.raw_mdn_input_allowed || spec.ppo_implemented ||
      spec.live_capital_allowed) {
    throw std::runtime_error(
        "[graph_node_allocation_spec] V1 is contract-only: raw MDN, PPO "
        "execution, and live capital must be disabled");
  }
}

inline void validate_graph_node_allocation_net_spec(
    const graph_node_allocation_net_spec_t &spec) {
  if (spec.version_token !=
      "wikimyei.policy.portfolio.graph_node_allocation.net.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] unsupported VERSION");
  }
  if (spec.input_node_feature_dim <= 0 || spec.input_global_feature_dim <= 0 ||
      spec.node_encoder_hidden_dim <= 0 ||
      spec.global_encoder_hidden_dim <= 0 ||
      spec.allocation_head_hidden_dim <= 0) {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] dimensions must be positive");
  }
  if (spec.output_head != "node_weight_logits") {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] OUTPUT_HEAD must be "
        "node_weight_logits");
  }
  if (spec.value_head != "reserved_for_ppo") {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] VALUE_HEAD must remain "
        "reserved_for_ppo until PPO is implemented");
  }
  if (spec.action_adapter != "target_node_weights_simplex.v1") {
    throw std::runtime_error(
        "[graph_node_allocation_net_spec] ACTION_ADAPTER mismatch");
  }
  if (spec.ppo_execution_allowed) {
    throw std::runtime_error("[graph_node_allocation_net_spec] PPO "
                             "execution is not implemented");
  }
}

[[nodiscard]] inline graph_node_allocation_spec_t
decode_graph_node_allocation_spec_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(dsl_text, "GRAPH_NODE_ALLOCATION");
  graph_node_allocation_spec_t spec{};
  spec.version_token = kv::required(block, "VERSION");
  spec.component_assembly_id = kv::required(block, "COMPONENT_ASSEMBLY_ID");
  spec.policy_kind = kv::required(block, "POLICY_KIND");
  spec.policy_input_schema = kv::required(block, "POLICY_INPUT_SCHEMA");
  spec.action_adapter = kv::required(block, "ACTION_ADAPTER");
  spec.action_schema = kv::required(block, "ACTION_SCHEMA");
  spec.reward_contract = kv::required(block, "REWARD_CONTRACT");
  spec.graph_node_universe_policy =
      kv::required(block, "GRAPH_NODE_UNIVERSE_POLICY");
  spec.scenario_input_policy = kv::required(block, "SCENARIO_INPUT_POLICY");
  spec.raw_mdn_input_allowed =
      kv::parse_bool(kv::required(block, "RAW_MDN_INPUT_ALLOWED"));
  spec.ppo_implemented = kv::parse_bool(kv::required(block, "PPO_IMPLEMENTED"));
  spec.live_capital_allowed =
      kv::parse_bool(kv::required(block, "LIVE_CAPITAL_ALLOWED"));
  validate_graph_node_allocation_spec(spec);
  return spec;
}

[[nodiscard]] inline graph_node_allocation_net_spec_t
decode_graph_node_allocation_net_spec_from_dsl(const std::string &net_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(net_text, "GRAPH_NODE_ALLOCATION_NET");
  graph_node_allocation_net_spec_t spec{};
  spec.version_token = kv::required(block, "VERSION");
  spec.input_node_feature_dim =
      kv::parse_i64(kv::required(block, "INPUT_NODE_FEATURE_DIM"));
  spec.input_global_feature_dim =
      kv::parse_i64(kv::required(block, "INPUT_GLOBAL_FEATURE_DIM"));
  spec.node_encoder_hidden_dim =
      kv::parse_i64(kv::required(block, "NODE_ENCODER_HIDDEN_DIM"));
  spec.global_encoder_hidden_dim =
      kv::parse_i64(kv::required(block, "GLOBAL_ENCODER_HIDDEN_DIM"));
  spec.allocation_head_hidden_dim =
      kv::parse_i64(kv::required(block, "ALLOCATION_HEAD_HIDDEN_DIM"));
  spec.output_head = kv::required(block, "OUTPUT_HEAD");
  spec.value_head = kv::required(block, "VALUE_HEAD");
  spec.action_adapter = kv::required(block, "ACTION_ADAPTER");
  spec.ppo_execution_allowed =
      kv::parse_bool(kv::required(block, "PPO_EXECUTION_ALLOWED"));
  validate_graph_node_allocation_net_spec(spec);
  return spec;
}

} // namespace cuwacunu::wikimyei::policy::portfolio::graph_node_allocation
