// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

#include <torch/torch.h>

#include "piaabo/parse/simple_kv_block.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "wikimyei/representation/encoding/vicreg/channel_preserving_encoder.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_projector.h"
#include "wikimyei/representation/encoding/vicreg/vicreg_train_model.h"

namespace cuwacunu::wikimyei::representation::encoding::vicreg {

enum class vicreg_input_route_t {
  channel_node_stream,
};

struct vicreg_spec_t {
  std::string version_token{"wikimyei.representation.vicreg.v1"};
  std::string component_assembly_id{};
  vicreg_input_route_t input_route{vicreg_input_route_t::channel_node_stream};
  int64_t channel_count{0};
  int64_t history_length{0};
  int64_t input_width{
      cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth};

  cell_valid_policy_t cell_valid_policy{cell_valid_policy_t::required_features};
  std::vector<int64_t> required_feature_coords{0, 1, 2, 3};
  double min_valid_fraction{1.0};
  bool use_missingness_indicators{true};

  int64_t encoding_dim{0};
  int64_t feature_hidden_dim{0};
  int64_t temporal_depth{0};
  double recency_decay{0.85};
  int64_t vicreg_projector_dim{0};
  int64_t vicreg_projector_hidden_dim{0};
  int64_t vicreg_projector_depth{1};
  double vicreg_invariance_weight{25.0};
  double vicreg_variance_weight{25.0};
  double vicreg_covariance_weight{1.0};
  double vicreg_variance_floor{1.0};
  double vicreg_eps{1e-4};
  double global_aux_weight{0.0};
  int64_t min_valid_rows{2};
  bool skip_non_finite_loss{true};
  double jitter_std{0.01};
  double feature_dropout_prob{0.0};
  double history_dropout_prob{0.0};

  std::string dtype{"float32"};
  std::string device{"cpu"};
};

namespace vicreg_spec_detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline vicreg_input_route_t parse_input_route(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "channel_node_stream") {
    return vicreg_input_route_t::channel_node_stream;
  }
  throw std::runtime_error("[vicreg_spec] invalid INPUT_ROUTE: " + value);
}

[[nodiscard]] inline cell_valid_policy_t
parse_cell_valid_policy(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "any_feature") {
    return cell_valid_policy_t::any_feature;
  }
  if (value == "all_features") {
    return cell_valid_policy_t::all_features;
  }
  if (value == "required_features") {
    return cell_valid_policy_t::required_features;
  }
  if (value == "min_valid_fraction") {
    return cell_valid_policy_t::min_valid_fraction;
  }
  throw std::runtime_error("[vicreg_spec] invalid CELL_VALID_POLICY: " + value);
}

inline void validate_coords(const std::vector<int64_t> &coords, int64_t width) {
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

[[nodiscard]] inline torch::Dtype torch_dtype(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "float32") {
    return torch::kFloat32;
  }
  if (value == "float64") {
    return torch::kFloat64;
  }
  throw std::runtime_error("[vicreg_spec] unsupported dtype");
}

} // namespace vicreg_spec_detail

inline void validate_vicreg_spec(const vicreg_spec_t &spec) {
  if (spec.version_token != "wikimyei.representation.vicreg.v1") {
    throw std::runtime_error("[vicreg_spec] unsupported version token");
  }
  if (spec.component_assembly_id.empty()) {
    throw std::runtime_error("[vicreg_spec] component_assembly_id is required");
  }
  if (spec.input_route != vicreg_input_route_t::channel_node_stream) {
    throw std::runtime_error(
        "[vicreg_spec] v1 requires channel_node_stream input");
  }
  if (spec.channel_count <= 0 || spec.history_length <= 0 ||
      spec.input_width !=
          cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth ||
      spec.encoding_dim <= 0 || spec.feature_hidden_dim <= 0 ||
      spec.temporal_depth < 0 || spec.vicreg_projector_dim <= 0 ||
      spec.vicreg_projector_depth < 0) {
    throw std::runtime_error("[vicreg_spec] invalid dimensions");
  }
  if (spec.vicreg_projector_depth > 0 &&
      spec.vicreg_projector_hidden_dim <= 0) {
    throw std::runtime_error("[vicreg_spec] projector hidden dim must be > 0");
  }
  if (!(spec.recency_decay > 0.0 && spec.recency_decay <= 1.0) ||
      !(spec.min_valid_fraction > 0.0 && spec.min_valid_fraction <= 1.0) ||
      spec.vicreg_invariance_weight < 0.0 ||
      spec.vicreg_variance_weight < 0.0 ||
      spec.vicreg_covariance_weight < 0.0 ||
      !(spec.vicreg_variance_floor > 0.0) || !(spec.vicreg_eps > 0.0) ||
      spec.global_aux_weight < 0.0 || spec.min_valid_rows <= 0 ||
      spec.jitter_std < 0.0 || spec.feature_dropout_prob < 0.0 ||
      spec.feature_dropout_prob > 1.0 || spec.history_dropout_prob < 0.0 ||
      spec.history_dropout_prob > 1.0) {
    throw std::runtime_error("[vicreg_spec] invalid scalar option");
  }
  if (spec.cell_valid_policy == cell_valid_policy_t::required_features &&
      spec.required_feature_coords.empty()) {
    throw std::runtime_error(
        "[vicreg_spec] required feature coordinates are empty");
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

inline void decode_vicreg_net_into_spec(const std::string &net_text,
                                        vicreg_spec_t &spec) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(net_text, "VICREG_NET");
  spec.encoding_dim = kv::parse_i64(kv::required(block, "ENCODING_DIM"));
  spec.feature_hidden_dim =
      kv::parse_i64(kv::required(block, "FEATURE_HIDDEN_DIM"));
  spec.temporal_depth = kv::parse_i64(kv::required(block, "TEMPORAL_DEPTH"));
  spec.recency_decay = kv::parse_double(kv::required(block, "RECENCY_DECAY"));
  spec.vicreg_projector_dim =
      kv::parse_i64(kv::required(block, "VICREG_PROJECTOR_DIM"));
  spec.vicreg_projector_hidden_dim =
      kv::parse_i64(kv::required(block, "VICREG_PROJECTOR_HIDDEN_DIM"));
  spec.vicreg_projector_depth =
      kv::parse_i64(kv::required(block, "VICREG_PROJECTOR_DEPTH"));
  spec.vicreg_invariance_weight =
      kv::parse_double(kv::required(block, "VICREG_INVARIANCE_WEIGHT"));
  spec.vicreg_variance_weight =
      kv::parse_double(kv::required(block, "VICREG_VARIANCE_WEIGHT"));
  spec.vicreg_covariance_weight =
      kv::parse_double(kv::required(block, "VICREG_COVARIANCE_WEIGHT"));
  spec.vicreg_variance_floor =
      kv::parse_double(kv::required(block, "VICREG_VARIANCE_FLOOR"));
  spec.vicreg_eps = kv::parse_double(kv::required(block, "VICREG_EPS"));
  spec.global_aux_weight =
      kv::parse_double(kv::required(block, "GLOBAL_AUX_WEIGHT"));
  spec.min_valid_rows = kv::parse_i64(kv::required(block, "MIN_VALID_ROWS"));
  spec.skip_non_finite_loss =
      kv::parse_bool(kv::required(block, "SKIP_NON_FINITE_LOSS"));
  spec.jitter_std = kv::parse_double(kv::required(block, "JITTER_STD"));
  spec.feature_dropout_prob =
      kv::parse_double(kv::required(block, "FEATURE_DROPOUT_PROB"));
  spec.history_dropout_prob =
      kv::parse_double(kv::required(block, "HISTORY_DROPOUT_PROB"));
}

[[nodiscard]] inline vicreg_spec_t
decode_vicreg_spec_from_split_dsl(const std::string &dsl_text,
                                  const std::string &net_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(dsl_text, "VICREG");
  vicreg_spec_t spec{};
  spec.version_token = kv::required(block, "VERSION");
  spec.component_assembly_id = kv::required(block, "COMPONENT_ASSEMBLY_ID");
  spec.input_route =
      vicreg_spec_detail::parse_input_route(kv::required(block, "INPUT_ROUTE"));
  spec.channel_count = kv::parse_i64(kv::required(block, "CHANNEL_COUNT"));
  spec.history_length = kv::parse_i64(kv::required(block, "HISTORY_LENGTH"));
  spec.input_width = kv::parse_i64(kv::required(block, "INPUT_WIDTH"));
  spec.cell_valid_policy = vicreg_spec_detail::parse_cell_valid_policy(
      kv::required(block, "CELL_VALID_POLICY"));
  const auto coord_text = kv::required(block, "REQUIRED_FEATURE_COORDS");
  if (!coord_text.empty()) {
    spec.required_feature_coords = kv::parse_i64_list(coord_text);
  }
  spec.min_valid_fraction =
      kv::parse_double(kv::required(block, "MIN_VALID_FRACTION"));
  spec.use_missingness_indicators =
      kv::parse_bool(kv::required(block, "USE_MISSINGNESS_INDICATORS"));
  spec.dtype = kv::required(block, "DTYPE");
  spec.device = kv::required(block, "DEVICE");
  decode_vicreg_net_into_spec(net_text, spec);
  validate_vicreg_spec(spec);
  return spec;
}

[[nodiscard]] inline channel_preserving_encoder_options_t
channel_encoder_options_from_spec(const vicreg_spec_t &spec) {
  validate_vicreg_spec(spec);
  channel_preserving_encoder_options_t out{};
  out.channel_count = spec.channel_count;
  out.history_length = spec.history_length;
  out.input_width = spec.input_width;
  out.encoding_dim = spec.encoding_dim;
  out.feature_hidden_dim = spec.feature_hidden_dim;
  out.temporal_depth = spec.temporal_depth;
  out.recency_decay = spec.recency_decay;
  out.cell_valid_policy = spec.cell_valid_policy;
  out.required_feature_coords = spec.required_feature_coords;
  out.min_valid_fraction = spec.min_valid_fraction;
  out.use_missingness_indicators = spec.use_missingness_indicators;
  out.dtype = vicreg_spec_detail::torch_dtype(spec.dtype);
  out.device = torch::Device(spec.device);
  return out;
}

[[nodiscard]] inline vicreg_projector_options_t
channel_projector_options_from_spec(const vicreg_spec_t &spec) {
  validate_vicreg_spec(spec);
  vicreg_projector_options_t out{};
  out.input_dim = spec.encoding_dim;
  out.projector_dim = spec.vicreg_projector_dim;
  out.hidden_dim = spec.vicreg_projector_hidden_dim;
  out.depth = spec.vicreg_projector_depth;
  out.dtype = vicreg_spec_detail::torch_dtype(spec.dtype);
  out.device = torch::Device(spec.device);
  return out;
}

[[nodiscard]] inline vicreg_train_options_t
vicreg_train_options_from_spec(const vicreg_spec_t &spec) {
  validate_vicreg_spec(spec);
  vicreg_train_options_t out{};
  out.vicreg.invariance_weight = spec.vicreg_invariance_weight;
  out.vicreg.variance_weight = spec.vicreg_variance_weight;
  out.vicreg.covariance_weight = spec.vicreg_covariance_weight;
  out.vicreg.variance_floor = spec.vicreg_variance_floor;
  out.vicreg.eps = spec.vicreg_eps;
  out.vicreg.global_aux_weight = spec.global_aux_weight;
  out.min_valid_rows = spec.min_valid_rows;
  out.skip_non_finite_loss = spec.skip_non_finite_loss;
  out.jitter_std = spec.jitter_std;
  out.feature_dropout_prob = spec.feature_dropout_prob;
  out.history_dropout_prob = spec.history_dropout_prob;
  return out;
}

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg
