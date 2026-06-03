// SPDX-License-Identifier: MIT
#pragma once

#include <string>

#include "wikimyei/assembly.h"

namespace cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility {

[[nodiscard]] inline cuwacunu::wikimyei::assembly::wikimyei_assembly_t
make_spot_distributional_utility_assembly(
    std::string component_assembly_id = "spot_distributional_utility_v1",
    std::string version_token =
        "wikimyei.policy.portfolio.spot_distributional_utility.v1") {
  namespace wa = cuwacunu::wikimyei::assembly;

  wa::wikimyei_assembly_t out{};
  out.family = "wikimyei.policy.portfolio.spot_distributional_utility";
  out.component_assembly_id = std::move(component_assembly_id);
  out.version_token = std::move(version_token);
  out.trainability = wa::assembly_trainability_t::deterministic;
  out.docks.push_back(wa::make_dock(
      "allocation_belief", wa::dock_direction_t::consumes,
      wa::dock_role_t::conditioning, wa::dock_domain_t::allocation_belief,
      "portfolio.base_relative_nodelift_projection.v1", "AllocationBelief[S,A]",
      "[A]",
      /*required=*/true, /*target_side_only=*/false, {"S", "A"}));
  out.docks.push_back(wa::make_dock(
      "allocation_target", wa::dock_direction_t::produces,
      wa::dock_role_t::output, wa::dock_domain_t::allocation_target,
      "portfolio.spot_distributional_utility.target.v1", "TargetPortfolio[A]",
      "[A]",
      /*required=*/true, /*target_side_only=*/false, {"A"}));
  out.docks.push_back(wa::make_dock(
      "portfolio_diagnostics", wa::dock_direction_t::produces,
      wa::dock_role_t::diagnostic_sidecar,
      wa::dock_domain_t::portfolio_diagnostics,
      "wikimyei.policy.portfolio.spot_distributional_utility.diagnostics.v1",
      "decision_diagnostics_t", {},
      /*required=*/false, /*target_side_only=*/false, {}));

  out.constraints.push_back("deterministic portfolio allocation assembly");
  out.constraints.push_back(
      "consumes only post-projection AllocationBelief, not raw NodeLift "
      "potentials");
  out.constraints.push_back("long-only risky weights");
  out.constraints.push_back(
      "base reserve weight is assigned to an explicit graph node supplied by "
      "the belief BasePolicy");
  out.constraints.push_back("spot-only allocation; execution is a separate "
                            "policy method boundary");
  out.constraints.push_back("no portfolio target is valid without finite "
                            "scenario growth");
  out.constraints.push_back("confidence, capacity, tradability, and turnover "
                            "constraints cap target weights");
  wa::validate_wikimyei_assembly(out);
  return out;
}

} // namespace
  // cuwacunu::wikimyei::policy::portfolio::spot_distributional_utility
