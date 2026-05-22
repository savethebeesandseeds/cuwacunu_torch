// SPDX-License-Identifier: MIT
#pragma once

#include "wikimyei/assembly.h"
#include "wikimyei/inference/expected_value/mdn/mdn_spec.h"

namespace cuwacunu::wikimyei::inference::expected_value::mdn {

[[nodiscard]] inline cuwacunu::wikimyei::assembly::wikimyei_assembly_t
make_mdn_expected_value_assembly(const mdn_spec_t &spec) {
  namespace wa = cuwacunu::wikimyei::assembly;
  validate_mdn_spec(spec);

  wa::wikimyei_assembly_t out{};
  out.family = "wikimyei.inference.expected_value.mdn";
  out.component_id = spec.component_id;
  out.version_token = spec.version_token;
  out.trainability = wa::assembly_trainability_t::trainable;
  out.docks.push_back(wa::make_dock(
      "node_representation", wa::dock_direction_t::consumes,
      wa::dock_role_t::conditioning, wa::dock_domain_t::node_representation,
      "graph_order.node_representation.v1", "[B,N,D_e] or [B,N,Hx',D_e]",
      "[B,N]", /*required=*/true, /*target_side_only=*/false,
      {"B", "N", "De"}));
  out.docks.push_back(wa::make_dock(
      "future_node_lifted_state", wa::dock_direction_t::consumes,
      wa::dock_role_t::supervised_target,
      wa::dock_domain_t::future_node_lifted_state, "graph_order.kline.v1",
      "[B,C,Hf,N,9]", "[B,C,Hf,N,9]", /*required=*/true,
      /*target_side_only=*/true, {"B", "C", "Hf", "N", "F"}));
  out.docks.push_back(wa::make_dock(
      "future_node_distribution", wa::dock_direction_t::produces,
      wa::dock_role_t::output, wa::dock_domain_t::future_node_distribution,
      "graph_order.node_expected_value.v1",
      "log_pi[B,N,C,Hf,K];mu/sigma[B,N,C,Hf,K,Df]",
      "[B,N,C,Hf]", /*required=*/true, /*target_side_only=*/false,
      {"B", "N", "C", "Hf", "Df", "K"}));
  out.constraints.push_back("target_domain=node_future");
  out.constraints.push_back("head_policy=per_node");
  out.constraints.push_back("target mask policy requires all selected future "
                            "node features to be valid");
  wa::validate_wikimyei_assembly(out);
  return out;
}

} // namespace cuwacunu::wikimyei::inference::expected_value::mdn
