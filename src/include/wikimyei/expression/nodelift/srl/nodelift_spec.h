// SPDX-License-Identifier: MIT
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "piaabo/parse/simple_kv_block.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "wikimyei/expression/nodelift/srl/synthetic_reference_lift.h"

namespace cuwacunu::wikimyei::expression::nodelift::srl {

enum class future_lift_policy_t {
  disabled,
  target_side,
};

struct nodelift_srl_spec_t {
  std::string version_token{kCanonicalToken};
  std::string component_assembly_id{};
  int64_t feature_width{kFeatureWidth};
  std::vector<int64_t> price_coords{kDefaultPriceCoords.begin(),
                                    kDefaultPriceCoords.end()};
  std::vector<int64_t> activity_coords{kDefaultActivityCoords.begin(),
                                       kDefaultActivityCoords.end()};
  gauge_policy_t gauge_policy{gauge_policy_t::uniform_per_component};
  precision_policy_t precision_policy{precision_policy_t::identity};
  activity_mode_t activity_mode{activity_mode_t::support_mean};
  future_lift_policy_t future_lift_policy{future_lift_policy_t::target_side};
  bool return_activity_total{true};
  bool return_activity_support{true};
  bool return_activity_coverage{true};
  bool return_coarse_masks{true};
  double eps{1e-12};
  double activity_max_exp_arg{40.0};
};

namespace nodelift_spec_detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

template <std::size_t N>
[[nodiscard]] inline std::vector<int64_t>
vector_from_array(const std::array<std::int64_t, N> &values) {
  std::vector<int64_t> out;
  out.reserve(N);
  for (const auto value : values) {
    out.push_back(static_cast<int64_t>(value));
  }
  return out;
}

template <std::size_t N>
[[nodiscard]] inline std::array<int64_t, N>
to_array(const std::vector<int64_t> &values, const char *field) {
  if (values.size() != N) {
    throw std::runtime_error(std::string("[nodelift_spec] ") + field +
                             " has unexpected width");
  }
  std::array<int64_t, N> out{};
  for (std::size_t i = 0; i < N; ++i) {
    out[i] = values[i];
  }
  return out;
}

inline void validate_unique_coords(const std::vector<int64_t> &coords,
                                   int64_t expected_width,
                                   int64_t feature_width, const char *field) {
  if (static_cast<int64_t>(coords.size()) != expected_width) {
    throw std::runtime_error(std::string("[nodelift_spec] ") + field +
                             " has unexpected coordinate count");
  }
  std::unordered_set<int64_t> seen;
  seen.reserve(coords.size());
  for (const int64_t coord : coords) {
    if (coord < 0 || coord >= feature_width) {
      throw std::runtime_error(std::string("[nodelift_spec] ") + field +
                               " coordinate out of range");
    }
    if (!seen.insert(coord).second) {
      throw std::runtime_error(std::string("[nodelift_spec] ") + field +
                               " contains duplicate coordinates");
    }
  }
}

[[nodiscard]] inline std::vector<int64_t>
parse_coord_selector(std::string value, const char *field) {
  const auto token = kv::lowercase(kv::trim(value));
  if (token == "kline_price" || token == "kline_price_features" ||
      token == "kline.price" || token == "kline.price_features") {
    if (std::string(field) != "PRICE_COORDS") {
      throw std::runtime_error(
          std::string("[nodelift_spec] ") + field +
          " cannot use the kline price coordinate selector");
    }
    return vector_from_array(
        cuwacunu::ujcamei::source::registry::types::kKlinePriceFeatureCoords);
  }
  if (token == "kline_activity" || token == "kline_activity_features" ||
      token == "kline.activity" || token == "kline.activity_features") {
    if (std::string(field) != "ACTIVITY_COORDS") {
      throw std::runtime_error(
          std::string("[nodelift_spec] ") + field +
          " cannot use the kline activity coordinate selector");
    }
    return vector_from_array(cuwacunu::ujcamei::source::registry::types::
                                 kKlineActivityFeatureCoords);
  }
  try {
    return kv::parse_i64_list(value);
  } catch (const std::exception &) {
    throw std::runtime_error(
        std::string("[nodelift_spec] invalid ") + field +
        " value; use kline_price/kline_activity or an exact kline registry "
        "coordinate list");
  }
}

template <std::size_t N>
inline void
validate_matches_kline_registry(const std::vector<int64_t> &coords,
                                const std::array<std::int64_t, N> &expected,
                                const char *field, const char *registry_name) {
  if (coords != vector_from_array(expected)) {
    throw std::runtime_error(std::string("[nodelift_spec] ") + field +
                             " must match " + registry_name +
                             " from kline_feature_registry.h");
  }
}

[[nodiscard]] inline gauge_policy_t parse_gauge_policy(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "uniform_per_component") {
    return gauge_policy_t::uniform_per_component;
  }
  throw std::runtime_error("[nodelift_spec] invalid GAUGE_POLICY: " + value);
}

[[nodiscard]] inline precision_policy_t
parse_precision_policy(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "identity") {
    return precision_policy_t::identity;
  }
  throw std::runtime_error("[nodelift_spec] invalid PRECISION_POLICY: " +
                           value);
}

[[nodiscard]] inline activity_mode_t parse_activity_mode(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "support_mean") {
    return activity_mode_t::support_mean;
  }
  throw std::runtime_error("[nodelift_spec] invalid ACTIVITY_MODE: " + value);
}

[[nodiscard]] inline future_lift_policy_t
parse_future_lift_policy(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "target_side") {
    return future_lift_policy_t::target_side;
  }
  if (value == "disabled") {
    return future_lift_policy_t::disabled;
  }
  throw std::runtime_error("[nodelift_spec] invalid LIFT_FUTURE: " + value);
}

} // namespace nodelift_spec_detail

inline void validate_nodelift_srl_spec(const nodelift_srl_spec_t &spec) {
  if (spec.version_token != kCanonicalToken) {
    throw std::runtime_error("[nodelift_spec] unsupported version token");
  }
  if (spec.component_assembly_id.empty()) {
    throw std::runtime_error("[nodelift_spec] component_assembly_id is required");
  }
  if (spec.feature_width != kFeatureWidth) {
    throw std::runtime_error("[nodelift_spec] feature_width must be 9");
  }
  nodelift_spec_detail::validate_unique_coords(
      spec.price_coords, kPriceWidth, spec.feature_width, "PRICE_COORDS");
  nodelift_spec_detail::validate_unique_coords(
      spec.activity_coords, kActivityWidth, spec.feature_width,
      "ACTIVITY_COORDS");
  nodelift_spec_detail::validate_matches_kline_registry(
      spec.price_coords,
      cuwacunu::ujcamei::source::registry::types::kKlinePriceFeatureCoords,
      "PRICE_COORDS", "kKlinePriceFeatureCoords");
  nodelift_spec_detail::validate_matches_kline_registry(
      spec.activity_coords,
      cuwacunu::ujcamei::source::registry::types::kKlineActivityFeatureCoords,
      "ACTIVITY_COORDS", "kKlineActivityFeatureCoords");
  if (spec.eps <= 0.0) {
    throw std::runtime_error("[nodelift_spec] EPS must be > 0");
  }
  if (spec.activity_max_exp_arg <= 0.0) {
    throw std::runtime_error(
        "[nodelift_spec] ACTIVITY_MAX_EXP_ARG must be > 0");
  }
}

[[nodiscard]] inline nodelift_srl_spec_t
decode_nodelift_srl_spec_from_dsl(const std::string &dsl_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(dsl_text, "NODELIFT_SRL");
  nodelift_srl_spec_t spec{};
  spec.version_token = kv::optional(block, "VERSION", spec.version_token);
  spec.component_assembly_id = kv::required(block, "COMPONENT_ASSEMBLY_ID");
  spec.feature_width = kv::parse_i64(
      kv::optional(block, "FEATURE_WIDTH", std::to_string(spec.feature_width)));
  spec.price_coords = nodelift_spec_detail::parse_coord_selector(
      kv::required(block, "PRICE_COORDS"), "PRICE_COORDS");
  spec.activity_coords = nodelift_spec_detail::parse_coord_selector(
      kv::required(block, "ACTIVITY_COORDS"), "ACTIVITY_COORDS");
  spec.gauge_policy = nodelift_spec_detail::parse_gauge_policy(
      kv::optional(block, "GAUGE_POLICY", "uniform_per_component"));
  spec.precision_policy = nodelift_spec_detail::parse_precision_policy(
      kv::optional(block, "PRECISION_POLICY", "identity"));
  spec.activity_mode = nodelift_spec_detail::parse_activity_mode(
      kv::optional(block, "ACTIVITY_MODE", "support_mean"));
  spec.future_lift_policy = nodelift_spec_detail::parse_future_lift_policy(
      kv::optional(block, "LIFT_FUTURE", "target_side"));
  spec.return_activity_total =
      kv::parse_bool(kv::optional(block, "RETURN_ACTIVITY_TOTAL", "true"));
  spec.return_activity_support =
      kv::parse_bool(kv::optional(block, "RETURN_ACTIVITY_SUPPORT", "true"));
  spec.return_activity_coverage =
      kv::parse_bool(kv::optional(block, "RETURN_ACTIVITY_COVERAGE", "true"));
  spec.return_coarse_masks =
      kv::parse_bool(kv::optional(block, "RETURN_COARSE_MASKS", "true"));
  spec.eps = kv::parse_double(kv::optional(block, "EPS", "0.000000000001"));
  spec.activity_max_exp_arg =
      kv::parse_double(kv::optional(block, "ACTIVITY_MAX_EXP_ARG", "40.0"));
  validate_nodelift_srl_spec(spec);
  return spec;
}

[[nodiscard]] inline nodelift_options_t
nodelift_options_from_spec(const nodelift_srl_spec_t &spec) {
  validate_nodelift_srl_spec(spec);
  nodelift_options_t out{};
  out.price_coords =
      nodelift_spec_detail::to_array<static_cast<std::size_t>(kPriceWidth)>(
          spec.price_coords, "PRICE_COORDS");
  out.activity_coords =
      nodelift_spec_detail::to_array<static_cast<std::size_t>(kActivityWidth)>(
          spec.activity_coords, "ACTIVITY_COORDS");
  out.gauge_policy = spec.gauge_policy;
  out.precision_policy = spec.precision_policy;
  out.activity_mode = spec.activity_mode;
  out.return_activity_total = spec.return_activity_total;
  out.return_activity_support = spec.return_activity_support;
  out.return_activity_coverage = spec.return_activity_coverage;
  out.return_coarse_masks = spec.return_coarse_masks;
  out.eps = spec.eps;
  out.activity_max_exp_arg = spec.activity_max_exp_arg;
  return out;
}

[[nodiscard]] inline bool lift_future_enabled(const nodelift_srl_spec_t &spec) {
  validate_nodelift_srl_spec(spec);
  return spec.future_lift_policy == future_lift_policy_t::target_side;
}

} // namespace cuwacunu::wikimyei::expression::nodelift::srl
