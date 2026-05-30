// SPDX-License-Identifier: MIT
#pragma once

#include <stdexcept>
#include <string>

#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::kikijyeba::protocol {

enum class protocol_representation_family_t {
  vicreg,
  mtf_jepa_mae_vicreg,
};

struct protocol_variant_t {
  std::string protocol_id{"cwu_01v"};
  std::string protocol_kind{"channel_graph_first"};
  std::string protocol_status{"active"};
  std::string successor_protocol{};
  std::string protocol_warning{};
  std::string graph_topology{"kikijyeba.topology.graph"};
  std::string nodelift_family{"wikimyei.expression.nodelift.srl"};
  protocol_representation_family_t representation_family{
      protocol_representation_family_t::vicreg};
  std::string inference_family{"wikimyei.inference.expected_value.mdn"};
  std::string representation_contract{
      "graph_order.channel_node_representation.v1"};
};

namespace protocol_variant_detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline protocol_representation_family_t
parse_representation_family(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "wikimyei.representation.encoding.vicreg" || value == "vicreg" ||
      value == "vicreg_representation") {
    return protocol_representation_family_t::vicreg;
  }
  if (value == "wikimyei.representation.encoding.mtf_jepa_mae_vicreg" ||
      value == "mtf_jepa_mae_vicreg" ||
      value == "mtf_jepa_mae_vicreg_representation" ||
      value == "mtf_jvmae_representation") {
    return protocol_representation_family_t::mtf_jepa_mae_vicreg;
  }
  throw std::runtime_error("[protocol_variant] invalid REPRESENTATION: " +
                           value);
}

} // namespace protocol_variant_detail

[[nodiscard]] inline const char *
protocol_representation_family_name(protocol_representation_family_t family) {
  switch (family) {
  case protocol_representation_family_t::vicreg:
    return "wikimyei.representation.encoding.vicreg";
  case protocol_representation_family_t::mtf_jepa_mae_vicreg:
    return "wikimyei.representation.encoding.mtf_jepa_mae_vicreg";
  }
  throw std::runtime_error("[protocol_variant] unknown representation family");
}

inline void validate_protocol_variant(const protocol_variant_t &variant) {
  namespace kv = protocol_variant_detail::kv;
  if (kv::trim(variant.protocol_id).empty()) {
    throw std::runtime_error("[protocol_variant] PROTOCOL_ID is required");
  }
  if (kv::lowercase(kv::trim(variant.protocol_kind)) != "channel_graph_first") {
    throw std::runtime_error(
        "[protocol_variant] PROTOCOL_KIND must be channel_graph_first");
  }
  const std::string status = kv::lowercase(kv::trim(variant.protocol_status));
  if (status != "active" && status != "legacy" && status != "deprecated") {
    throw std::runtime_error("[protocol_variant] PROTOCOL_STATUS must be "
                             "active, legacy, or deprecated");
  }
  if (kv::trim(variant.graph_topology) != "kikijyeba.topology.graph") {
    throw std::runtime_error(
        "[protocol_variant] GRAPH_TOPOLOGY must be kikijyeba.topology.graph");
  }
  if (kv::trim(variant.nodelift_family) != "wikimyei.expression.nodelift.srl") {
    throw std::runtime_error(
        "[protocol_variant] NODELIFT must be wikimyei.expression.nodelift.srl");
  }
  if (kv::trim(variant.inference_family) !=
      "wikimyei.inference.expected_value.mdn") {
    throw std::runtime_error("[protocol_variant] INFERENCE must be "
                             "wikimyei.inference.expected_value.mdn");
  }
  if (kv::trim(variant.representation_contract).empty()) {
    throw std::runtime_error(
        "[protocol_variant] REPRESENTATION_CONTRACT is required");
  }
  if (variant.representation_family ==
          protocol_representation_family_t::vicreg &&
      variant.representation_contract !=
          "graph_order.channel_node_representation.v1") {
    throw std::runtime_error("[protocol_variant] cwu_01v/VICReg-style "
                             "protocols require "
                             "graph_order.channel_node_representation.v1");
  }
}

[[nodiscard]] inline protocol_variant_t
decode_protocol_variant_from_dsl(const std::string &dsl_text) {
  namespace kv = protocol_variant_detail::kv;
  const auto &block = kv::single_block(dsl_text, "PROTOCOL");

  protocol_variant_t out{};
  out.protocol_id = kv::required(block, "PROTOCOL_ID");
  out.protocol_kind = kv::optional(block, "PROTOCOL_KIND", out.protocol_kind);
  out.protocol_status =
      kv::optional(block, "PROTOCOL_STATUS", out.protocol_status);
  out.successor_protocol =
      kv::optional(block, "SUCCESSOR_PROTOCOL", out.successor_protocol);
  out.protocol_warning =
      kv::optional(block, "PROTOCOL_WARNING", out.protocol_warning);
  out.graph_topology =
      kv::optional(block, "GRAPH_TOPOLOGY", out.graph_topology);
  out.nodelift_family = kv::optional(block, "NODELIFT", out.nodelift_family);
  out.representation_family =
      protocol_variant_detail::parse_representation_family(
          kv::required(block, "REPRESENTATION"));
  out.inference_family = kv::optional(block, "INFERENCE", out.inference_family);
  out.representation_contract = kv::optional(block, "REPRESENTATION_CONTRACT",
                                             out.representation_contract);
  validate_protocol_variant(out);
  return out;
}

} // namespace cuwacunu::kikijyeba::protocol
