// SPDX-License-Identifier: MIT
#pragma once

#include <string>

#include "wikimyei/assembly.h"
#include "wikimyei/expression/nodelift/srl/nodelift_spec.h"

namespace cuwacunu::wikimyei::expression::nodelift::srl {

[[nodiscard]] inline cuwacunu::wikimyei::assembly::wikimyei_assembly_t
make_nodelift_srl_assembly(const nodelift_srl_spec_t &spec) {
  namespace wa = cuwacunu::wikimyei::assembly;
  validate_nodelift_srl_spec(spec);

  wa::wikimyei_assembly_t out{};
  out.family = "wikimyei.expression.nodelift.srl";
  out.component_id = spec.component_id;
  out.version_token = spec.version_token;
  out.trainability = wa::assembly_trainability_t::deterministic;
  out.docks.push_back(wa::make_dock(
      "observed_graph_edge_kline_evidence", wa::dock_direction_t::consumes,
      wa::dock_role_t::conditioning,
      wa::dock_domain_t::graph_edge_kline_evidence, "graph_order.kline.v1",
      "[B,L,C,Hx,9]", "[B,L,C,Hx]", /*required=*/true,
      /*target_side_only=*/false, {"B", "L", "C", "Hx", "F"}));
  out.docks.push_back(wa::make_dock(
      "observed_node_lifted_state", wa::dock_direction_t::produces,
      wa::dock_role_t::conditioning, wa::dock_domain_t::node_lifted_state,
      "graph_order.kline.v1", "[B,C,Hx,N,9]", "[B,C,Hx,N,9]",
      /*required=*/true, /*target_side_only=*/false,
      {"B", "C", "Hx", "N", "F"}));

  if (spec.future_lift_policy == future_lift_policy_t::target_side) {
    out.docks.push_back(wa::make_dock(
        "future_graph_edge_kline_evidence", wa::dock_direction_t::consumes,
        wa::dock_role_t::supervised_target,
        wa::dock_domain_t::future_graph_edge_kline_evidence,
        "graph_order.kline.v1", "[B,L,C,Hf,9]", "[B,L,C,Hf]",
        /*required=*/true, /*target_side_only=*/true,
        {"B", "L", "C", "Hf", "F"}));
    out.docks.push_back(wa::make_dock(
        "future_node_lifted_state", wa::dock_direction_t::produces,
        wa::dock_role_t::supervised_target,
        wa::dock_domain_t::future_node_lifted_state, "graph_order.kline.v1",
        "[B,C,Hf,N,9]", "[B,C,Hf,N,9]", /*required=*/true,
        /*target_side_only=*/true, {"B", "C", "Hf", "N", "F"}));
  }

  out.docks.push_back(wa::make_dock(
      "price_residual_sidecar", wa::dock_direction_t::produces,
      wa::dock_role_t::diagnostic_sidecar,
      wa::dock_domain_t::price_residual_sidecar, "graph_order.kline.v1",
      "[B,C,Hx,L,4]", "[B,C,Hx,L,4]", /*required=*/false,
      /*target_side_only=*/false, {"B", "C", "Hx", "L"}));
  out.docks.push_back(wa::make_dock(
      "activity_sidecar", wa::dock_direction_t::produces,
      wa::dock_role_t::diagnostic_sidecar, wa::dock_domain_t::activity_sidecar,
      "graph_order.kline.v1", "[B,C,Hx,N,activity_sidecars]", {},
      /*required=*/false, /*target_side_only=*/false, {"B", "C", "Hx", "N"}));

  out.constraints.push_back(
      "future_node_lifted_state is target-side only and must not feed "
      "same-anchor representation conditioning");
  out.constraints.push_back("price coordinates use uniform synthetic gauge");
  out.constraints.push_back(
      "activity coordinates are support-mean node intensities");
  wa::validate_wikimyei_assembly(out);
  return out;
}

} // namespace cuwacunu::wikimyei::expression::nodelift::srl
