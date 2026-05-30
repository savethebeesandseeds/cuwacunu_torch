// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <torch/torch.h>

#include "piaabo/parse/simple_kv_block.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"

namespace cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg {

enum class mtf_jepa_mae_input_route_t {
  channel_node_stream,
};

struct mtf_jepa_mae_vicreg_spec_t {
  std::string version_token{"wikimyei.representation.mtf_jepa_mae_vicreg.v1"};
  std::string component_assembly_id{};
  mtf_jepa_mae_input_route_t input_route{
      mtf_jepa_mae_input_route_t::channel_node_stream};
  mtf_jepa_mae_vicreg_config_t config{};
};

namespace mtf_jepa_mae_vicreg_spec_detail {

namespace kv = cuwacunu::piaabo::parse::simple_kv;

[[nodiscard]] inline mtf_jepa_mae_input_route_t
parse_input_route(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "channel_node_stream") {
    return mtf_jepa_mae_input_route_t::channel_node_stream;
  }
  throw std::runtime_error("[mtf_jepa_mae_vicreg_spec] invalid INPUT_ROUTE: " +
                           value);
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
  throw std::runtime_error("[mtf_jepa_mae_vicreg_spec] unsupported DTYPE");
}

[[nodiscard]] inline torch::Device torch_device(std::string value) {
  value = kv::lowercase(kv::trim(value));
  if (value == "cpu") {
    return torch::Device(torch::kCPU);
  }
  if (value == "cuda") {
    return torch::Device(torch::kCUDA);
  }
  if (value.rfind("cuda:", 0) == 0) {
    return torch::Device(torch::kCUDA, static_cast<c10::DeviceIndex>(
                                           kv::parse_i64(value.substr(5))));
  }
  throw std::runtime_error("[mtf_jepa_mae_vicreg_spec] unsupported DEVICE");
}

} // namespace mtf_jepa_mae_vicreg_spec_detail

inline void
validate_mtf_jepa_mae_vicreg_spec(const mtf_jepa_mae_vicreg_spec_t &spec) {
  if (spec.version_token != "wikimyei.representation.mtf_jepa_mae_vicreg.v1") {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg_spec] unsupported version token");
  }
  if (spec.component_assembly_id.empty()) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg_spec] component_assembly_id is required");
  }
  if (spec.input_route != mtf_jepa_mae_input_route_t::channel_node_stream) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg_spec] v1 requires channel_node_stream input");
  }
  if (spec.config.input_width !=
      cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg_spec] INPUT_WIDTH must match kline feature "
        "width");
  }
  detail::validate_config(spec.config);
}

inline void
decode_mtf_jepa_mae_vicreg_net_into_spec(const std::string &net_text,
                                         mtf_jepa_mae_vicreg_spec_t &spec) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  const auto &block = kv::single_block(net_text, "MTF_JEPA_MAE_VICREG_NET");
  auto &cfg = spec.config;
  cfg.d_model = kv::parse_i64(kv::required(block, "D_MODEL"));
  cfg.latent_dim = kv::parse_i64(kv::required(block, "LATENT_DIM"));
  cfg.projector_dim = kv::parse_i64(kv::required(block, "PROJECTOR_DIM"));
  cfg.predictor_hidden_dim =
      kv::parse_i64(kv::required(block, "PREDICTOR_HIDDEN_DIM"));
  cfg.num_encoder_layers =
      kv::parse_i64(kv::required(block, "NUM_ENCODER_LAYERS"));
  cfg.num_predictor_layers =
      kv::parse_i64(kv::required(block, "NUM_PREDICTOR_LAYERS"));
  cfg.num_decoder_layers =
      kv::parse_i64(kv::required(block, "NUM_DECODER_LAYERS"));
  cfg.num_heads = kv::parse_i64(kv::required(block, "NUM_HEADS"));
  cfg.dropout = kv::parse_double(kv::required(block, "DROPOUT"));

  cfg.time_scales = kv::parse_i64_list(kv::required(block, "TIME_SCALES"));
  cfg.scale_strides = kv::parse_i64_list(kv::required(block, "SCALE_STRIDES"));
  cfg.use_frequency_tokens =
      kv::parse_bool(kv::required(block, "USE_FREQUENCY_TOKENS"));
  cfg.frequency_num_bins =
      kv::parse_i64(kv::required(block, "FREQUENCY_NUM_BINS"));
  cfg.frequency_log_magnitude =
      kv::parse_bool(kv::required(block, "FREQUENCY_LOG_MAGNITUDE"));

  cfg.mask_ratio_time =
      kv::parse_double(kv::required(block, "MASK_RATIO_TIME"));
  cfg.mask_ratio_frequency =
      kv::parse_double(kv::required(block, "MASK_RATIO_FREQUENCY"));
  cfg.mask_ratio_channel =
      kv::parse_double(kv::required(block, "MASK_RATIO_CHANNEL"));
  cfg.min_context_ratio =
      kv::parse_double(kv::required(block, "MIN_CONTEXT_RATIO"));

  cfg.lambda_jepa = kv::parse_double(kv::required(block, "LAMBDA_JEPA"));
  cfg.lambda_mae = kv::parse_double(kv::required(block, "LAMBDA_MAE"));
  cfg.lambda_tf_align =
      kv::parse_double(kv::required(block, "LAMBDA_TF_ALIGN"));
  cfg.lambda_vicreg = kv::parse_double(kv::required(block, "LAMBDA_VICREG"));
  cfg.lambda_global_vicreg =
      kv::parse_double(kv::required(block, "LAMBDA_GLOBAL_VICREG"));
  cfg.lambda_channel_vicreg =
      kv::parse_double(kv::required(block, "LAMBDA_CHANNEL_VICREG"));

  cfg.vicreg_sim_weight =
      kv::parse_double(kv::required(block, "VICREG_SIM_WEIGHT"));
  cfg.vicreg_var_weight =
      kv::parse_double(kv::required(block, "VICREG_VAR_WEIGHT"));
  cfg.vicreg_cov_weight =
      kv::parse_double(kv::required(block, "VICREG_COV_WEIGHT"));
  cfg.vicreg_variance_floor =
      kv::parse_double(kv::required(block, "VICREG_VARIANCE_FLOOR"));
  cfg.vicreg_variance_epsilon =
      kv::parse_double(kv::required(block, "VICREG_VARIANCE_EPSILON"));

  cfg.target_ema_tau = kv::parse_double(kv::required(block, "TARGET_EMA_TAU"));
  cfg.use_target_ema = kv::parse_bool(kv::required(block, "USE_TARGET_EMA"));
  cfg.stop_gradient_target =
      kv::parse_bool(kv::required(block, "STOP_GRADIENT_TARGET"));
  cfg.return_diagnostics =
      kv::parse_bool(kv::required(block, "RETURN_DIAGNOSTICS"));

  cfg.use_mae_decoder = kv::parse_bool(kv::required(block, "USE_MAE_DECODER"));
  cfg.use_jepa_loss = kv::parse_bool(kv::required(block, "USE_JEPA_LOSS"));
  cfg.use_tf_align_loss =
      kv::parse_bool(kv::required(block, "USE_TF_ALIGN_LOSS"));
  cfg.use_vicreg_loss = kv::parse_bool(kv::required(block, "USE_VICREG_LOSS"));
  cfg.use_global_vicreg =
      kv::parse_bool(kv::required(block, "USE_GLOBAL_VICREG"));
  cfg.use_channel_vicreg =
      kv::parse_bool(kv::required(block, "USE_CHANNEL_VICREG"));
  cfg.use_raw_reconstruction_targets =
      kv::parse_bool(kv::required(block, "USE_RAW_RECONSTRUCTION_TARGETS"));
  cfg.strict_finite_loss =
      kv::parse_bool(kv::required(block, "STRICT_FINITE_LOSS"));
  cfg.couple_time_frequency_masks =
      kv::parse_bool(kv::required(block, "COUPLE_TIME_FREQUENCY_MASKS"));
  cfg.mask_same_window_across_domains =
      kv::parse_bool(kv::required(block, "MASK_SAME_WINDOW_ACROSS_DOMAINS"));
  cfg.mask_same_channel_block =
      kv::parse_bool(kv::required(block, "MASK_SAME_CHANNEL_BLOCK"));
  cfg.max_context_target_time_overlap =
      kv::parse_double(kv::required(block, "MAX_CONTEXT_TARGET_TIME_OVERLAP"));
}

[[nodiscard]] inline mtf_jepa_mae_vicreg_spec_t
decode_mtf_jepa_mae_vicreg_spec_from_split_dsl(const std::string &dsl_text,
                                               const std::string &net_text) {
  namespace kv = cuwacunu::piaabo::parse::simple_kv;
  namespace detail = mtf_jepa_mae_vicreg_spec_detail;
  const auto &block = kv::single_block(dsl_text, "MTF_JEPA_MAE_VICREG");
  mtf_jepa_mae_vicreg_spec_t spec{};
  spec.version_token = kv::required(block, "VERSION");
  spec.component_assembly_id = kv::required(block, "COMPONENT_ASSEMBLY_ID");
  spec.input_route =
      detail::parse_input_route(kv::required(block, "INPUT_ROUTE"));
  spec.config.channel_count =
      kv::parse_i64(kv::required(block, "CHANNEL_COUNT"));
  spec.config.history_length =
      kv::parse_i64(kv::required(block, "HISTORY_LENGTH"));
  spec.config.input_width = kv::parse_i64(kv::required(block, "INPUT_WIDTH"));
  const auto dtype = kv::required(block, "DTYPE");
  const auto device = kv::required(block, "DEVICE");
  if (!detail::valid_dtype(dtype)) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg_spec] DTYPE must be float32 or float64");
  }
  if (!detail::valid_device(device)) {
    throw std::runtime_error(
        "[mtf_jepa_mae_vicreg_spec] DEVICE must be cpu, cuda, or cuda:N");
  }
  spec.config.dtype = detail::torch_dtype(dtype);
  spec.config.device = detail::torch_device(device);
  decode_mtf_jepa_mae_vicreg_net_into_spec(net_text, spec);
  validate_mtf_jepa_mae_vicreg_spec(spec);
  return spec;
}

} // namespace cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg
  // mtf_jepa_mae_vicreg
