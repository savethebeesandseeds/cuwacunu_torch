// SPDX-License-Identifier: MIT
#pragma once

#include "wikimyei/assembly.h"

namespace cuwacunu::kikijyeba::topology {

inline void validate_node_value_assembly_chain(
    const cuwacunu::wikimyei::assembly::wikimyei_assembly_t &nodelift,
    const cuwacunu::wikimyei::assembly::wikimyei_assembly_t &vicreg,
    const cuwacunu::wikimyei::assembly::wikimyei_assembly_t &mdn) {
  namespace assembly = cuwacunu::wikimyei::assembly;

  assembly::validate_wikimyei_assembly(nodelift);
  assembly::validate_wikimyei_assembly(vicreg);
  assembly::validate_wikimyei_assembly(mdn);

  assembly::require_compatible_docks(
      nodelift, assembly::dock_domain_t::node_lifted_state, vicreg,
      assembly::dock_domain_t::node_lifted_state,
      "NodeLift->VICReg conditioning");
  assembly::require_compatible_docks(
      vicreg, assembly::dock_domain_t::node_representation, mdn,
      assembly::dock_domain_t::node_representation, "VICReg->MDN conditioning");
  assembly::require_compatible_docks(
      nodelift, assembly::dock_domain_t::future_node_lifted_state, mdn,
      assembly::dock_domain_t::future_node_lifted_state,
      "NodeLift->MDN target");
  assembly::require_dock(mdn, assembly::dock_direction_t::produces,
                         assembly::dock_domain_t::future_node_distribution,
                         "MDN future node distribution");
}

} // namespace cuwacunu::kikijyeba::topology
