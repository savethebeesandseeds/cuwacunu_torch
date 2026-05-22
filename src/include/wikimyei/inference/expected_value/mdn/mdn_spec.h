// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "piaabo/parse/simple_kv_block.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "wikimyei/inference/expected_value/mdn/mixture_density_network_utils.h"
#include "wikimyei/inference/expected_value/mdn/stream/mdn_adapter.h"

namespace cuwacunu::wikimyei::inference::expected_value::mdn {

enum class target_domain_t {
  node_future,
};

enum class activity_target_t {
  node_feature_support_mean,
};

enum class head_policy_t {
  per_node,
};

struct mdn_spec_t {
  std::string version_token{"wikimyei.inference.expected_value.mdn.v1"};
  std::string component_id{};
  std::string input_representation_id{};
  target_domain_t target_domain{target_domain_t::node_future};
  std::vector<int64_t> target_coords{0, 1, 2, 3, 4, 5, 6, 7, 8};
  stream::target_mask_policy_t target_mask_policy{
      stream::target_mask_policy_t::all_target_features_valid};
  activity_target_t activity_target{
      activity_target_t::node_feature_support_mean};
  head_policy_t head_policy{head_policy_t::per_node};
  stream::context_reduction_policy_t context_reduction{
      stream::context_reduction_policy_t::last};
  int64_t mixture_count{0};
  int64_t hidden_width{0};
  int64_t residual_depth{0};
  double sigma_min{1e-3};
  double sigma_max{0.0};
  double eps{1e-6};
};

namespace mdn_spec_detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline target_domain_t parse_target_domain(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "node_future") {
    return target_domain_t::node_future;
  }
  throw std::runtime_error("[mdn_spec] invalid TARGET_DOMAIN: " + value);
}

[[nodiscard]] inline stream::target_mask_policy_t
parse_target_mask_policy(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "all_target_features_valid") {
    return stream::target_mask_policy_t::all_target_features_valid;
  }
  throw std::runtime_error("[mdn_spec] invalid TARGET_MASK_POLICY: " + value);
}

[[nodiscard]] inline activity_target_t
parse_activity_target(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "node_feature_support_mean") {
    return activity_target_t::node_feature_support_mean;
  }
  throw std::runtime_error("[mdn_spec] invalid ACTIVITY_TARGET: " + value);
}

[[nodiscard]] inline head_policy_t parse_head_policy(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "per_node") {
    return head_policy_t::per_node;
  }
  throw std::runtime_error("[mdn_spec] invalid HEAD_POLICY: " + value);
}

[[nodiscard]] inline stream::context_reduction_policy_t
parse_context_reduction(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "last") {
    return stream::context_reduction_policy_t::last;
  }
  if (value == "mean") {
    return stream::context_reduction_policy_t::mean;
  }
  throw std::runtime_error("[mdn_spec] invalid CONTEXT_REDUCTION: " + value);
}

inline void validate_target_coords(const std::vector<int64_t> &coords) {
  if (coords.empty()) {
    throw std::runtime_error("[mdn_spec] target_coords must not be empty");
  }
  if (coords.size() !=
      static_cast<std::size_t>(
          cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth)) {
    throw std::runtime_error(
        "[mdn_spec] v1 requires all 9 future node feature coordinates");
  }
  std::unordered_set<int64_t> seen;
  seen.reserve(coords.size());
  for (const int64_t coord : coords) {
    if (coord < 0 ||
        coord >= cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth) {
      throw std::runtime_error("[mdn_spec] target coord out of range");
    }
    if (!seen.insert(coord).second) {
      throw std::runtime_error("[mdn_spec] duplicate target coord");
    }
  }
  for (int64_t coord = 0;
       coord < cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth; ++coord) {
    if (seen.find(coord) == seen.end()) {
      throw std::runtime_error(
          "[mdn_spec] v1 target coords must contain every kline coord");
    }
  }
}

} // namespace mdn_spec_detail

inline void validate_mdn_spec(const mdn_spec_t &spec) {
  if (spec.version_token != "wikimyei.inference.expected_value.mdn.v1") {
    throw std::runtime_error("[mdn_spec] unsupported version token");
  }
  if (spec.component_id.empty()) {
    throw std::runtime_error("[mdn_spec] component_id is required");
  }
  if (spec.input_representation_id.empty()) {
    throw std::runtime_error("[mdn_spec] input_representation_id is required");
  }
  if (spec.target_domain != target_domain_t::node_future) {
    throw std::runtime_error("[mdn_spec] v1 requires node_future target");
  }
  if (spec.target_mask_policy !=
      stream::target_mask_policy_t::all_target_features_valid) {
    throw std::runtime_error(
        "[mdn_spec] v1 requires all_target_features_valid");
  }
  if (spec.activity_target != activity_target_t::node_feature_support_mean) {
    throw std::runtime_error(
        "[mdn_spec] v1 requires node_feature_support_mean activity "
        "targets");
  }
  if (spec.head_policy != head_policy_t::per_node) {
    throw std::runtime_error("[mdn_spec] v1 requires per_node heads");
  }
  mdn_spec_detail::validate_target_coords(spec.target_coords);
  if (spec.mixture_count <= 0 || spec.hidden_width <= 0 ||
      spec.residual_depth < 0) {
    throw std::runtime_error("[mdn_spec] invalid MDN architecture dimensions");
  }
  if (spec.eps <= 0.0 || spec.sigma_min < 0.0 || spec.sigma_max < 0.0) {
    throw std::runtime_error("[mdn_spec] invalid NLL scale options");
  }
  if (spec.sigma_max > 0.0 && spec.sigma_max < spec.sigma_min) {
    throw std::runtime_error("[mdn_spec] sigma_max must be >= sigma_min");
  }
}

inline void decode_mdn_net_into_spec(const std::string &net_text,
                                     mdn_spec_t &spec) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(net_text, "MDN_NET");
  spec.mixture_count = kv::parse_i64(kv::required(block, "MIXTURE_COUNT"));
  spec.hidden_width = kv::parse_i64(kv::required(block, "HIDDEN_WIDTH"));
  spec.residual_depth =
      kv::parse_i64(kv::optional(block, "RESIDUAL_DEPTH", "0"));
}

[[nodiscard]] inline mdn_spec_t
decode_mdn_spec_from_split_dsl(const std::string &dsl_text,
                               const std::string &net_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(dsl_text, "MDN");
  mdn_spec_t spec{};
  spec.version_token = kv::optional(block, "VERSION", spec.version_token);
  spec.component_id = kv::required(block, "COMPONENT_ID");
  spec.input_representation_id = kv::required(block, "INPUT_REPRESENTATION_ID");
  spec.target_domain = mdn_spec_detail::parse_target_domain(
      kv::optional(block, "TARGET_DOMAIN", "node_future"));
  spec.target_coords = kv::parse_i64_list(kv::required(block, "TARGET_COORDS"));
  spec.target_mask_policy = mdn_spec_detail::parse_target_mask_policy(
      kv::optional(block, "TARGET_MASK_POLICY", "all_target_features_valid"));
  spec.activity_target = mdn_spec_detail::parse_activity_target(
      kv::optional(block, "ACTIVITY_TARGET", "node_feature_support_mean"));
  spec.head_policy = mdn_spec_detail::parse_head_policy(
      kv::optional(block, "HEAD_POLICY", "per_node"));
  spec.context_reduction = mdn_spec_detail::parse_context_reduction(
      kv::optional(block, "CONTEXT_REDUCTION", "last"));
  spec.sigma_min = kv::parse_double(kv::optional(block, "SIGMA_MIN", "0.001"));
  spec.sigma_max = kv::parse_double(kv::optional(block, "SIGMA_MAX", "0.0"));
  spec.eps = kv::parse_double(kv::optional(block, "EPS", "0.000001"));
  decode_mdn_net_into_spec(net_text, spec);
  validate_mdn_spec(spec);
  return spec;
}

[[nodiscard]] inline stream::mdn_adapter_options_t
mdn_adapter_options_from_spec(const mdn_spec_t &spec) {
  validate_mdn_spec(spec);
  stream::mdn_adapter_options_t out{};
  out.context_reduction = spec.context_reduction;
  out.target_mask_policy = spec.target_mask_policy;
  out.target_coords = spec.target_coords;
  return out;
}

[[nodiscard]] inline cuwacunu::wikimyei::inference::expected_value::mdn::
    MdnNllOptions
    mdn_nll_options_from_spec(const mdn_spec_t &spec) {
  validate_mdn_spec(spec);
  cuwacunu::wikimyei::inference::expected_value::mdn::MdnNllOptions out{};
  out.eps = spec.eps;
  out.sigma_min = spec.sigma_min;
  out.sigma_max = spec.sigma_max;
  return out;
}

[[nodiscard]] inline std::string head_policy_name(head_policy_t policy) {
  switch (policy) {
  case head_policy_t::per_node:
    return "per_node";
  }
  return "unknown";
}

} // namespace cuwacunu::wikimyei::inference::expected_value::mdn
