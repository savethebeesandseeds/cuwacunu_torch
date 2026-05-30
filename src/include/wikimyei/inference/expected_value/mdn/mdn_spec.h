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
#include "wikimyei/inference/expected_value/mdn/channel_context_mdn_train_model.h"
#include "wikimyei/inference/expected_value/mdn/stream/mdn_adapter.h"

namespace cuwacunu::wikimyei::inference::expected_value::mdn {

enum class channel_mdn_context_mode_t {
  channel_context_strict,
  channel_context_plus_global,
};

enum class channel_mdn_target_domain_t {
  channel_node_future,
};

enum class channel_mdn_activity_target_t {
  node_feature_support_mean,
};

struct channel_mdn_spec_t {
  std::string version_token{"wikimyei.inference.expected_value.mdn.v1"};
  std::string component_assembly_id{};
  std::string input_representation_assembly_id{};
  channel_mdn_context_mode_t context_mode{
      channel_mdn_context_mode_t::channel_context_strict};
  channel_mdn_target_domain_t target_domain{
      channel_mdn_target_domain_t::channel_node_future};
  std::vector<int64_t> target_coords{0, 1, 2, 3, 4, 5, 6, 7, 8};
  stream::channel_target_mask_policy_t target_mask_policy{
      stream::channel_target_mask_policy_t::per_target_feature_valid};
  channel_mdn_activity_target_t activity_target{
      channel_mdn_activity_target_t::node_feature_support_mean};
  int64_t channel_count{0};
  int64_t future_horizon{0};
  int64_t mixture_count{0};
  int64_t hidden_width{0};
  int64_t residual_depth{0};
  int64_t feature_embedding_dim{0};
  int64_t channel_adapter_rank{0};
  int64_t global_context_dim{0};
  double sigma_min{1e-3};
  double sigma_max{0.0};
  double eps{1e-6};
};

namespace channel_mdn_spec_detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline channel_mdn_context_mode_t
parse_context_mode(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "channel_context_strict") {
    return channel_mdn_context_mode_t::channel_context_strict;
  }
  if (value == "channel_context_plus_global") {
    return channel_mdn_context_mode_t::channel_context_plus_global;
  }
  throw std::runtime_error("[channel_mdn_spec] invalid CONTEXT_MODE: " + value);
}

[[nodiscard]] inline channel_mdn_target_domain_t
parse_target_domain(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "channel_node_future") {
    return channel_mdn_target_domain_t::channel_node_future;
  }
  throw std::runtime_error("[channel_mdn_spec] invalid TARGET_DOMAIN: " +
                           value);
}

[[nodiscard]] inline stream::channel_target_mask_policy_t
parse_target_mask_policy(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "all_target_features_valid") {
    // Historical spelling accepted while active runtime now preserves the
    // per-feature target mask.
    return stream::channel_target_mask_policy_t::per_target_feature_valid;
  }
  if (value == "per_target_feature_valid") {
    return stream::channel_target_mask_policy_t::per_target_feature_valid;
  }
  throw std::runtime_error("[channel_mdn_spec] invalid TARGET_MASK_POLICY: " +
                           value);
}

[[nodiscard]] inline channel_mdn_activity_target_t
parse_activity_target(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "node_feature_support_mean") {
    return channel_mdn_activity_target_t::node_feature_support_mean;
  }
  throw std::runtime_error("[channel_mdn_spec] invalid ACTIVITY_TARGET: " +
                           value);
}

inline void validate_target_coords(const std::vector<int64_t> &coords) {
  if (coords.empty()) {
    throw std::runtime_error("[channel_mdn_spec] target coords are empty");
  }
  std::unordered_set<int64_t> seen;
  seen.reserve(coords.size());
  for (const int64_t coord : coords) {
    if (coord < 0 ||
        coord >=
            cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth) {
      throw std::runtime_error("[channel_mdn_spec] target coord out of range");
    }
    if (!seen.insert(coord).second) {
      throw std::runtime_error("[channel_mdn_spec] duplicate target coord");
    }
  }
}

} // namespace channel_mdn_spec_detail

inline void validate_channel_mdn_spec(const channel_mdn_spec_t &spec) {
  if (spec.version_token != "wikimyei.inference.expected_value.mdn.v1") {
    throw std::runtime_error("[channel_mdn_spec] unsupported version token");
  }
  if (spec.component_assembly_id.empty()) {
    throw std::runtime_error(
        "[channel_mdn_spec] component_assembly_id is required");
  }
  if (spec.input_representation_assembly_id.empty()) {
    throw std::runtime_error(
        "[channel_mdn_spec] input_representation_assembly_id is required");
  }
  if (spec.target_domain != channel_mdn_target_domain_t::channel_node_future) {
    throw std::runtime_error(
        "[channel_mdn_spec] v1 requires channel_node_future target");
  }
  if (spec.target_mask_policy !=
      stream::channel_target_mask_policy_t::per_target_feature_valid) {
    throw std::runtime_error(
        "[channel_mdn_spec] v1 requires per_target_feature_valid");
  }
  if (spec.activity_target !=
      channel_mdn_activity_target_t::node_feature_support_mean) {
    throw std::runtime_error(
        "[channel_mdn_spec] v1 requires node_feature_support_mean activity");
  }
  if (spec.channel_count <= 0 || spec.future_horizon <= 0 ||
      spec.mixture_count <= 0 || spec.hidden_width <= 0 ||
      spec.residual_depth < 0 || spec.feature_embedding_dim <= 0 ||
      spec.channel_adapter_rank <= 0 || spec.global_context_dim < 0) {
    throw std::runtime_error("[channel_mdn_spec] invalid dimensions");
  }
  if (spec.channel_adapter_rank > spec.hidden_width) {
    throw std::runtime_error(
        "[channel_mdn_spec] CHANNEL_ADAPTER_RANK must be <= HIDDEN_WIDTH");
  }
  if (spec.future_horizon != 1) {
    throw std::runtime_error(
        "[channel_mdn_spec] active channel MDN is one-step; FUTURE_HORIZON "
        "must be 1");
  }
  if (spec.context_mode == channel_mdn_context_mode_t::channel_context_strict &&
      spec.global_context_dim != 0) {
    throw std::runtime_error(
        "[channel_mdn_spec] strict channel context requires "
        "GLOBAL_CONTEXT_DIM=0");
  }
  if (spec.context_mode ==
          channel_mdn_context_mode_t::channel_context_plus_global &&
      spec.global_context_dim <= 0) {
    throw std::runtime_error(
        "[channel_mdn_spec] channel_context_plus_global requires "
        "GLOBAL_CONTEXT_DIM>0");
  }
  channel_mdn_spec_detail::validate_target_coords(spec.target_coords);
  if (spec.eps <= 0.0 || spec.sigma_min < 0.0 || spec.sigma_max < 0.0) {
    throw std::runtime_error("[channel_mdn_spec] invalid NLL options");
  }
  if (spec.sigma_max > 0.0 && spec.sigma_max < spec.sigma_min) {
    throw std::runtime_error(
        "[channel_mdn_spec] sigma_max must be >= sigma_min");
  }
}

inline void decode_channel_mdn_net_into_spec(const std::string &net_text,
                                             channel_mdn_spec_t &spec) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(net_text, "MDN_NET");
  spec.channel_count = kv::parse_i64(kv::required(block, "CHANNEL_COUNT"));
  spec.future_horizon = kv::parse_i64(kv::required(block, "FUTURE_HORIZON"));
  spec.mixture_count = kv::parse_i64(kv::required(block, "MIXTURE_COUNT"));
  spec.hidden_width = kv::parse_i64(kv::required(block, "HIDDEN_WIDTH"));
  spec.residual_depth = kv::parse_i64(kv::required(block, "RESIDUAL_DEPTH"));
  spec.feature_embedding_dim =
      kv::parse_i64(kv::required(block, "FEATURE_EMBEDDING_DIM"));
  spec.channel_adapter_rank =
      kv::parse_i64(kv::required(block, "CHANNEL_ADAPTER_RANK"));
  spec.global_context_dim =
      kv::parse_i64(kv::required(block, "GLOBAL_CONTEXT_DIM"));
}

[[nodiscard]] inline channel_mdn_spec_t
decode_channel_mdn_spec_from_split_dsl(const std::string &dsl_text,
                                       const std::string &net_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(dsl_text, "MDN");
  channel_mdn_spec_t spec{};
  spec.version_token = kv::required(block, "VERSION");
  spec.component_assembly_id = kv::required(block, "COMPONENT_ASSEMBLY_ID");
  spec.input_representation_assembly_id =
      kv::required(block, "INPUT_REPRESENTATION_ASSEMBLY_ID");
  spec.context_mode = channel_mdn_spec_detail::parse_context_mode(
      kv::required(block, "CONTEXT_MODE"));
  spec.target_domain = channel_mdn_spec_detail::parse_target_domain(
      kv::required(block, "TARGET_DOMAIN"));
  spec.target_coords = kv::parse_i64_list(kv::required(block, "TARGET_COORDS"));
  spec.target_mask_policy = channel_mdn_spec_detail::parse_target_mask_policy(
      kv::required(block, "TARGET_MASK_POLICY"));
  spec.activity_target = channel_mdn_spec_detail::parse_activity_target(
      kv::required(block, "ACTIVITY_TARGET"));
  spec.sigma_min = kv::parse_double(kv::required(block, "SIGMA_MIN"));
  spec.sigma_max = kv::parse_double(kv::required(block, "SIGMA_MAX"));
  spec.eps = kv::parse_double(kv::required(block, "EPS"));
  decode_channel_mdn_net_into_spec(net_text, spec);
  validate_channel_mdn_spec(spec);
  return spec;
}

[[nodiscard]] inline stream::channel_mdn_adapter_options_t
channel_mdn_adapter_options_from_spec(const channel_mdn_spec_t &spec) {
  validate_channel_mdn_spec(spec);
  stream::channel_mdn_adapter_options_t out{};
  out.target_mask_policy = spec.target_mask_policy;
  out.target_coords = spec.target_coords;
  return out;
}

[[nodiscard]] inline cuwacunu::wikimyei::inference::expected_value::mdn::
    MdnNllOptions
    channel_mdn_nll_options_from_spec(const channel_mdn_spec_t &spec) {
  validate_channel_mdn_spec(spec);
  cuwacunu::wikimyei::inference::expected_value::mdn::MdnNllOptions out{};
  out.eps = spec.eps;
  out.sigma_min = spec.sigma_min;
  out.sigma_max = spec.sigma_max;
  return out;
}

[[nodiscard]] inline channel_context_mdn_train_options_t
channel_context_mdn_train_options_from_spec(const channel_mdn_spec_t &spec) {
  channel_context_mdn_train_options_t out{};
  out.nll = channel_mdn_nll_options_from_spec(spec);
  return out;
}

} // namespace cuwacunu::wikimyei::inference::expected_value::mdn
