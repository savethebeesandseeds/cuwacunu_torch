// SPDX-License-Identifier: MIT
#pragma once

#include <bit>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <locale>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "piaabo/digest/sha256.h"

namespace cuwacunu::wikimyei::inference::expected_value::mdn {

inline constexpr std::int64_t kPerEdgeConditionedAffineReturnHeadSchemaVersion =
    2;
inline constexpr const char *kPerEdgeConditionedAffineReturnHeadArtifactFamily =
    "wikimyei.inference.expected_value.mdn."
    "per_edge_conditioned_affine_return_head.v2";
inline constexpr const char *kPerEdgeConditionedAffineReturnHeadFeatureLayout =
    "readout_feature_prefix.v2";
inline constexpr const char *kPerEdgeConditionedAffineReturnHeadSuffixPolicy =
    "ignore_after_feature_dim.v1";
inline constexpr const char
    *kPerEdgeConditionedAffineReturnHeadNormalizationArithmetic =
        "float32_subtract_mean_then_multiply_inv_std.v1";
inline constexpr const char
    *kPerEdgeConditionedAffineReturnHeadPromotionArithmetic =
        "normalized_float32_to_float64_same_device.v1";
inline constexpr const char
    *kPerEdgeConditionedAffineReturnHeadAffineArithmetic =
        "float64_subtract_edge_center_then_dot_mapped_weights_plus_"
        "centered_bias.v1";
inline constexpr const char *kPerEdgeConditionedAffineReturnHeadInputDtype =
    "float32";
inline constexpr const char
    *kPerEdgeConditionedAffineReturnHeadNormalizationDtype = "float32";
inline constexpr const char
    *kPerEdgeConditionedAffineReturnHeadCoefficientDtype = "float64";
inline constexpr const char
    *kPerEdgeConditionedAffineReturnHeadAccumulationDtype = "float64";
inline constexpr const char *kPerEdgeConditionedAffineReturnHeadOutputDtype =
    "float32";
inline constexpr const char *kPerEdgeConditionedAffineReturnHeadDevicePolicy =
    "input_and_registered_buffers_same_device.v1";
inline constexpr const char *
    kPerEdgeConditionedAffineReturnHeadStatisticsScope = "development_fit_only";

struct PerEdgeConditionedAffineReturnHeadOptions {
  std::int64_t feature_dim{0};
  std::int64_t edge_count{0};
  std::int64_t channel_count{0};
};

struct PerEdgeConditionedAffineReturnHeadState {
  torch::Tensor feature_mean{};    // [F], float32
  torch::Tensor feature_inv_std{}; // [F], float32
  torch::Tensor edge_center{};     // [E,F], float64
  torch::Tensor mapped_weights{};  // [E,F], float64
  torch::Tensor centered_bias{};   // [E], float64
};

struct PerEdgeConditionedAffineReturnHeadArtifactMetadata {
  std::int64_t schema_version{kPerEdgeConditionedAffineReturnHeadSchemaVersion};
  std::string artifact_family{
      kPerEdgeConditionedAffineReturnHeadArtifactFamily};
  bool run_only{true};
  bool policy_eligible{false};

  std::int64_t feature_dim{0};
  std::int64_t edge_count{0};
  std::int64_t channel_count{0};

  std::string feature_layout{kPerEdgeConditionedAffineReturnHeadFeatureLayout};
  std::string suffix_policy{kPerEdgeConditionedAffineReturnHeadSuffixPolicy};
  std::string normalization_arithmetic{
      kPerEdgeConditionedAffineReturnHeadNormalizationArithmetic};
  std::string promotion_arithmetic{
      kPerEdgeConditionedAffineReturnHeadPromotionArithmetic};
  std::string affine_arithmetic{
      kPerEdgeConditionedAffineReturnHeadAffineArithmetic};
  std::string input_dtype{kPerEdgeConditionedAffineReturnHeadInputDtype};
  std::string normalization_dtype{
      kPerEdgeConditionedAffineReturnHeadNormalizationDtype};
  std::string coefficient_dtype{
      kPerEdgeConditionedAffineReturnHeadCoefficientDtype};
  std::string accumulation_dtype{
      kPerEdgeConditionedAffineReturnHeadAccumulationDtype};
  std::string output_dtype{kPerEdgeConditionedAffineReturnHeadOutputDtype};
  std::string device_policy{kPerEdgeConditionedAffineReturnHeadDevicePolicy};
  std::string statistics_scope{
      kPerEdgeConditionedAffineReturnHeadStatisticsScope};

  double conditioning_alpha{0.0};
  std::int64_t fit_begin{0};
  std::int64_t fit_end{0};
  std::int64_t purge_begin{0};
  std::int64_t purge_end{0};
  std::int64_t validation_begin{0};
  std::int64_t validation_end{0};
  std::int64_t max_anchor{0};

  std::string graph_order_fingerprint{};
  std::vector<std::string> edge_ids{};
  std::vector<std::string> node_ids{};
  std::string source_probe_schema_id{};
  std::string source_probe_sha256{};
  std::string conditioning_probe_schema_id{};
  std::string conditioning_probe_sha256{};
  std::string representation_checkpoint_sha256{};
  std::string mdn_checkpoint_sha256{};
  std::string conditioning_transform_schema_id{};
  std::string conditioning_transform_sha256{};

  std::string semantic_tensor_digest{};
};

namespace per_edge_conditioned_affine_detail {

inline constexpr const char *kArchiveMagic =
    "per_edge_conditioned_affine_return_head_archive.v2";

inline void
validate_options(const PerEdgeConditionedAffineReturnHeadOptions &options) {
  TORCH_CHECK(options.feature_dim > 0,
              "[PerEdgeConditionedAffineReturnHead] feature_dim must be "
              "positive");
  TORCH_CHECK(options.edge_count > 0,
              "[PerEdgeConditionedAffineReturnHead] edge_count must be "
              "positive");
  TORCH_CHECK(options.channel_count > 0,
              "[PerEdgeConditionedAffineReturnHead] channel_count must be "
              "positive");
}

inline void validate_metadata_string(const std::string &value, const char *name,
                                     bool required = true) {
  TORCH_CHECK(!required || !value.empty(),
              "[PerEdgeConditionedAffineReturnHead] ", name,
              " must not be empty");
  TORCH_CHECK(value.find('\n') == std::string::npos &&
                  value.find('\r') == std::string::npos,
              "[PerEdgeConditionedAffineReturnHead] ", name,
              " must not contain line breaks");
}

inline void validate_unique_ids(const std::vector<std::string> &values,
                                std::int64_t expected_size, const char *name) {
  TORCH_CHECK(static_cast<std::int64_t>(values.size()) == expected_size,
              "[PerEdgeConditionedAffineReturnHead] ", name, " size mismatch");
  std::set<std::string> unique;
  for (const auto &value : values) {
    validate_metadata_string(value, name);
    TORCH_CHECK(unique.insert(value).second,
                "[PerEdgeConditionedAffineReturnHead] duplicate ", name,
                " value");
  }
}

inline void validate_sha256(const std::string &value, const char *name) {
  TORCH_CHECK(cuwacunu::piaabo::digest::is_sha256_hex(value),
              "[PerEdgeConditionedAffineReturnHead] invalid ", name,
              " SHA-256");
}

inline void validate_metadata(
    const PerEdgeConditionedAffineReturnHeadArtifactMetadata &metadata,
    const PerEdgeConditionedAffineReturnHeadOptions *expected_options,
    bool require_digest) {
  TORCH_CHECK(
      metadata.schema_version ==
          kPerEdgeConditionedAffineReturnHeadSchemaVersion,
      "[PerEdgeConditionedAffineReturnHead] unsupported schema_version");
  TORCH_CHECK(
      metadata.artifact_family ==
          kPerEdgeConditionedAffineReturnHeadArtifactFamily,
      "[PerEdgeConditionedAffineReturnHead] unsupported artifact_family");
  TORCH_CHECK(metadata.run_only,
              "[PerEdgeConditionedAffineReturnHead] artifact must be "
              "run-only");
  TORCH_CHECK(!metadata.policy_eligible,
              "[PerEdgeConditionedAffineReturnHead] artifact must not be "
              "policy-eligible");

  const PerEdgeConditionedAffineReturnHeadOptions options{
      .feature_dim = metadata.feature_dim,
      .edge_count = metadata.edge_count,
      .channel_count = metadata.channel_count,
  };
  validate_options(options);
  if (expected_options != nullptr) {
    TORCH_CHECK(options.feature_dim == expected_options->feature_dim &&
                    options.edge_count == expected_options->edge_count &&
                    options.channel_count == expected_options->channel_count,
                "[PerEdgeConditionedAffineReturnHead] artifact/head "
                "dimensions mismatch");
  }

  TORCH_CHECK(
      metadata.feature_layout ==
              kPerEdgeConditionedAffineReturnHeadFeatureLayout &&
          metadata.suffix_policy ==
              kPerEdgeConditionedAffineReturnHeadSuffixPolicy &&
          metadata.normalization_arithmetic ==
              kPerEdgeConditionedAffineReturnHeadNormalizationArithmetic &&
          metadata.promotion_arithmetic ==
              kPerEdgeConditionedAffineReturnHeadPromotionArithmetic &&
          metadata.affine_arithmetic ==
              kPerEdgeConditionedAffineReturnHeadAffineArithmetic &&
          metadata.input_dtype ==
              kPerEdgeConditionedAffineReturnHeadInputDtype &&
          metadata.normalization_dtype ==
              kPerEdgeConditionedAffineReturnHeadNormalizationDtype &&
          metadata.coefficient_dtype ==
              kPerEdgeConditionedAffineReturnHeadCoefficientDtype &&
          metadata.accumulation_dtype ==
              kPerEdgeConditionedAffineReturnHeadAccumulationDtype &&
          metadata.output_dtype ==
              kPerEdgeConditionedAffineReturnHeadOutputDtype &&
          metadata.device_policy ==
              kPerEdgeConditionedAffineReturnHeadDevicePolicy &&
          metadata.statistics_scope ==
              kPerEdgeConditionedAffineReturnHeadStatisticsScope,
      "[PerEdgeConditionedAffineReturnHead] unsupported arithmetic or dtype "
      "contract");

  TORCH_CHECK(std::isfinite(metadata.conditioning_alpha) &&
                  metadata.conditioning_alpha > 0.0,
              "[PerEdgeConditionedAffineReturnHead] conditioning_alpha must "
              "be positive");
  TORCH_CHECK(metadata.fit_begin >= 0 &&
                  metadata.fit_begin < metadata.fit_end &&
                  metadata.fit_end == metadata.purge_begin &&
                  metadata.purge_begin <= metadata.purge_end &&
                  metadata.purge_end == metadata.validation_begin &&
                  metadata.validation_begin < metadata.validation_end &&
                  metadata.max_anchor == metadata.validation_end - 1,
              "[PerEdgeConditionedAffineReturnHead] invalid development split "
              "metadata");

  validate_metadata_string(metadata.graph_order_fingerprint,
                           "graph_order_fingerprint");
  validate_unique_ids(metadata.edge_ids, metadata.edge_count, "edge_ids");
  validate_unique_ids(metadata.node_ids, metadata.edge_count + 1, "node_ids");
  validate_metadata_string(metadata.source_probe_schema_id,
                           "source_probe_schema_id");
  validate_metadata_string(metadata.conditioning_probe_schema_id,
                           "conditioning_probe_schema_id");
  validate_metadata_string(metadata.conditioning_transform_schema_id,
                           "conditioning_transform_schema_id");
  validate_sha256(metadata.source_probe_sha256, "source_probe");
  validate_sha256(metadata.conditioning_probe_sha256, "conditioning_probe");
  validate_sha256(metadata.representation_checkpoint_sha256,
                  "representation_checkpoint");
  validate_sha256(metadata.mdn_checkpoint_sha256, "mdn_checkpoint");
  validate_sha256(metadata.conditioning_transform_sha256,
                  "conditioning_transform");
  if (require_digest) {
    validate_sha256(metadata.semantic_tensor_digest, "semantic_tensor_digest");
  } else {
    TORCH_CHECK(metadata.semantic_tensor_digest.empty() ||
                    cuwacunu::piaabo::digest::is_sha256_hex(
                        metadata.semantic_tensor_digest),
                "[PerEdgeConditionedAffineReturnHead] invalid semantic tensor "
                "digest");
  }
}

inline void append_string(std::ostringstream &out, const std::string &key,
                          const std::string &value) {
  out << key << '=' << value.size() << ':' << value << '\n';
}

inline std::string canonical_metadata_without_digest(
    const PerEdgeConditionedAffineReturnHeadArtifactMetadata &metadata) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << "per_edge_conditioned_affine_return_head_metadata.v2\n";
  out << "schema_version=" << metadata.schema_version << '\n';
  append_string(out, "artifact_family", metadata.artifact_family);
  out << "run_only=" << (metadata.run_only ? 1 : 0) << '\n';
  out << "policy_eligible=" << (metadata.policy_eligible ? 1 : 0) << '\n';
  out << "feature_dim=" << metadata.feature_dim << '\n';
  out << "edge_count=" << metadata.edge_count << '\n';
  out << "channel_count=" << metadata.channel_count << '\n';
  append_string(out, "feature_layout", metadata.feature_layout);
  append_string(out, "suffix_policy", metadata.suffix_policy);
  append_string(out, "normalization_arithmetic",
                metadata.normalization_arithmetic);
  append_string(out, "promotion_arithmetic", metadata.promotion_arithmetic);
  append_string(out, "affine_arithmetic", metadata.affine_arithmetic);
  append_string(out, "input_dtype", metadata.input_dtype);
  append_string(out, "normalization_dtype", metadata.normalization_dtype);
  append_string(out, "coefficient_dtype", metadata.coefficient_dtype);
  append_string(out, "accumulation_dtype", metadata.accumulation_dtype);
  append_string(out, "output_dtype", metadata.output_dtype);
  append_string(out, "device_policy", metadata.device_policy);
  append_string(out, "statistics_scope", metadata.statistics_scope);
  out << std::scientific << std::setprecision(17);
  out << "conditioning_alpha=" << metadata.conditioning_alpha << '\n';
  out << std::defaultfloat;
  out << "fit_begin=" << metadata.fit_begin << '\n';
  out << "fit_end=" << metadata.fit_end << '\n';
  out << "purge_begin=" << metadata.purge_begin << '\n';
  out << "purge_end=" << metadata.purge_end << '\n';
  out << "validation_begin=" << metadata.validation_begin << '\n';
  out << "validation_end=" << metadata.validation_end << '\n';
  out << "max_anchor=" << metadata.max_anchor << '\n';
  append_string(out, "graph_order_fingerprint",
                metadata.graph_order_fingerprint);
  out << "edge_ids.count=" << metadata.edge_ids.size() << '\n';
  for (std::size_t i = 0; i < metadata.edge_ids.size(); ++i) {
    append_string(out, "edge_ids." + std::to_string(i), metadata.edge_ids[i]);
  }
  out << "node_ids.count=" << metadata.node_ids.size() << '\n';
  for (std::size_t i = 0; i < metadata.node_ids.size(); ++i) {
    append_string(out, "node_ids." + std::to_string(i), metadata.node_ids[i]);
  }
  append_string(out, "source_probe_schema_id", metadata.source_probe_schema_id);
  append_string(out, "source_probe_sha256", metadata.source_probe_sha256);
  append_string(out, "conditioning_probe_schema_id",
                metadata.conditioning_probe_schema_id);
  append_string(out, "conditioning_probe_sha256",
                metadata.conditioning_probe_sha256);
  append_string(out, "representation_checkpoint_sha256",
                metadata.representation_checkpoint_sha256);
  append_string(out, "mdn_checkpoint_sha256", metadata.mdn_checkpoint_sha256);
  append_string(out, "conditioning_transform_schema_id",
                metadata.conditioning_transform_schema_id);
  append_string(out, "conditioning_transform_sha256",
                metadata.conditioning_transform_sha256);
  return out.str();
}

inline torch::Tensor int64_tensor(std::int64_t value) {
  return torch::tensor({value}, torch::TensorOptions().dtype(torch::kInt64));
}

inline torch::Tensor float64_tensor(double value) {
  return torch::tensor({value}, torch::TensorOptions().dtype(torch::kFloat64));
}

inline torch::Tensor bytes_tensor(const std::string &value) {
  std::vector<std::int64_t> bytes;
  bytes.reserve(value.size());
  for (const unsigned char byte : value) {
    bytes.push_back(static_cast<std::int64_t>(byte));
  }
  return torch::tensor(bytes, torch::TensorOptions().dtype(torch::kInt64));
}

inline std::string tensor_string(const torch::Tensor &tensor,
                                 const char *name) {
  TORCH_CHECK(tensor.defined() && tensor.dim() == 1,
              "[PerEdgeConditionedAffineReturnHead] ", name,
              " bytes must be rank one");
  auto cpu = tensor.to(torch::kCPU).to(torch::kInt64).contiguous();
  std::string value;
  value.reserve(static_cast<std::size_t>(cpu.numel()));
  const auto accessor = cpu.accessor<std::int64_t, 1>();
  for (std::int64_t i = 0; i < cpu.numel(); ++i) {
    TORCH_CHECK(accessor[i] >= 0 && accessor[i] <= 255,
                "[PerEdgeConditionedAffineReturnHead] invalid byte in ", name);
    value.push_back(static_cast<char>(accessor[i]));
  }
  return value;
}

inline std::int64_t tensor_i64(const torch::Tensor &tensor, const char *name) {
  TORCH_CHECK(tensor.defined() && tensor.numel() == 1,
              "[PerEdgeConditionedAffineReturnHead] ", name, " must be scalar");
  return tensor.to(torch::kCPU).to(torch::kInt64).item<std::int64_t>();
}

inline double tensor_f64(const torch::Tensor &tensor, const char *name) {
  TORCH_CHECK(tensor.defined() && tensor.numel() == 1,
              "[PerEdgeConditionedAffineReturnHead] ", name, " must be scalar");
  return tensor.to(torch::kCPU).to(torch::kFloat64).item<double>();
}

inline void write_string(torch::serialize::OutputArchive &root,
                         const std::string &key, const std::string &value) {
  root.write(key, bytes_tensor(value));
}

inline std::string read_string(torch::serialize::InputArchive &root,
                               const std::string &key) {
  torch::Tensor value;
  root.read(key, value);
  return tensor_string(value, key.c_str());
}

inline void write_string_vector(torch::serialize::OutputArchive &root,
                                const std::string &prefix,
                                const std::vector<std::string> &values) {
  std::vector<std::int64_t> lengths;
  std::string bytes;
  lengths.reserve(values.size());
  for (const auto &value : values) {
    lengths.push_back(static_cast<std::int64_t>(value.size()));
    bytes += value;
  }
  root.write(
      prefix + "/lengths",
      torch::tensor(lengths, torch::TensorOptions().dtype(torch::kInt64)));
  root.write(prefix + "/bytes", bytes_tensor(bytes));
}

inline std::vector<std::string>
read_string_vector(torch::serialize::InputArchive &root,
                   const std::string &prefix) {
  torch::Tensor lengths_tensor;
  root.read(prefix + "/lengths", lengths_tensor);
  const auto bytes = read_string(root, prefix + "/bytes");
  auto lengths =
      lengths_tensor.to(torch::kCPU).to(torch::kInt64).contiguous().view({-1});
  const auto accessor = lengths.accessor<std::int64_t, 1>();
  std::vector<std::string> values;
  values.reserve(static_cast<std::size_t>(lengths.numel()));
  std::size_t offset = 0;
  for (std::int64_t i = 0; i < lengths.numel(); ++i) {
    TORCH_CHECK(accessor[i] >= 0,
                "[PerEdgeConditionedAffineReturnHead] negative string "
                "length");
    const auto length = static_cast<std::size_t>(accessor[i]);
    TORCH_CHECK(offset + length <= bytes.size(),
                "[PerEdgeConditionedAffineReturnHead] invalid string vector "
                "bytes");
    values.push_back(bytes.substr(offset, length));
    offset += length;
  }
  TORCH_CHECK(offset == bytes.size(),
              "[PerEdgeConditionedAffineReturnHead] trailing string vector "
              "bytes");
  return values;
}

} // namespace per_edge_conditioned_affine_detail

struct PerEdgeConditionedAffineReturnHeadImpl : torch::nn::Module {
  explicit PerEdgeConditionedAffineReturnHeadImpl(
      const PerEdgeConditionedAffineReturnHeadOptions &options)
      : options_(options) {
    per_edge_conditioned_affine_detail::validate_options(options_);
    const auto f32 = torch::TensorOptions().dtype(torch::kFloat32);
    const auto f64 = torch::TensorOptions().dtype(torch::kFloat64);
    feature_mean = register_buffer("feature_mean",
                                   torch::zeros({options_.feature_dim}, f32));
    feature_inv_std = register_buffer("feature_inv_std",
                                      torch::ones({options_.feature_dim}, f32));
    edge_center = register_buffer(
        "edge_center",
        torch::zeros({options_.edge_count, options_.feature_dim}, f64));
    mapped_weights = register_buffer(
        "mapped_weights",
        torch::zeros({options_.edge_count, options_.feature_dim}, f64));
    centered_bias = register_buffer("centered_bias",
                                    torch::zeros({options_.edge_count}, f64));
    eval();
    validate_registered_state();
  }

  PerEdgeConditionedAffineReturnHeadImpl(
      const PerEdgeConditionedAffineReturnHeadOptions &options,
      const PerEdgeConditionedAffineReturnHeadState &state)
      : PerEdgeConditionedAffineReturnHeadImpl(options) {
    copy_conditioned_affine_state(state);
  }

  [[nodiscard]] const PerEdgeConditionedAffineReturnHeadOptions &
  options() const {
    return options_;
  }

  [[nodiscard]] PerEdgeConditionedAffineReturnHeadState
  conditioned_affine_state() const {
    validate_registered_state();
    return {
        .feature_mean = feature_mean.detach().clone(),
        .feature_inv_std = feature_inv_std.detach().clone(),
        .edge_center = edge_center.detach().clone(),
        .mapped_weights = mapped_weights.detach().clone(),
        .centered_bias = centered_bias.detach().clone(),
    };
  }

  void copy_conditioned_affine_state(
      const PerEdgeConditionedAffineReturnHeadState &state) {
    validate_state_tensor(state.feature_mean, {options_.feature_dim},
                          torch::kFloat32, "feature_mean");
    validate_state_tensor(state.feature_inv_std, {options_.feature_dim},
                          torch::kFloat32, "feature_inv_std");
    validate_state_tensor(state.edge_center,
                          {options_.edge_count, options_.feature_dim},
                          torch::kFloat64, "edge_center");
    validate_state_tensor(state.mapped_weights,
                          {options_.edge_count, options_.feature_dim},
                          torch::kFloat64, "mapped_weights");
    validate_state_tensor(state.centered_bias, {options_.edge_count},
                          torch::kFloat64, "centered_bias");
    const auto source_device = state.feature_mean.device();
    TORCH_CHECK(state.feature_inv_std.device() == source_device &&
                    state.edge_center.device() == source_device &&
                    state.mapped_weights.device() == source_device &&
                    state.centered_bias.device() == source_device,
                "[PerEdgeConditionedAffineReturnHead] state tensors must "
                "share one device");
    TORCH_CHECK((state.feature_inv_std > 0.0).all().item<bool>(),
                "[PerEdgeConditionedAffineReturnHead] feature_inv_std must "
                "be positive");
    torch::NoGradGuard no_grad;
    feature_mean.copy_(state.feature_mean);
    feature_inv_std.copy_(state.feature_inv_std);
    edge_center.copy_(state.edge_center);
    mapped_weights.copy_(state.mapped_weights);
    centered_bias.copy_(state.centered_bias);
    eval();
    validate_registered_state();
  }

  void validate_registered_state() const {
    per_edge_conditioned_affine_detail::validate_options(options_);
    validate_registered_tensor(feature_mean, {options_.feature_dim},
                               torch::kFloat32, "feature_mean");
    validate_registered_tensor(feature_inv_std, {options_.feature_dim},
                               torch::kFloat32, "feature_inv_std");
    validate_registered_tensor(edge_center,
                               {options_.edge_count, options_.feature_dim},
                               torch::kFloat64, "edge_center");
    validate_registered_tensor(mapped_weights,
                               {options_.edge_count, options_.feature_dim},
                               torch::kFloat64, "mapped_weights");
    validate_registered_tensor(centered_bias, {options_.edge_count},
                               torch::kFloat64, "centered_bias");
    TORCH_CHECK(feature_inv_std.device() == feature_mean.device() &&
                    edge_center.device() == feature_mean.device() &&
                    mapped_weights.device() == feature_mean.device() &&
                    centered_bias.device() == feature_mean.device(),
                "[PerEdgeConditionedAffineReturnHead] registered buffers must "
                "share one device");
    TORCH_CHECK((feature_inv_std > 0.0).all().item<bool>(),
                "[PerEdgeConditionedAffineReturnHead] feature_inv_std must "
                "be positive");
    TORCH_CHECK(named_parameters(/*recurse=*/true).size() == 0,
                "[PerEdgeConditionedAffineReturnHead] head must remain "
                "parameter-free");
  }

  torch::Tensor
  forward_readout_features(const torch::Tensor &readout_features) {
    validate_registered_state();
    TORCH_CHECK(readout_features.defined() && readout_features.dim() == 4 &&
                    readout_features.size(0) > 0 &&
                    readout_features.size(1) == options_.edge_count &&
                    readout_features.size(2) == options_.channel_count &&
                    readout_features.size(3) >= options_.feature_dim,
                "[PerEdgeConditionedAffineReturnHead] readout features must "
                "be [B,E,C,D] with D >= feature_dim");
    TORCH_CHECK(readout_features.scalar_type() == torch::kFloat32,
                "[PerEdgeConditionedAffineReturnHead] readout features must "
                "be float32");
    TORCH_CHECK(readout_features.device() == feature_mean.device(),
                "[PerEdgeConditionedAffineReturnHead] readout features and "
                "buffers must share one device");
    const auto F = options_.feature_dim;
    auto prefix_f32 = readout_features.narrow(/*dim=*/3, /*start=*/0, F);
    TORCH_CHECK(torch::isfinite(prefix_f32).all().item<bool>(),
                "[PerEdgeConditionedAffineReturnHead] readout feature prefix "
                "must be finite");
    auto normalized_f32 = (prefix_f32 - feature_mean.view({1, 1, 1, F})) *
                          feature_inv_std.view({1, 1, 1, F});
    TORCH_CHECK(torch::isfinite(normalized_f32).all().item<bool>(),
                "[PerEdgeConditionedAffineReturnHead] float32 normalization "
                "overflowed");
    auto centered_f64 = normalized_f32.to(torch::kFloat64) -
                        edge_center.view({1, options_.edge_count, 1, F});
    auto output_f64 =
        (centered_f64 * mapped_weights.view({1, options_.edge_count, 1, F}))
            .sum(/*dim=*/3) +
        centered_bias.view({1, options_.edge_count, 1});
    TORCH_CHECK(torch::isfinite(output_f64).all().item<bool>(),
                "[PerEdgeConditionedAffineReturnHead] float64 affine "
                "accumulation overflowed");
    auto output_f32 = output_f64.to(torch::kFloat32);
    TORCH_CHECK(torch::isfinite(output_f32).all().item<bool>(),
                "[PerEdgeConditionedAffineReturnHead] float32 output cast "
                "overflowed");
    return output_f32;
  }

  torch::Tensor feature_mean;
  torch::Tensor feature_inv_std;
  torch::Tensor edge_center;
  torch::Tensor mapped_weights;
  torch::Tensor centered_bias;

private:
  static void validate_state_tensor(const torch::Tensor &tensor,
                                    torch::IntArrayRef shape,
                                    torch::ScalarType dtype, const char *name) {
    TORCH_CHECK(tensor.defined() && tensor.scalar_type() == dtype &&
                    tensor.sizes() == shape &&
                    torch::isfinite(tensor).all().item<bool>(),
                "[PerEdgeConditionedAffineReturnHead] invalid state tensor ",
                name);
  }

  static void validate_registered_tensor(const torch::Tensor &tensor,
                                         torch::IntArrayRef shape,
                                         torch::ScalarType dtype,
                                         const char *name) {
    validate_state_tensor(tensor, shape, dtype, name);
  }

  PerEdgeConditionedAffineReturnHeadOptions options_{};
};
TORCH_MODULE(PerEdgeConditionedAffineReturnHead);

inline std::string canonical_conditioned_affine_metadata_text(
    const PerEdgeConditionedAffineReturnHeadArtifactMetadata &metadata) {
  auto text =
      per_edge_conditioned_affine_detail::canonical_metadata_without_digest(
          metadata);
  std::ostringstream digest_line;
  per_edge_conditioned_affine_detail::append_string(
      digest_line, "semantic_tensor_digest", metadata.semantic_tensor_digest);
  text += digest_line.str();
  return text;
}

namespace per_edge_conditioned_affine_detail {

inline void append_u32_be(std::string &bytes, std::uint32_t value) {
  for (int shift = 24; shift >= 0; shift -= 8) {
    bytes.push_back(static_cast<char>((value >> shift) & 0xffU));
  }
}

inline void append_u64_be(std::string &bytes, std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8) {
    bytes.push_back(static_cast<char>((value >> shift) & 0xffU));
  }
}

inline void append_tensor_header(std::string &bytes, const std::string &name,
                                 const torch::Tensor &tensor,
                                 const char *dtype) {
  bytes += "tensor=" + name + "\n";
  bytes += "dtype=" + std::string(dtype) + "\n";
  bytes += "rank=" + std::to_string(tensor.dim()) + "\n";
  for (std::int64_t dim = 0; dim < tensor.dim(); ++dim) {
    bytes += "dim." + std::to_string(dim) + "=" +
             std::to_string(tensor.size(dim)) + "\n";
  }
  bytes += "count=" + std::to_string(tensor.numel()) + "\n";
}

inline void append_tensor_f32_be(std::string &bytes, const std::string &name,
                                 const torch::Tensor &tensor) {
  TORCH_CHECK(tensor.scalar_type() == torch::kFloat32,
              "[PerEdgeConditionedAffineReturnHead] digest tensor ", name,
              " must be float32");
  auto flat = tensor.detach().to(torch::kCPU).contiguous().view({-1});
  append_tensor_header(bytes, name, tensor, "float32");
  const auto accessor = flat.accessor<float, 1>();
  for (std::int64_t i = 0; i < flat.numel(); ++i) {
    append_u32_be(bytes, std::bit_cast<std::uint32_t>(accessor[i]));
  }
}

inline void append_tensor_f64_be(std::string &bytes, const std::string &name,
                                 const torch::Tensor &tensor) {
  TORCH_CHECK(tensor.scalar_type() == torch::kFloat64,
              "[PerEdgeConditionedAffineReturnHead] digest tensor ", name,
              " must be float64");
  auto flat = tensor.detach().to(torch::kCPU).contiguous().view({-1});
  append_tensor_header(bytes, name, tensor, "float64");
  const auto accessor = flat.accessor<double, 1>();
  for (std::int64_t i = 0; i < flat.numel(); ++i) {
    append_u64_be(bytes, std::bit_cast<std::uint64_t>(accessor[i]));
  }
}

} // namespace per_edge_conditioned_affine_detail

inline std::string conditioned_affine_semantic_tensor_digest(
    const PerEdgeConditionedAffineReturnHead &head,
    const PerEdgeConditionedAffineReturnHeadArtifactMetadata &metadata) {
  TORCH_CHECK(!head.is_empty(),
              "[PerEdgeConditionedAffineReturnHead] cannot digest a null "
              "head");
  head->validate_registered_state();
  per_edge_conditioned_affine_detail::validate_metadata(
      metadata, &head->options(), /*require_digest=*/false);
  const auto state = head->conditioned_affine_state();
  std::string bytes =
      per_edge_conditioned_affine_detail::canonical_metadata_without_digest(
          metadata);
  bytes += "semantic_tensor_encoding=ieee754_exact_big_endian.v2\n";
  per_edge_conditioned_affine_detail::append_tensor_f32_be(
      bytes, "feature_mean", state.feature_mean);
  per_edge_conditioned_affine_detail::append_tensor_f32_be(
      bytes, "feature_inv_std", state.feature_inv_std);
  per_edge_conditioned_affine_detail::append_tensor_f64_be(bytes, "edge_center",
                                                           state.edge_center);
  per_edge_conditioned_affine_detail::append_tensor_f64_be(
      bytes, "mapped_weights", state.mapped_weights);
  per_edge_conditioned_affine_detail::append_tensor_f64_be(
      bytes, "centered_bias", state.centered_bias);
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

namespace per_edge_conditioned_affine_detail {

inline void write_metadata(
    torch::serialize::OutputArchive &root,
    const PerEdgeConditionedAffineReturnHeadArtifactMetadata &metadata) {
  write_string(root, "meta/archive_magic_bytes", kArchiveMagic);
  root.write("meta/schema_version", int64_tensor(metadata.schema_version));
  write_string(root, "meta/artifact_family_bytes", metadata.artifact_family);
  root.write("meta/run_only", int64_tensor(metadata.run_only ? 1 : 0));
  root.write("meta/policy_eligible",
             int64_tensor(metadata.policy_eligible ? 1 : 0));
  root.write("meta/feature_dim", int64_tensor(metadata.feature_dim));
  root.write("meta/edge_count", int64_tensor(metadata.edge_count));
  root.write("meta/channel_count", int64_tensor(metadata.channel_count));
  write_string(root, "meta/feature_layout_bytes", metadata.feature_layout);
  write_string(root, "meta/suffix_policy_bytes", metadata.suffix_policy);
  write_string(root, "meta/normalization_arithmetic_bytes",
               metadata.normalization_arithmetic);
  write_string(root, "meta/promotion_arithmetic_bytes",
               metadata.promotion_arithmetic);
  write_string(root, "meta/affine_arithmetic_bytes",
               metadata.affine_arithmetic);
  write_string(root, "meta/input_dtype_bytes", metadata.input_dtype);
  write_string(root, "meta/normalization_dtype_bytes",
               metadata.normalization_dtype);
  write_string(root, "meta/coefficient_dtype_bytes",
               metadata.coefficient_dtype);
  write_string(root, "meta/accumulation_dtype_bytes",
               metadata.accumulation_dtype);
  write_string(root, "meta/output_dtype_bytes", metadata.output_dtype);
  write_string(root, "meta/device_policy_bytes", metadata.device_policy);
  write_string(root, "meta/statistics_scope_bytes", metadata.statistics_scope);
  root.write("meta/conditioning_alpha",
             float64_tensor(metadata.conditioning_alpha));
  root.write("meta/fit_begin", int64_tensor(metadata.fit_begin));
  root.write("meta/fit_end", int64_tensor(metadata.fit_end));
  root.write("meta/purge_begin", int64_tensor(metadata.purge_begin));
  root.write("meta/purge_end", int64_tensor(metadata.purge_end));
  root.write("meta/validation_begin", int64_tensor(metadata.validation_begin));
  root.write("meta/validation_end", int64_tensor(metadata.validation_end));
  root.write("meta/max_anchor", int64_tensor(metadata.max_anchor));
  write_string(root, "meta/graph_order_fingerprint_bytes",
               metadata.graph_order_fingerprint);
  write_string_vector(root, "meta/edge_ids", metadata.edge_ids);
  write_string_vector(root, "meta/node_ids", metadata.node_ids);
  write_string(root, "meta/source_probe_schema_id_bytes",
               metadata.source_probe_schema_id);
  write_string(root, "meta/source_probe_sha256_bytes",
               metadata.source_probe_sha256);
  write_string(root, "meta/conditioning_probe_schema_id_bytes",
               metadata.conditioning_probe_schema_id);
  write_string(root, "meta/conditioning_probe_sha256_bytes",
               metadata.conditioning_probe_sha256);
  write_string(root, "meta/representation_checkpoint_sha256_bytes",
               metadata.representation_checkpoint_sha256);
  write_string(root, "meta/mdn_checkpoint_sha256_bytes",
               metadata.mdn_checkpoint_sha256);
  write_string(root, "meta/conditioning_transform_schema_id_bytes",
               metadata.conditioning_transform_schema_id);
  write_string(root, "meta/conditioning_transform_sha256_bytes",
               metadata.conditioning_transform_sha256);
  write_string(root, "meta/semantic_tensor_digest_bytes",
               metadata.semantic_tensor_digest);
}

inline PerEdgeConditionedAffineReturnHeadArtifactMetadata
read_metadata(torch::serialize::InputArchive &root) {
  TORCH_CHECK(read_string(root, "meta/archive_magic_bytes") == kArchiveMagic,
              "[PerEdgeConditionedAffineReturnHead] unsupported archive "
              "magic");
  PerEdgeConditionedAffineReturnHeadArtifactMetadata metadata;
  const auto read_tensor = [&](const std::string &key) {
    torch::Tensor value;
    root.read(key, value);
    return value;
  };
  metadata.schema_version =
      tensor_i64(read_tensor("meta/schema_version"), "schema_version");
  metadata.artifact_family = read_string(root, "meta/artifact_family_bytes");
  const auto run_only = tensor_i64(read_tensor("meta/run_only"), "run_only");
  TORCH_CHECK(run_only == 0 || run_only == 1,
              "[PerEdgeConditionedAffineReturnHead] run_only must be 0 or 1");
  metadata.run_only = run_only == 1;
  const auto policy_eligible =
      tensor_i64(read_tensor("meta/policy_eligible"), "policy_eligible");
  TORCH_CHECK(policy_eligible == 0 || policy_eligible == 1,
              "[PerEdgeConditionedAffineReturnHead] policy_eligible must be "
              "0 or 1");
  metadata.policy_eligible = policy_eligible == 1;
  metadata.feature_dim =
      tensor_i64(read_tensor("meta/feature_dim"), "feature_dim");
  metadata.edge_count =
      tensor_i64(read_tensor("meta/edge_count"), "edge_count");
  metadata.channel_count =
      tensor_i64(read_tensor("meta/channel_count"), "channel_count");
  metadata.feature_layout = read_string(root, "meta/feature_layout_bytes");
  metadata.suffix_policy = read_string(root, "meta/suffix_policy_bytes");
  metadata.normalization_arithmetic =
      read_string(root, "meta/normalization_arithmetic_bytes");
  metadata.promotion_arithmetic =
      read_string(root, "meta/promotion_arithmetic_bytes");
  metadata.affine_arithmetic =
      read_string(root, "meta/affine_arithmetic_bytes");
  metadata.input_dtype = read_string(root, "meta/input_dtype_bytes");
  metadata.normalization_dtype =
      read_string(root, "meta/normalization_dtype_bytes");
  metadata.coefficient_dtype =
      read_string(root, "meta/coefficient_dtype_bytes");
  metadata.accumulation_dtype =
      read_string(root, "meta/accumulation_dtype_bytes");
  metadata.output_dtype = read_string(root, "meta/output_dtype_bytes");
  metadata.device_policy = read_string(root, "meta/device_policy_bytes");
  metadata.statistics_scope = read_string(root, "meta/statistics_scope_bytes");
  metadata.conditioning_alpha =
      tensor_f64(read_tensor("meta/conditioning_alpha"), "conditioning_alpha");
  metadata.fit_begin = tensor_i64(read_tensor("meta/fit_begin"), "fit_begin");
  metadata.fit_end = tensor_i64(read_tensor("meta/fit_end"), "fit_end");
  metadata.purge_begin =
      tensor_i64(read_tensor("meta/purge_begin"), "purge_begin");
  metadata.purge_end = tensor_i64(read_tensor("meta/purge_end"), "purge_end");
  metadata.validation_begin =
      tensor_i64(read_tensor("meta/validation_begin"), "validation_begin");
  metadata.validation_end =
      tensor_i64(read_tensor("meta/validation_end"), "validation_end");
  metadata.max_anchor =
      tensor_i64(read_tensor("meta/max_anchor"), "max_anchor");
  metadata.graph_order_fingerprint =
      read_string(root, "meta/graph_order_fingerprint_bytes");
  metadata.edge_ids = read_string_vector(root, "meta/edge_ids");
  metadata.node_ids = read_string_vector(root, "meta/node_ids");
  metadata.source_probe_schema_id =
      read_string(root, "meta/source_probe_schema_id_bytes");
  metadata.source_probe_sha256 =
      read_string(root, "meta/source_probe_sha256_bytes");
  metadata.conditioning_probe_schema_id =
      read_string(root, "meta/conditioning_probe_schema_id_bytes");
  metadata.conditioning_probe_sha256 =
      read_string(root, "meta/conditioning_probe_sha256_bytes");
  metadata.representation_checkpoint_sha256 =
      read_string(root, "meta/representation_checkpoint_sha256_bytes");
  metadata.mdn_checkpoint_sha256 =
      read_string(root, "meta/mdn_checkpoint_sha256_bytes");
  metadata.conditioning_transform_schema_id =
      read_string(root, "meta/conditioning_transform_schema_id_bytes");
  metadata.conditioning_transform_sha256 =
      read_string(root, "meta/conditioning_transform_sha256_bytes");
  metadata.semantic_tensor_digest =
      read_string(root, "meta/semantic_tensor_digest_bytes");
  return metadata;
}

// Internal writer used by the checked save path and focused corruption tests.
inline void write_artifact_archive_unchecked(
    const std::filesystem::path &path,
    const PerEdgeConditionedAffineReturnHead &head,
    const PerEdgeConditionedAffineReturnHeadArtifactMetadata &metadata,
    const std::string &canonical_text) {
  if (!path.parent_path().empty()) {
    std::filesystem::create_directories(path.parent_path());
  }
  torch::serialize::OutputArchive root;
  write_metadata(root, metadata);
  write_string(root, "meta/canonical_metadata_text_bytes", canonical_text);
  torch::serialize::OutputArchive model;
  head->save(model);
  root.write("model", model);
  root.save_to(path.string());
}

} // namespace per_edge_conditioned_affine_detail

inline void save_per_edge_conditioned_affine_return_head(
    const std::filesystem::path &path,
    const PerEdgeConditionedAffineReturnHead &head,
    const PerEdgeConditionedAffineReturnHeadArtifactMetadata &metadata) {
  TORCH_CHECK(!path.empty(),
              "[PerEdgeConditionedAffineReturnHead] artifact path must not "
              "be empty");
  TORCH_CHECK(!head.is_empty(),
              "[PerEdgeConditionedAffineReturnHead] cannot save a null head");
  head->validate_registered_state();
  per_edge_conditioned_affine_detail::validate_metadata(
      metadata, &head->options(), /*require_digest=*/false);
  auto stored = metadata;
  const auto digest = conditioned_affine_semantic_tensor_digest(head, stored);
  TORCH_CHECK(stored.semantic_tensor_digest.empty() ||
                  stored.semantic_tensor_digest == digest,
              "[PerEdgeConditionedAffineReturnHead] supplied semantic digest "
              "mismatch");
  stored.semantic_tensor_digest = digest;
  per_edge_conditioned_affine_detail::write_artifact_archive_unchecked(
      path, head, stored, canonical_conditioned_affine_metadata_text(stored));
}

struct LoadedPerEdgeConditionedAffineReturnHead {
  PerEdgeConditionedAffineReturnHead head{nullptr};
  PerEdgeConditionedAffineReturnHeadArtifactMetadata metadata{};
};

inline LoadedPerEdgeConditionedAffineReturnHead
load_per_edge_conditioned_affine_return_head(
    const std::filesystem::path &path) {
  TORCH_CHECK(!path.empty() && std::filesystem::exists(path),
              "[PerEdgeConditionedAffineReturnHead] artifact does not exist: ",
              path.string());
  torch::serialize::InputArchive root;
  root.load_from(path.string(), torch::Device(torch::kCPU));
  auto metadata = per_edge_conditioned_affine_detail::read_metadata(root);
  per_edge_conditioned_affine_detail::validate_metadata(
      metadata, nullptr, /*require_digest=*/true);
  const auto stored_canonical = per_edge_conditioned_affine_detail::read_string(
      root, "meta/canonical_metadata_text_bytes");
  TORCH_CHECK(stored_canonical ==
                  canonical_conditioned_affine_metadata_text(metadata),
              "[PerEdgeConditionedAffineReturnHead] canonical metadata text "
              "mismatch");
  const PerEdgeConditionedAffineReturnHeadOptions options{
      .feature_dim = metadata.feature_dim,
      .edge_count = metadata.edge_count,
      .channel_count = metadata.channel_count,
  };
  auto head = PerEdgeConditionedAffineReturnHead(options);
  torch::serialize::InputArchive model;
  root.read("model", model);
  head->load(model);
  head->eval();
  head->validate_registered_state();
  const auto digest = conditioned_affine_semantic_tensor_digest(head, metadata);
  TORCH_CHECK(digest == metadata.semantic_tensor_digest,
              "[PerEdgeConditionedAffineReturnHead] semantic tensor digest "
              "mismatch");
  return {.head = std::move(head), .metadata = std::move(metadata)};
}

} // namespace cuwacunu::wikimyei::inference::expected_value::mdn
