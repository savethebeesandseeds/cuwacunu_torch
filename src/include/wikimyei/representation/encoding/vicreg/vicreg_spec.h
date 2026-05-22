// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include "piaabo/parse/simple_kv_block.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "wikimyei/representation/encoding/vicreg/node_stream_adapter.h"

namespace cuwacunu::wikimyei::representation::encoding::vicreg {

enum class vicreg_input_route_t {
  node_stream,
};

enum class vicreg_mask_profile_t {
  all_9,
  price_only,
  close_only,
  activity_only,
  custom,
};

struct vicreg_node_representation_spec_t {
  std::string version_token{"wikimyei.representation.vicreg.v1"};
  std::string component_id{};
  vicreg_input_route_t input_route{vicreg_input_route_t::node_stream};
  int64_t input_width{cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth};

  int64_t encoding_dim{0};
  int64_t channel_expansion_dim{0};
  int64_t fused_feature_dim{0};
  int64_t encoder_hidden_dim{0};
  int64_t encoder_depth{0};
  vicreg_mask_profile_t mask_profile{vicreg_mask_profile_t::all_9};
  std::vector<int64_t> required_feature_coords{
      cuwacunu::wikimyei::representation::encoding::vicreg::
          node_vicreg_all_9_feature_coords()};

  std::string dtype{"float32"};
  std::string device{"cpu"};
};

namespace vicreg_spec_detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline vicreg_input_route_t parse_input_route(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "node_stream") {
    return vicreg_input_route_t::node_stream;
  }
  throw std::runtime_error("[vicreg_spec] invalid INPUT_ROUTE: " + value);
}

[[nodiscard]] inline vicreg_mask_profile_t
parse_mask_profile(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "all_9") {
    return vicreg_mask_profile_t::all_9;
  }
  if (value == "price_only") {
    return vicreg_mask_profile_t::price_only;
  }
  if (value == "close_only") {
    return vicreg_mask_profile_t::close_only;
  }
  if (value == "activity_only") {
    return vicreg_mask_profile_t::activity_only;
  }
  if (value == "custom") {
    return vicreg_mask_profile_t::custom;
  }
  throw std::runtime_error("[vicreg_spec] invalid MASK_PROFILE: " + value);
}

[[nodiscard]] inline std::vector<int64_t>
default_coords_for_profile(vicreg_mask_profile_t profile) {
  switch (profile) {
  case vicreg_mask_profile_t::all_9:
    return node_vicreg_all_9_feature_coords();
  case vicreg_mask_profile_t::price_only:
    return node_vicreg_price_feature_coords();
  case vicreg_mask_profile_t::close_only:
    return node_vicreg_close_feature_coords();
  case vicreg_mask_profile_t::activity_only:
    return node_vicreg_activity_feature_coords();
  case vicreg_mask_profile_t::custom:
    return {};
  }
  throw std::runtime_error("[vicreg_spec] unknown mask profile");
}

inline void validate_coords(const std::vector<int64_t> &coords, int64_t width) {
  if (coords.empty()) {
    throw std::runtime_error(
        "[vicreg_spec] required feature coordinates are empty");
  }
  std::unordered_set<int64_t> seen;
  seen.reserve(coords.size());
  for (const int64_t coord : coords) {
    if (coord < 0 || coord >= width) {
      throw std::runtime_error("[vicreg_spec] feature coordinate out of range");
    }
    if (!seen.insert(coord).second) {
      throw std::runtime_error("[vicreg_spec] duplicate feature coordinate");
    }
  }
}

[[nodiscard]] inline bool valid_dtype(std::string value) {
  value = kv::lowercase(kv::trim(value));
  return value == "float32" || value == "float64";
}

[[nodiscard]] inline bool valid_device(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "cpu" || value == "cuda") {
    return true;
  }
  if (value.rfind("cuda:", 0) != 0) {
    return false;
  }
  value = value.substr(5);
  return !value.empty() &&
         std::all_of(value.begin(), value.end(),
                     [](unsigned char ch) { return std::isdigit(ch) != 0; });
}

} // namespace vicreg_spec_detail

inline void validate_vicreg_node_representation_spec(
    const vicreg_node_representation_spec_t &spec) {
  if (spec.version_token != "wikimyei.representation.vicreg.v1") {
    throw std::runtime_error("[vicreg_spec] unsupported version token");
  }
  if (spec.component_id.empty()) {
    throw std::runtime_error("[vicreg_spec] component_id is required");
  }
  if (spec.input_route != vicreg_input_route_t::node_stream) {
    throw std::runtime_error("[vicreg_spec] v1 requires node_stream input");
  }
  if (spec.input_width !=
      cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth) {
    throw std::runtime_error("[vicreg_spec] input_width must be 9 for v1");
  }
  if (spec.encoding_dim <= 0 || spec.channel_expansion_dim <= 0 ||
      spec.fused_feature_dim <= 0 || spec.encoder_hidden_dim <= 0 ||
      spec.encoder_depth <= 0) {
    throw std::runtime_error("[vicreg_spec] encoder dimensions must be > 0");
  }
  vicreg_spec_detail::validate_coords(spec.required_feature_coords,
                                      spec.input_width);
  if (!vicreg_spec_detail::valid_dtype(spec.dtype)) {
    throw std::runtime_error("[vicreg_spec] DTYPE must be float32 or float64");
  }
  if (!vicreg_spec_detail::valid_device(spec.device)) {
    throw std::runtime_error(
        "[vicreg_spec] DEVICE must be cpu, cuda, or cuda:N");
  }
}

inline void
decode_vicreg_net_into_spec(const std::string &net_text,
                            vicreg_node_representation_spec_t &spec) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(net_text, "VICREG_NET");
  spec.encoding_dim = kv::parse_i64(kv::required(block, "ENCODING_DIM"));
  spec.channel_expansion_dim =
      kv::parse_i64(kv::required(block, "CHANNEL_EXPANSION_DIM"));
  spec.fused_feature_dim =
      kv::parse_i64(kv::required(block, "FUSED_FEATURE_DIM"));
  spec.encoder_hidden_dim =
      kv::parse_i64(kv::required(block, "ENCODER_HIDDEN_DIM"));
  spec.encoder_depth = kv::parse_i64(kv::required(block, "ENCODER_DEPTH"));
}

[[nodiscard]] inline vicreg_node_representation_spec_t
decode_vicreg_node_representation_spec_from_split_dsl(
    const std::string &dsl_text, const std::string &net_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(dsl_text, "VICREG");
  vicreg_node_representation_spec_t spec{};
  spec.version_token = kv::optional(block, "VERSION", spec.version_token);
  spec.component_id = kv::required(block, "COMPONENT_ID");
  spec.input_route = vicreg_spec_detail::parse_input_route(
      kv::optional(block, "INPUT_ROUTE", "node_stream"));
  spec.input_width = kv::parse_i64(
      kv::optional(block, "INPUT_WIDTH", std::to_string(spec.input_width)));
  spec.mask_profile = vicreg_spec_detail::parse_mask_profile(
      kv::optional(block, "MASK_PROFILE", "all_9"));
  const auto coord_value = kv::optional(block, "REQUIRED_FEATURE_COORDS", "");
  spec.required_feature_coords =
      coord_value.empty()
          ? vicreg_spec_detail::default_coords_for_profile(spec.mask_profile)
          : kv::parse_i64_list(coord_value);
  spec.dtype = kv::optional(block, "DTYPE", spec.dtype);
  spec.device = kv::optional(block, "DEVICE", spec.device);
  decode_vicreg_net_into_spec(net_text, spec);
  validate_vicreg_node_representation_spec(spec);
  return spec;
}

[[nodiscard]] inline node_vicreg_adapter_options_t
node_adapter_options_from_vicreg_spec(
    const vicreg_node_representation_spec_t &spec, bool training) {
  validate_vicreg_node_representation_spec(spec);
  auto out = training ? node_vicreg_training_adapter_options()
                      : node_vicreg_encoding_adapter_options();
  out.mask_policy = node_mask_reduce_policy_t::required_features;
  out.required_feature_coords = spec.required_feature_coords;
  return out;
}

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg
