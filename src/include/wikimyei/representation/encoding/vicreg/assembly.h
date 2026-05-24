// SPDX-License-Identifier: MIT
#pragma once

#include <string>

#include "wikimyei/assembly.h"

namespace cuwacunu::wikimyei::representation::encoding::vicreg {

[[nodiscard]] inline cuwacunu::wikimyei::assembly::wikimyei_assembly_t
make_vicreg_assembly(
    std::string component_assembly_id = "vicreg_v1",
    std::string version_token = "wikimyei.representation.vicreg.v1") {
  namespace wa = cuwacunu::wikimyei::assembly;

  wa::wikimyei_assembly_t out{};
  out.family = "wikimyei.representation.encoding.vicreg";
  out.component_assembly_id = std::move(component_assembly_id);
  out.version_token = std::move(version_token);
  out.trainability = wa::assembly_trainability_t::trainable;
  out.docks.push_back(wa::make_dock(
      "observed_node_lifted_state", wa::dock_direction_t::consumes,
      wa::dock_role_t::conditioning, wa::dock_domain_t::node_lifted_state,
      "graph_order.kline.v1", "[B,C,Hx,N,9]", "[B,C,Hx,N,9]",
      /*required=*/true, /*target_side_only=*/false,
      {"B", "C", "Hx", "N", "F"}));
  out.docks.push_back(wa::make_dock(
      "channel_node_representation", wa::dock_direction_t::produces,
      wa::dock_role_t::output, wa::dock_domain_t::node_representation,
      "graph_order.channel_node_representation.v1", "[B,N,C,De]", "[B,N,C]",
      /*required=*/true, /*target_side_only=*/false, {"B", "N", "C", "De"}));
  out.constraints.push_back("primary representation preserves channel axis C");
  out.constraints.push_back(
      "temporal reduction is mask-aware and owned by representation boundary");
  out.constraints.push_back(
      "global fused node vectors must be separately named auxiliary products");
  wa::validate_wikimyei_assembly(out);
  return out;
}

[[nodiscard]] inline cuwacunu::wikimyei::assembly::wikimyei_assembly_t
make_channel_global_fusion_assembly(
    std::string component_assembly_id = "channel_global_fusion_v1",
    std::string version_token =
        "wikimyei.representation.channel_global_fusion.v1") {
  namespace wa = cuwacunu::wikimyei::assembly;

  wa::wikimyei_assembly_t out{};
  out.family = "wikimyei.representation.encoding.channel_global_fusion";
  out.component_assembly_id = std::move(component_assembly_id);
  out.version_token = std::move(version_token);
  out.trainability = wa::assembly_trainability_t::trainable;
  out.docks.push_back(wa::make_dock(
      "channel_node_representation", wa::dock_direction_t::consumes,
      wa::dock_role_t::conditioning, wa::dock_domain_t::node_representation,
      "graph_order.channel_node_representation.v1", "[B,N,C,De]", "[B,N,C]",
      /*required=*/true, /*target_side_only=*/false, {"B", "N", "C", "De"}));
  out.docks.push_back(wa::make_dock(
      "global_node_representation", wa::dock_direction_t::produces,
      wa::dock_role_t::output, wa::dock_domain_t::node_representation,
      "graph_order.global_node_representation.v1", "[B,N,Dg]", "[B,N]",
      /*required=*/true, /*target_side_only=*/false, {"B", "N", "Dg"}));
  out.constraints.push_back(
      "global context is an auxiliary product after channel evidence exists");
  out.constraints.push_back(
      "channel_node_representation remains the primary representation");
  out.constraints.push_back(
      "fusion must be mask-aware and expose channel weights as diagnostics");
  wa::validate_wikimyei_assembly(out);
  return out;
}

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg
