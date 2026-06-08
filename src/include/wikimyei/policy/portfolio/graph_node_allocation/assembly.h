// SPDX-License-Identifier: MIT
#pragma once

#include <string>
#include <utility>

#include "wikimyei/assembly.h"

namespace cuwacunu::wikimyei::policy::portfolio::graph_node_allocation {

[[nodiscard]] inline cuwacunu::wikimyei::assembly::wikimyei_assembly_t
make_graph_node_allocation_assembly(
    std::string component_assembly_id = "graph_node_allocation_v1",
    std::string version_token =
        "wikimyei.policy.portfolio.graph_node_allocation.v1") {
  namespace wa = cuwacunu::wikimyei::assembly;

  wa::wikimyei_assembly_t out{};
  out.family = "wikimyei.policy.portfolio.graph_node_allocation";
  out.component_assembly_id = std::move(component_assembly_id);
  out.version_token = std::move(version_token);
  out.trainability = wa::assembly_trainability_t::trainable;
  out.docks.push_back(wa::make_dock(
      "policy_input", wa::dock_direction_t::consumes,
      wa::dock_role_t::conditioning, wa::dock_domain_t::allocation_belief,
      "kikijyeba.environment.policy_input.v1", "PolicyInput[A,F]", "[A,F]",
      /*required=*/true, /*target_side_only=*/false, {"A", "F"}));
  out.docks.push_back(wa::make_dock(
      "target_node_weights", wa::dock_direction_t::produces,
      wa::dock_role_t::output, wa::dock_domain_t::allocation_target,
      "kikijyeba.environment.action.target_node_weights.v1",
      "TargetNodeWeights[A]", "[A]",
      /*required=*/true, /*target_side_only=*/false, {"A"}));
  out.docks.push_back(wa::make_dock(
      "policy_diagnostics", wa::dock_direction_t::produces,
      wa::dock_role_t::diagnostic_sidecar,
      wa::dock_domain_t::portfolio_diagnostics,
      "wikimyei.policy.portfolio.graph_node_allocation.diagnostics.v1",
      "trainable_policy_diagnostics_t", {},
      /*required=*/false, /*target_side_only=*/false, {}));

  out.constraints.push_back(
      "bounded PPO V0 trainable allocation surface; Runtime owns execution");
  out.constraints.push_back(
      "consumes curated policy_input_t, not raw observation_t or raw MDN "
      "tensors");
  out.constraints.push_back(
      "emits one unified target-node weight vector over the ordered graph-node "
      "action universe");
  out.constraints.push_back(
      "accounting numeraire is an ordinary graph node, not a separate reserve "
      "output");
  out.constraints.push_back(
      "target_node_weights_simplex.v1 adapter constrains neural logits to a "
      "long-only simplex action");
  out.constraints.push_back(
      "trainable checkpoints must bind a named simplex action distribution "
      "before PPO execution");
  out.constraints.push_back(
      "training must bind causal_walk_forward_training.v1 before executable "
      "policy learning");
  out.constraints.push_back(
      "live capital remains forbidden for graph-node allocation PPO V0");
  wa::validate_wikimyei_assembly(out);
  return out;
}

} // namespace cuwacunu::wikimyei::policy::portfolio::graph_node_allocation
