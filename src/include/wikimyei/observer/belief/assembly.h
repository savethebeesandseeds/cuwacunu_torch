// SPDX-License-Identifier: MIT
#pragma once

#include <string>

#include "wikimyei/assembly.h"

namespace cuwacunu::wikimyei::observer::belief {

[[nodiscard]] inline cuwacunu::wikimyei::assembly::wikimyei_assembly_t
make_nodelift_allocation_belief_assembly(
    std::string component_assembly_id = "nodelift_allocation_belief_v1",
    std::string version_token =
        "wikimyei.observer.belief.nodelift_allocation_belief.v1") {
  namespace wa = cuwacunu::wikimyei::assembly;

  wa::wikimyei_assembly_t out{};
  out.family = "wikimyei.observer.belief";
  out.component_assembly_id = std::move(component_assembly_id);
  out.version_token = std::move(version_token);
  out.trainability = wa::assembly_trainability_t::deterministic;
  out.docks.push_back(wa::make_dock(
      "future_channel_node_distribution", wa::dock_direction_t::consumes,
      wa::dock_role_t::conditioning,
      wa::dock_domain_t::future_node_distribution,
      "graph_order.channel_node_future_distribution.v1",
      "log_pi[B,N,C,Df,K];mu/sigma[B,N,C,Df,K]", "[B,N,C,Df]",
      /*required=*/true, /*target_side_only=*/false,
      {"B", "N", "C", "Df", "K"}));
  out.docks.push_back(wa::make_dock(
      "nodelift_potential_belief", wa::dock_direction_t::produces,
      wa::dock_role_t::diagnostic_sidecar,
      wa::dock_domain_t::nodelift_potential_belief,
      "graph_order.kline.nodelift_potential_belief.v1",
      "NodeLiftPotentialBelief[G,Df,Kc]", "[G,Df]",
      /*required=*/false, /*target_side_only=*/false, {"G", "Df", "Kc"}));
  out.docks.push_back(wa::make_dock(
      "allocation_belief", wa::dock_direction_t::produces,
      wa::dock_role_t::output, wa::dock_domain_t::allocation_belief,
      "portfolio.numeraire_relative_nodelift_projection.v1", "AllocationBelief[S,A]",
      "[A]",
      /*required=*/true, /*target_side_only=*/false, {"S", "A"}));
  out.docks.push_back(wa::make_dock(
      "belief_diagnostics", wa::dock_direction_t::produces,
      wa::dock_role_t::diagnostic_sidecar,
      wa::dock_domain_t::belief_diagnostics,
      "wikimyei.observer.belief.diagnostics.v1", "BeliefDiagnostics", {},
      /*required=*/false, /*target_side_only=*/false, {}));

  out.constraints.push_back("deterministic observer assembly");
  out.constraints.push_back("feature coordinates resolve through observer "
                            "feature semantics utilities");
  out.constraints.push_back(
      "graph-node axis identity is supplied by GraphNodeAxisBinding");
  out.constraints.push_back(
      "portfolio returns require numeraire-relative NodeLift projection");
  out.constraints.push_back(
      "AllocationBelief node_ids are the portfolio target graph-node universe");
  out.constraints.push_back(
      "accounting numeraire and projection reference must appear in "
      "graph_node_axis");
  wa::validate_wikimyei_assembly(out);
  return out;
}

} // namespace cuwacunu::wikimyei::observer::belief
