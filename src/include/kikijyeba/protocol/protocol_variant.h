// SPDX-License-Identifier: MIT
#pragma once

#include <sstream>
#include <stdexcept>
#include <string>

#include "piaabo/digest/sha256.h"
#include "piaabo/parse/simple_kv_block.h"

namespace cuwacunu::kikijyeba::protocol {

enum class protocol_representation_family_t {
  vicreg,
  mtf_jepa_mae_vicreg,
};

struct protocol_no_lookahead_contract_t {
  std::string contract_id{};
  std::string contract_digest{};
  std::string certificate_schema{};
  std::string influence_schema{};
  std::string frontier_unit{};
  std::string serving_order{};
  std::string visibility_policy{};
  std::string derived_artifact_rule{};
  std::string checkpoint_rule{};
  std::string publish_rule{};
  std::string bootstrap_policy{};
  std::string research_policy{};
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
  std::string observer_family{"wikimyei.observer.belief"};
  std::string allocation_policy_family{
      "wikimyei.policy.portfolio.spot_distributional_utility"};
  std::string policy_component_family{
      "wikimyei.policy.portfolio.graph_node_allocation"};
  std::string representation_contract{
      "graph_order.channel_node_representation.v1"};
  protocol_no_lookahead_contract_t no_lookahead_contract{};
};

namespace protocol_variant_detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

inline void append_canonical_field(std::ostringstream &out,
                                   const std::string &key,
                                   const std::string &value) {
  out << key << "=" << value.size() << ":" << value << "\n";
}

[[nodiscard]] inline std::string canonical_no_lookahead_contract_text(
    const protocol_no_lookahead_contract_t &contract) {
  std::ostringstream out;
  out << "kikijyeba.protocol.no_lookahead_contract.canonical.v1\n";
  append_canonical_field(out, "CONTRACT_ID", kv::trim(contract.contract_id));
  append_canonical_field(out, "CERTIFICATE_SCHEMA",
                         kv::trim(contract.certificate_schema));
  append_canonical_field(out, "INFLUENCE_SCHEMA",
                         kv::trim(contract.influence_schema));
  append_canonical_field(out, "FRONTIER_UNIT",
                         kv::trim(contract.frontier_unit));
  append_canonical_field(out, "SERVING_ORDER",
                         kv::trim(contract.serving_order));
  append_canonical_field(out, "VISIBILITY_POLICY",
                         kv::trim(contract.visibility_policy));
  append_canonical_field(out, "DERIVED_ARTIFACT_RULE",
                         kv::trim(contract.derived_artifact_rule));
  append_canonical_field(out, "CHECKPOINT_RULE",
                         kv::trim(contract.checkpoint_rule));
  append_canonical_field(out, "PUBLISH_RULE", kv::trim(contract.publish_rule));
  append_canonical_field(out, "BOOTSTRAP_POLICY",
                         kv::trim(contract.bootstrap_policy));
  append_canonical_field(out, "RESEARCH_POLICY",
                         kv::trim(contract.research_policy));
  return out.str();
}

[[nodiscard]] inline std::string derive_no_lookahead_contract_digest(
    const protocol_no_lookahead_contract_t &contract) {
  return cuwacunu::piaabo::digest::sha256_hex(
      canonical_no_lookahead_contract_text(contract));
}

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

[[nodiscard]] inline bool protocol_no_lookahead_contract_declared(
    const protocol_no_lookahead_contract_t &contract) {
  return !contract.contract_id.empty() ||
         !contract.certificate_schema.empty() ||
         !contract.influence_schema.empty() ||
         !contract.frontier_unit.empty() || !contract.serving_order.empty() ||
         !contract.visibility_policy.empty() ||
         !contract.derived_artifact_rule.empty() ||
         !contract.checkpoint_rule.empty() || !contract.publish_rule.empty() ||
         !contract.bootstrap_policy.empty() ||
         !contract.research_policy.empty();
}

inline void validate_protocol_no_lookahead_contract(
    const protocol_no_lookahead_contract_t &contract) {
  namespace kv = protocol_variant_detail::kv;
  if (kv::trim(contract.contract_id).empty()) {
    throw std::runtime_error(
        "[protocol_variant] NO_LOOKAHEAD_CONTRACT CONTRACT_ID is required");
  }
  if (kv::trim(contract.certificate_schema).empty()) {
    throw std::runtime_error("[protocol_variant] NO_LOOKAHEAD_CONTRACT "
                             "CERTIFICATE_SCHEMA is required");
  }
  if (kv::trim(contract.influence_schema).empty()) {
    throw std::runtime_error("[protocol_variant] NO_LOOKAHEAD_CONTRACT "
                             "INFLUENCE_SCHEMA is required");
  }
  if (kv::trim(contract.frontier_unit).empty()) {
    throw std::runtime_error(
        "[protocol_variant] NO_LOOKAHEAD_CONTRACT FRONTIER_UNIT is required");
  }
  if (kv::trim(contract.serving_order).empty()) {
    throw std::runtime_error(
        "[protocol_variant] NO_LOOKAHEAD_CONTRACT SERVING_ORDER is required");
  }
  if (kv::trim(contract.visibility_policy).empty()) {
    throw std::runtime_error("[protocol_variant] NO_LOOKAHEAD_CONTRACT "
                             "VISIBILITY_POLICY is required");
  }
  if (kv::trim(contract.derived_artifact_rule).empty()) {
    throw std::runtime_error("[protocol_variant] NO_LOOKAHEAD_CONTRACT "
                             "DERIVED_ARTIFACT_RULE is required");
  }
  if (kv::trim(contract.checkpoint_rule).empty()) {
    throw std::runtime_error(
        "[protocol_variant] NO_LOOKAHEAD_CONTRACT CHECKPOINT_RULE is required");
  }
  if (kv::trim(contract.publish_rule).empty()) {
    throw std::runtime_error(
        "[protocol_variant] NO_LOOKAHEAD_CONTRACT PUBLISH_RULE is required");
  }
  if (kv::trim(contract.bootstrap_policy).empty()) {
    throw std::runtime_error("[protocol_variant] NO_LOOKAHEAD_CONTRACT "
                             "BOOTSTRAP_POLICY is required");
  }
  if (kv::trim(contract.research_policy).empty()) {
    throw std::runtime_error(
        "[protocol_variant] NO_LOOKAHEAD_CONTRACT RESEARCH_POLICY is required");
  }
}

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
  if (kv::trim(variant.observer_family) != "wikimyei.observer.belief") {
    throw std::runtime_error(
        "[protocol_variant] OBSERVER must be wikimyei.observer.belief");
  }
  if (kv::trim(variant.allocation_policy_family) !=
      "wikimyei.policy.portfolio.spot_distributional_utility") {
    throw std::runtime_error(
        "[protocol_variant] ALLOCATION_POLICY must be "
        "wikimyei.policy.portfolio.spot_distributional_utility");
  }
  if (kv::trim(variant.policy_component_family) !=
      "wikimyei.policy.portfolio.graph_node_allocation") {
    throw std::runtime_error("[protocol_variant] POLICY_COMPONENT must be "
                             "wikimyei.policy.portfolio.graph_node_allocation");
  }
  if (kv::trim(variant.representation_contract).empty()) {
    throw std::runtime_error(
        "[protocol_variant] REPRESENTATION_CONTRACT is required");
  }
  if (protocol_no_lookahead_contract_declared(variant.no_lookahead_contract)) {
    validate_protocol_no_lookahead_contract(variant.no_lookahead_contract);
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
  out.observer_family = kv::required(block, "OBSERVER");
  out.allocation_policy_family = kv::required(block, "ALLOCATION_POLICY");
  out.policy_component_family = kv::required(block, "POLICY_COMPONENT");
  out.representation_contract = kv::optional(block, "REPRESENTATION_CONTRACT",
                                             out.representation_contract);
  const auto blocks = kv::parse_blocks(dsl_text);
  const kv::block_t *no_lookahead_block = nullptr;
  for (const auto &candidate : blocks) {
    if (candidate.name != "NO_LOOKAHEAD_CONTRACT") {
      continue;
    }
    if (no_lookahead_block != nullptr) {
      throw std::runtime_error(
          "[protocol_variant] duplicate NO_LOOKAHEAD_CONTRACT block");
    }
    no_lookahead_block = &candidate;
  }
  if (no_lookahead_block != nullptr) {
    auto &contract = out.no_lookahead_contract;
    if (no_lookahead_block->values.find("CONTRACT_DIGEST") !=
        no_lookahead_block->values.end()) {
      throw std::runtime_error(
          "[protocol_variant] NO_LOOKAHEAD_CONTRACT CONTRACT_DIGEST is derived "
          "from the contract body; remove authored CONTRACT_DIGEST");
    }
    contract.contract_id = kv::optional(*no_lookahead_block, "CONTRACT_ID", "");
    contract.certificate_schema =
        kv::optional(*no_lookahead_block, "CERTIFICATE_SCHEMA", "");
    contract.influence_schema =
        kv::optional(*no_lookahead_block, "INFLUENCE_SCHEMA", "");
    contract.frontier_unit =
        kv::optional(*no_lookahead_block, "FRONTIER_UNIT", "");
    contract.serving_order =
        kv::optional(*no_lookahead_block, "SERVING_ORDER", "");
    contract.visibility_policy =
        kv::optional(*no_lookahead_block, "VISIBILITY_POLICY", "");
    contract.derived_artifact_rule =
        kv::optional(*no_lookahead_block, "DERIVED_ARTIFACT_RULE", "");
    contract.checkpoint_rule =
        kv::optional(*no_lookahead_block, "CHECKPOINT_RULE", "");
    contract.publish_rule =
        kv::optional(*no_lookahead_block, "PUBLISH_RULE", "");
    contract.bootstrap_policy =
        kv::optional(*no_lookahead_block, "BOOTSTRAP_POLICY", "");
    contract.research_policy =
        kv::optional(*no_lookahead_block, "RESEARCH_POLICY", "");
    contract.contract_digest =
        protocol_variant_detail::derive_no_lookahead_contract_digest(contract);
  }
  validate_protocol_variant(out);
  return out;
}

} // namespace cuwacunu::kikijyeba::protocol
