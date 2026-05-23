// SPDX-License-Identifier: MIT
#pragma once

#include <string>

#include "wikimyei/assembly.h"

namespace cuwacunu::wikimyei::inference::expected_value::mdn {

[[nodiscard]] inline cuwacunu::wikimyei::assembly::wikimyei_assembly_t
make_channel_context_mdn_assembly(
    std::string component_id = "channel_context_mdn_v1",
    std::string version_token = "wikimyei.inference.expected_value.mdn.v1") {
  namespace wa = cuwacunu::wikimyei::assembly;

  wa::wikimyei_assembly_t out{};
  out.family = "wikimyei.inference.expected_value.mdn";
  out.component_id = std::move(component_id);
  out.version_token = std::move(version_token);
  out.trainability = wa::assembly_trainability_t::trainable;
  out.docks.push_back(wa::make_dock(
      "channel_node_representation", wa::dock_direction_t::consumes,
      wa::dock_role_t::conditioning, wa::dock_domain_t::node_representation,
      "graph_order.channel_node_representation.v1", "[B,N,C,De]", "[B,N,C]",
      /*required=*/true, /*target_side_only=*/false, {"B", "N", "C", "De"}));
  out.docks.push_back(wa::make_dock(
      "future_node_lifted_state", wa::dock_direction_t::consumes,
      wa::dock_role_t::supervised_target,
      wa::dock_domain_t::future_node_lifted_state, "graph_order.kline.v1",
      "[B,C,Hf,N,9]", "[B,C,Hf,N,9]", /*required=*/true,
      /*target_side_only=*/true, {"B", "C", "Hf", "N", "F"}));
  out.docks.push_back(wa::make_dock(
      "future_channel_node_distribution", wa::dock_direction_t::produces,
      wa::dock_role_t::output, wa::dock_domain_t::future_node_distribution,
      "graph_order.channel_node_future_distribution.v1",
      "log_pi[B,N,C,Hf,K];mu/sigma[B,N,C,Hf,K,Df]", "[B,N,C,Hf]",
      /*required=*/true, /*target_side_only=*/false,
      {"B", "N", "C", "Hf", "Df", "K"}));
  out.constraints.push_back("context_mode=channel_context_strict");
  out.constraints.push_back("each target channel consumes its own context row");
  out.constraints.push_back(
      "global_context mode must use a different dock/assembly identity");
  wa::validate_wikimyei_assembly(out);
  return out;
}

[[nodiscard]] inline cuwacunu::wikimyei::assembly::wikimyei_assembly_t
make_channel_context_plus_global_mdn_assembly(
    std::string component_id = "channel_context_plus_global_mdn_v1",
    std::string version_token =
        "wikimyei.inference.expected_value.mdn.plus_global.v1") {
  namespace wa = cuwacunu::wikimyei::assembly;

  wa::wikimyei_assembly_t out{};
  out.family = "wikimyei.inference.expected_value.mdn";
  out.component_id = std::move(component_id);
  out.version_token = std::move(version_token);
  out.trainability = wa::assembly_trainability_t::trainable;
  out.docks.push_back(wa::make_dock(
      "channel_node_representation", wa::dock_direction_t::consumes,
      wa::dock_role_t::conditioning, wa::dock_domain_t::node_representation,
      "graph_order.channel_node_representation.v1", "[B,N,C,De]", "[B,N,C]",
      /*required=*/true, /*target_side_only=*/false, {"B", "N", "C", "De"}));
  out.docks.push_back(wa::make_dock(
      "global_node_representation", wa::dock_direction_t::consumes,
      wa::dock_role_t::conditioning, wa::dock_domain_t::node_representation,
      "graph_order.global_node_representation.v1", "[B,N,Dg]", "[B,N]",
      /*required=*/true, /*target_side_only=*/false, {"B", "N", "Dg"}));
  out.docks.push_back(wa::make_dock(
      "future_node_lifted_state", wa::dock_direction_t::consumes,
      wa::dock_role_t::supervised_target,
      wa::dock_domain_t::future_node_lifted_state, "graph_order.kline.v1",
      "[B,C,Hf,N,9]", "[B,C,Hf,N,9]", /*required=*/true,
      /*target_side_only=*/true, {"B", "C", "Hf", "N", "F"}));
  out.docks.push_back(wa::make_dock(
      "future_channel_node_distribution", wa::dock_direction_t::produces,
      wa::dock_role_t::output, wa::dock_domain_t::future_node_distribution,
      "graph_order.channel_node_future_distribution.v1",
      "log_pi[B,N,C,Hf,K];mu/sigma[B,N,C,Hf,K,Df]", "[B,N,C,Hf]",
      /*required=*/true, /*target_side_only=*/false,
      {"B", "N", "C", "Hf", "Df", "K"}));
  out.constraints.push_back("context_mode=channel_context_plus_global");
  out.constraints.push_back("target channel consumes its own context row plus "
                            "explicit global branch");
  out.constraints.push_back(
      "global branch must be produced after channel representation exists");
  wa::validate_wikimyei_assembly(out);
  return out;
}

} // namespace cuwacunu::wikimyei::inference::expected_value::mdn
