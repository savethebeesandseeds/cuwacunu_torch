// SPDX-License-Identifier: MIT
#pragma once

#include "wikimyei/assembly.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_spec.h"

namespace cuwacunu::wikimyei::representation::encoding::vicreg {

[[nodiscard]] inline cuwacunu::wikimyei::assembly::wikimyei_assembly_t
make_vicreg_representation_assembly(
    const vicreg_node_representation_spec_t &spec) {
  namespace wa = cuwacunu::wikimyei::assembly;
  validate_vicreg_node_representation_spec(spec);

  wa::wikimyei_assembly_t out{};
  out.family = "wikimyei.representation.encoding.vicreg";
  out.component_id = spec.component_id;
  out.version_token = spec.version_token;
  out.trainability = wa::assembly_trainability_t::trainable;
  out.docks.push_back(wa::make_dock(
      "observed_node_lifted_state", wa::dock_direction_t::consumes,
      wa::dock_role_t::conditioning, wa::dock_domain_t::node_lifted_state,
      "graph_order.kline.v1", "[B,C,Hx,N,9]", "[B,C,Hx,N,9]",
      /*required=*/true, /*target_side_only=*/false,
      {"B", "C", "Hx", "N", "F"}));
  out.docks.push_back(wa::make_dock(
      "node_representation", wa::dock_direction_t::produces,
      wa::dock_role_t::output, wa::dock_domain_t::node_representation,
      "graph_order.node_representation.v1", "[B,N,D_e] or [B,N,Hx',D_e]",
      "[B,N]", /*required=*/true, /*target_side_only=*/false,
      {"B", "N", "De"}));
  out.constraints.push_back(
      "node stream adapter flattens row order as row = b * N + n");
  out.constraints.push_back("training may compact valid node windows");
  out.constraints.push_back("encoding preserves full graph-node order");
  wa::validate_wikimyei_assembly(out);
  return out;
}

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg
