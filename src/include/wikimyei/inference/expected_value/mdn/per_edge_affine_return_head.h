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

inline constexpr std::int64_t kPerEdgeAffineReturnHeadSchemaVersion = 1;
inline constexpr const char *kPerEdgeAffineReturnHeadArtifactFamily =
    "wikimyei.inference.expected_value.mdn.per_edge_affine_return_head.v1";
inline constexpr const char *kPerEdgeAffineReturnHeadFeatureLayout =
    "base_quote_base_minus_quote_prefix.v1";
inline constexpr const char *kPerEdgeAffineReturnHeadSuffixPolicy =
    "ignored_after_3h";
inline constexpr const char *kPerEdgeAffineReturnHeadNormalizationLayout =
    "shared_featurewise_population.v1";
inline constexpr const char *kPerEdgeAffineReturnHeadCoefficientLayout =
    "standardized_per_edge_linear.v1";
inline constexpr const char *kPerEdgeAffineReturnHeadCoefficientSource =
    "analytic_ridge_refit";
inline constexpr const char *kPerEdgeAffineReturnHeadStatisticsScope =
    "development_fit_only";
inline constexpr const char *kPerEdgeAffineReturnHeadComputeDevice = "cpu";
inline constexpr const char *kPerEdgeAffineReturnHeadComputeDtype = "float64";
inline constexpr const char *kPerEdgeAffineReturnHeadOutputDtypePolicy =
    "match_input";

struct PerEdgeAffineReturnHeadOptions {
  std::int64_t feature_dim{0};
  std::int64_t edge_count{0};
  std::int64_t readout_feature_dim{0};
  std::int64_t channel_count{0};
  std::int64_t quote_node_index{0};
};

struct PerEdgeAffineReturnHeadState {
  torch::Tensor feature_mean{};    // [3H], float64 CPU
  torch::Tensor feature_inv_std{}; // [3H], float64 CPU
  torch::Tensor weights{};         // [E,3H], float64 CPU
  torch::Tensor bias{};            // [E], float64 CPU
};

struct PerEdgeAffineReturnHeadArtifactMetadata {
  std::int64_t schema_version{kPerEdgeAffineReturnHeadSchemaVersion};
  std::string artifact_family{kPerEdgeAffineReturnHeadArtifactFamily};
  bool diagnostic_only{true};

  std::int64_t feature_dim{0};
  std::int64_t edge_count{0};
  std::int64_t readout_feature_dim{0};
  std::int64_t channel_count{0};
  std::int64_t quote_node_index{0};

  std::string feature_layout{kPerEdgeAffineReturnHeadFeatureLayout};
  std::string suffix_policy{kPerEdgeAffineReturnHeadSuffixPolicy};
  std::string normalization_layout{kPerEdgeAffineReturnHeadNormalizationLayout};
  std::string coefficient_layout{kPerEdgeAffineReturnHeadCoefficientLayout};
  std::string coefficient_source{kPerEdgeAffineReturnHeadCoefficientSource};
  std::string statistics_scope{kPerEdgeAffineReturnHeadStatisticsScope};
  std::string compute_device{kPerEdgeAffineReturnHeadComputeDevice};
  std::string compute_dtype{kPerEdgeAffineReturnHeadComputeDtype};
  std::string output_dtype_policy{kPerEdgeAffineReturnHeadOutputDtypePolicy};

  double selected_alpha{0.0};
  std::int64_t selection_train_begin{0};
  std::int64_t selection_train_end{0};
  std::int64_t validation_purge_begin{0};
  std::int64_t validation_purge_end{0};
  std::int64_t validation_begin{0};
  std::int64_t validation_end{0};
  std::int64_t refit_begin{0};
  std::int64_t refit_end{0};
  std::int64_t valid_from_anchor{0};

  std::string graph_order_fingerprint{};
  std::vector<std::string> edge_ids{};
  std::vector<std::string> node_ids{};
  std::string fit_probe_schema_id{};
  std::string fit_probe_sha256{};
  std::string representation_checkpoint_sha256{};
  std::string mdn_checkpoint_sha256{};
  std::string legacy_capture_authority{};

  std::string semantic_tensor_digest{};
};

namespace per_edge_affine_detail {

inline void validate_options(const PerEdgeAffineReturnHeadOptions &options) {
  TORCH_CHECK(options.feature_dim > 0,
              "[PerEdgeAffineReturnHead] feature_dim must be positive");
  TORCH_CHECK(options.edge_count > 0,
              "[PerEdgeAffineReturnHead] edge_count must be positive");
  TORCH_CHECK(options.channel_count > 0,
              "[PerEdgeAffineReturnHead] channel_count must be positive");
  TORCH_CHECK(options.quote_node_index == 0,
              "[PerEdgeAffineReturnHead] v1 requires quote_node_index=0");
  TORCH_CHECK(options.readout_feature_dim >= 3 * options.feature_dim,
              "[PerEdgeAffineReturnHead] readout_feature_dim must be >= 3H");
}

inline void validate_metadata_string(const std::string &value, const char *name,
                                     bool required = true) {
  TORCH_CHECK(!required || !value.empty(), "[PerEdgeAffineReturnHead] ", name,
              " must not be empty");
  TORCH_CHECK(value.find('\n') == std::string::npos &&
                  value.find('\r') == std::string::npos,
              "[PerEdgeAffineReturnHead] ", name,
              " must not contain line breaks");
}

inline void validate_unique_ids(const std::vector<std::string> &values,
                                std::int64_t expected_size, const char *name) {
  TORCH_CHECK(static_cast<std::int64_t>(values.size()) == expected_size,
              "[PerEdgeAffineReturnHead] ", name, " size mismatch");
  std::set<std::string> unique;
  for (const auto &value : values) {
    validate_metadata_string(value, name);
    TORCH_CHECK(unique.insert(value).second,
                "[PerEdgeAffineReturnHead] duplicate ", name, " value");
  }
}

inline void
validate_metadata(const PerEdgeAffineReturnHeadArtifactMetadata &metadata,
                  const PerEdgeAffineReturnHeadOptions *expected_options,
                  bool require_digest) {
  TORCH_CHECK(metadata.schema_version == kPerEdgeAffineReturnHeadSchemaVersion,
              "[PerEdgeAffineReturnHead] unsupported schema_version");
  TORCH_CHECK(metadata.artifact_family ==
                  kPerEdgeAffineReturnHeadArtifactFamily,
              "[PerEdgeAffineReturnHead] unsupported artifact_family");
  TORCH_CHECK(metadata.diagnostic_only,
              "[PerEdgeAffineReturnHead] artifact must be diagnostic-only");
  const PerEdgeAffineReturnHeadOptions options{
      .feature_dim = metadata.feature_dim,
      .edge_count = metadata.edge_count,
      .readout_feature_dim = metadata.readout_feature_dim,
      .channel_count = metadata.channel_count,
      .quote_node_index = metadata.quote_node_index,
  };
  validate_options(options);
  if (expected_options != nullptr) {
    TORCH_CHECK(options.feature_dim == expected_options->feature_dim &&
                    options.edge_count == expected_options->edge_count &&
                    options.readout_feature_dim ==
                        expected_options->readout_feature_dim &&
                    options.channel_count == expected_options->channel_count &&
                    options.quote_node_index ==
                        expected_options->quote_node_index,
                "[PerEdgeAffineReturnHead] artifact/head dimensions mismatch");
  }
  TORCH_CHECK(
      metadata.feature_layout == kPerEdgeAffineReturnHeadFeatureLayout &&
          metadata.suffix_policy == kPerEdgeAffineReturnHeadSuffixPolicy &&
          metadata.normalization_layout ==
              kPerEdgeAffineReturnHeadNormalizationLayout &&
          metadata.coefficient_layout ==
              kPerEdgeAffineReturnHeadCoefficientLayout &&
          metadata.coefficient_source ==
              kPerEdgeAffineReturnHeadCoefficientSource &&
          metadata.statistics_scope ==
              kPerEdgeAffineReturnHeadStatisticsScope &&
          metadata.compute_device == kPerEdgeAffineReturnHeadComputeDevice &&
          metadata.compute_dtype == kPerEdgeAffineReturnHeadComputeDtype &&
          metadata.output_dtype_policy ==
              kPerEdgeAffineReturnHeadOutputDtypePolicy,
      "[PerEdgeAffineReturnHead] unsupported semantic metadata");
  TORCH_CHECK(std::isfinite(metadata.selected_alpha) &&
                  metadata.selected_alpha > 0.0,
              "[PerEdgeAffineReturnHead] selected_alpha must be positive");
  TORCH_CHECK(
      metadata.selection_train_begin >= 0 &&
          metadata.selection_train_begin < metadata.selection_train_end &&
          metadata.selection_train_end == metadata.validation_purge_begin &&
          metadata.validation_purge_begin <= metadata.validation_purge_end &&
          metadata.validation_purge_end == metadata.validation_begin &&
          metadata.validation_begin < metadata.validation_end &&
          metadata.refit_begin == metadata.selection_train_begin &&
          metadata.refit_end == metadata.validation_end &&
          metadata.valid_from_anchor == metadata.refit_end,
      "[PerEdgeAffineReturnHead] invalid development split metadata");
  validate_metadata_string(metadata.graph_order_fingerprint,
                           "graph_order_fingerprint");
  validate_unique_ids(metadata.edge_ids, metadata.edge_count, "edge_ids");
  validate_unique_ids(metadata.node_ids, metadata.edge_count + 1, "node_ids");
  validate_metadata_string(metadata.fit_probe_schema_id, "fit_probe_schema_id");
  validate_metadata_string(metadata.legacy_capture_authority,
                           "legacy_capture_authority");
  TORCH_CHECK(
      cuwacunu::piaabo::digest::is_sha256_hex(metadata.fit_probe_sha256) &&
          cuwacunu::piaabo::digest::is_sha256_hex(
              metadata.representation_checkpoint_sha256) &&
          cuwacunu::piaabo::digest::is_sha256_hex(
              metadata.mdn_checkpoint_sha256),
      "[PerEdgeAffineReturnHead] invalid fit/checkpoint SHA-256");
  if (require_digest) {
    TORCH_CHECK(cuwacunu::piaabo::digest::is_sha256_hex(
                    metadata.semantic_tensor_digest),
                "[PerEdgeAffineReturnHead] invalid semantic tensor digest");
  } else {
    TORCH_CHECK(metadata.semantic_tensor_digest.empty() ||
                    cuwacunu::piaabo::digest::is_sha256_hex(
                        metadata.semantic_tensor_digest),
                "[PerEdgeAffineReturnHead] invalid semantic tensor digest");
  }
}

inline void append_string(std::ostringstream &out, const std::string &key,
                          const std::string &value) {
  out << key << '=' << value.size() << ':' << value << '\n';
}

inline std::string canonical_metadata_without_digest(
    const PerEdgeAffineReturnHeadArtifactMetadata &metadata) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << "per_edge_affine_return_head_metadata.v1\n";
  out << "schema_version=" << metadata.schema_version << '\n';
  append_string(out, "artifact_family", metadata.artifact_family);
  out << "diagnostic_only=" << (metadata.diagnostic_only ? 1 : 0) << '\n';
  out << "feature_dim=" << metadata.feature_dim << '\n';
  out << "edge_count=" << metadata.edge_count << '\n';
  out << "readout_feature_dim=" << metadata.readout_feature_dim << '\n';
  out << "dynamic_feature_dim=" << 3 * metadata.feature_dim << '\n';
  out << "channel_count=" << metadata.channel_count << '\n';
  out << "quote_node_index=" << metadata.quote_node_index << '\n';
  append_string(out, "feature_layout", metadata.feature_layout);
  append_string(out, "suffix_policy", metadata.suffix_policy);
  append_string(out, "normalization_layout", metadata.normalization_layout);
  append_string(out, "coefficient_layout", metadata.coefficient_layout);
  append_string(out, "coefficient_source", metadata.coefficient_source);
  append_string(out, "statistics_scope", metadata.statistics_scope);
  append_string(out, "compute_device", metadata.compute_device);
  append_string(out, "compute_dtype", metadata.compute_dtype);
  append_string(out, "output_dtype_policy", metadata.output_dtype_policy);
  out << std::scientific << std::setprecision(17);
  out << "selected_alpha=" << metadata.selected_alpha << '\n';
  out << std::defaultfloat;
  out << "selection_train_begin=" << metadata.selection_train_begin << '\n';
  out << "selection_train_end=" << metadata.selection_train_end << '\n';
  out << "validation_purge_begin=" << metadata.validation_purge_begin << '\n';
  out << "validation_purge_end=" << metadata.validation_purge_end << '\n';
  out << "validation_begin=" << metadata.validation_begin << '\n';
  out << "validation_end=" << metadata.validation_end << '\n';
  out << "refit_begin=" << metadata.refit_begin << '\n';
  out << "refit_end=" << metadata.refit_end << '\n';
  out << "valid_from_anchor=" << metadata.valid_from_anchor << '\n';
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
  append_string(out, "fit_probe_schema_id", metadata.fit_probe_schema_id);
  append_string(out, "fit_probe_sha256", metadata.fit_probe_sha256);
  append_string(out, "representation_checkpoint_sha256",
                metadata.representation_checkpoint_sha256);
  append_string(out, "mdn_checkpoint_sha256", metadata.mdn_checkpoint_sha256);
  append_string(out, "legacy_capture_authority",
                metadata.legacy_capture_authority);
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
              "[PerEdgeAffineReturnHead] ", name, " bytes must be rank one");
  auto cpu = tensor.to(torch::kCPU).to(torch::kInt64).contiguous();
  std::string value;
  value.reserve(static_cast<std::size_t>(cpu.numel()));
  const auto accessor = cpu.accessor<std::int64_t, 1>();
  for (std::int64_t i = 0; i < cpu.numel(); ++i) {
    TORCH_CHECK(accessor[i] >= 0 && accessor[i] <= 255,
                "[PerEdgeAffineReturnHead] invalid byte in ", name);
    value.push_back(static_cast<char>(accessor[i]));
  }
  return value;
}

inline std::int64_t tensor_i64(const torch::Tensor &tensor, const char *name) {
  TORCH_CHECK(tensor.defined() && tensor.numel() == 1,
              "[PerEdgeAffineReturnHead] ", name, " must be scalar");
  return tensor.to(torch::kCPU).to(torch::kInt64).item<std::int64_t>();
}

inline double tensor_f64(const torch::Tensor &tensor, const char *name) {
  TORCH_CHECK(tensor.defined() && tensor.numel() == 1,
              "[PerEdgeAffineReturnHead] ", name, " must be scalar");
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
                "[PerEdgeAffineReturnHead] negative string length");
    const auto length = static_cast<std::size_t>(accessor[i]);
    TORCH_CHECK(offset + length <= bytes.size(),
                "[PerEdgeAffineReturnHead] invalid string vector bytes");
    values.push_back(bytes.substr(offset, length));
    offset += length;
  }
  TORCH_CHECK(offset == bytes.size(),
              "[PerEdgeAffineReturnHead] trailing string vector bytes");
  return values;
}

} // namespace per_edge_affine_detail

struct PerEdgeAffineReturnHeadImpl : torch::nn::Module {
  explicit PerEdgeAffineReturnHeadImpl(
      const PerEdgeAffineReturnHeadOptions &options)
      : options_(options) {
    per_edge_affine_detail::validate_options(options_);
    const auto dynamic_feature_dim = 3 * options_.feature_dim;
    const auto f64 = torch::TensorOptions().dtype(torch::kFloat64);
    feature_mean = register_buffer(
        "feature_mean", torch::zeros({1, 1, 1, dynamic_feature_dim}, f64));
    feature_inv_std = register_buffer(
        "feature_inv_std", torch::ones({1, 1, 1, dynamic_feature_dim}, f64));
    edge_projections.reserve(static_cast<std::size_t>(options_.edge_count));
    for (std::int64_t edge = 0; edge < options_.edge_count; ++edge) {
      auto projection = torch::nn::Linear(dynamic_feature_dim, 1);
      projection->to(torch::kFloat64);
      {
        torch::NoGradGuard no_grad;
        projection->weight.zero_();
        projection->bias.zero_();
      }
      projection->weight.set_requires_grad(false);
      projection->bias.set_requires_grad(false);
      edge_projections.push_back(register_module(
          "projection_edge_" + std::to_string(edge), projection));
    }
    eval();
    validate_registered_state();
  }

  PerEdgeAffineReturnHeadImpl(const PerEdgeAffineReturnHeadOptions &options,
                              const PerEdgeAffineReturnHeadState &state)
      : PerEdgeAffineReturnHeadImpl(options) {
    copy_affine_state(state);
  }

  [[nodiscard]] const PerEdgeAffineReturnHeadOptions &options() const {
    return options_;
  }

  [[nodiscard]] PerEdgeAffineReturnHeadState affine_state() const {
    validate_registered_state();
    std::vector<torch::Tensor> rows;
    rows.reserve(static_cast<std::size_t>(options_.edge_count));
    std::vector<torch::Tensor> biases;
    biases.reserve(static_cast<std::size_t>(options_.edge_count));
    for (const auto &projection : edge_projections) {
      rows.push_back(projection->weight.detach().reshape({-1}).clone());
      biases.push_back(projection->bias.detach().reshape({}).clone());
    }
    return {
        .feature_mean = feature_mean.detach().reshape({-1}).clone(),
        .feature_inv_std = feature_inv_std.detach().reshape({-1}).clone(),
        .weights = torch::stack(rows, /*dim=*/0),
        .bias = torch::stack(biases, /*dim=*/0),
    };
  }

  void copy_affine_state(const PerEdgeAffineReturnHeadState &state) {
    const auto dynamic_feature_dim = 3 * options_.feature_dim;
    validate_state_tensor(state.feature_mean, {dynamic_feature_dim},
                          "feature_mean");
    validate_state_tensor(state.feature_inv_std, {dynamic_feature_dim},
                          "feature_inv_std");
    validate_state_tensor(
        state.weights, {options_.edge_count, dynamic_feature_dim}, "weights");
    validate_state_tensor(state.bias, {options_.edge_count}, "bias");
    TORCH_CHECK((state.feature_inv_std > 0.0).all().item<bool>(),
                "[PerEdgeAffineReturnHead] feature_inv_std must be positive");
    torch::NoGradGuard no_grad;
    feature_mean.copy_(state.feature_mean.view_as(feature_mean));
    feature_inv_std.copy_(state.feature_inv_std.view_as(feature_inv_std));
    for (std::int64_t edge = 0; edge < options_.edge_count; ++edge) {
      auto &projection = edge_projections[static_cast<std::size_t>(edge)];
      projection->weight.copy_(
          state.weights.select(0, edge).view({1, dynamic_feature_dim}));
      projection->bias.copy_(state.bias.select(0, edge).view({1}));
      projection->weight.set_requires_grad(false);
      projection->bias.set_requires_grad(false);
    }
    eval();
    validate_registered_state();
  }

  void validate_registered_state() const {
    per_edge_affine_detail::validate_options(options_);
    const auto dynamic_feature_dim = 3 * options_.feature_dim;
    validate_registered_tensor(feature_mean, {1, 1, 1, dynamic_feature_dim},
                               "feature_mean");
    validate_registered_tensor(feature_inv_std, {1, 1, 1, dynamic_feature_dim},
                               "feature_inv_std");
    TORCH_CHECK((feature_inv_std > 0.0).all().item<bool>(),
                "[PerEdgeAffineReturnHead] feature_inv_std must be positive");
    TORCH_CHECK(static_cast<std::int64_t>(edge_projections.size()) ==
                    options_.edge_count,
                "[PerEdgeAffineReturnHead] projection count mismatch");
    for (const auto &projection : edge_projections) {
      TORCH_CHECK(!projection.is_empty(),
                  "[PerEdgeAffineReturnHead] null edge projection");
      validate_registered_tensor(projection->weight, {1, dynamic_feature_dim},
                                 "weight");
      validate_registered_tensor(projection->bias, {1}, "bias");
      TORCH_CHECK(!projection->weight.requires_grad() &&
                      !projection->bias.requires_grad(),
                  "[PerEdgeAffineReturnHead] parameters must remain frozen");
    }
  }

  // context is the post-DirectEdgeReturnHead::adapted_context tensor.
  torch::Tensor forward(const torch::Tensor &context) {
    validate_input(context, "context");
    TORCH_CHECK(context.dim() == 4 && context.size(0) > 0 &&
                    context.size(1) == options_.edge_count + 1 &&
                    context.size(2) == options_.channel_count &&
                    context.size(3) == options_.feature_dim,
                "[PerEdgeAffineReturnHead] context must be [B,E+1,C,H]");
    const auto B = context.size(0);
    const auto H = options_.feature_dim;
    auto quote =
        context.select(1, options_.quote_node_index)
            .unsqueeze(1)
            .expand({B, options_.edge_count, options_.channel_count, H});
    auto base = context.narrow(1, 1, options_.edge_count);
    auto dynamic =
        torch::cat({base, quote, base - quote}, /*dim=*/-1).contiguous();
    return forward_dynamic_features(dynamic, context.scalar_type());
  }

  torch::Tensor
  forward_readout_features(const torch::Tensor &readout_features) {
    validate_input(readout_features, "readout_features");
    TORCH_CHECK(readout_features.dim() == 4 && readout_features.size(0) > 0 &&
                    readout_features.size(1) == options_.edge_count &&
                    readout_features.size(2) == options_.channel_count &&
                    readout_features.size(3) == options_.readout_feature_dim,
                "[PerEdgeAffineReturnHead] readout features must be "
                "[B,E,C,F] with exact configured F");
    auto dynamic = readout_features.narrow(
        /*dim=*/3, /*start=*/0, /*length=*/3 * options_.feature_dim);
    return forward_dynamic_features(dynamic, readout_features.scalar_type());
  }

  torch::Tensor feature_mean;
  torch::Tensor feature_inv_std;
  std::vector<torch::nn::Linear> edge_projections{};

private:
  static void validate_state_tensor(const torch::Tensor &tensor,
                                    torch::IntArrayRef shape,
                                    const char *name) {
    TORCH_CHECK(tensor.defined() && tensor.device().is_cpu() &&
                    tensor.scalar_type() == torch::kFloat64 &&
                    tensor.sizes() == shape &&
                    torch::isfinite(tensor).all().item<bool>(),
                "[PerEdgeAffineReturnHead] invalid float64 CPU state tensor ",
                name);
  }

  static void validate_registered_tensor(const torch::Tensor &tensor,
                                         torch::IntArrayRef shape,
                                         const char *name) {
    validate_state_tensor(tensor, shape, name);
  }

  void validate_input(const torch::Tensor &tensor, const char *name) const {
    validate_registered_state();
    TORCH_CHECK(tensor.defined() && tensor.device().is_cpu(),
                "[PerEdgeAffineReturnHead] ", name, " must be on CPU");
    TORCH_CHECK(tensor.scalar_type() == torch::kFloat32 ||
                    tensor.scalar_type() == torch::kFloat64,
                "[PerEdgeAffineReturnHead] ", name,
                " must be float32 or float64");
    TORCH_CHECK(torch::isfinite(tensor).all().item<bool>(),
                "[PerEdgeAffineReturnHead] ", name, " must be finite");
  }

  torch::Tensor forward_dynamic_features(const torch::Tensor &dynamic,
                                         torch::ScalarType output_dtype) {
    const auto B = dynamic.size(0);
    const auto C = dynamic.size(2);
    const auto dynamic_feature_dim = 3 * options_.feature_dim;
    auto standardized =
        (dynamic.to(torch::kFloat64) - feature_mean) * feature_inv_std;
    using torch::indexing::Slice;
    std::vector<torch::Tensor> outputs;
    outputs.reserve(static_cast<std::size_t>(options_.edge_count));
    for (std::int64_t edge = 0; edge < options_.edge_count; ++edge) {
      auto input = standardized.index({Slice(), edge, Slice(), Slice()})
                       .contiguous()
                       .view({B * C, dynamic_feature_dim});
      outputs.push_back(
          edge_projections[static_cast<std::size_t>(edge)]->forward(input).view(
              {B, 1, C}));
    }
    return torch::cat(outputs, /*dim=*/1).to(output_dtype);
  }

  PerEdgeAffineReturnHeadOptions options_{};
};
TORCH_MODULE(PerEdgeAffineReturnHead);

inline std::string canonical_metadata_text(
    const PerEdgeAffineReturnHeadArtifactMetadata &metadata) {
  auto text =
      per_edge_affine_detail::canonical_metadata_without_digest(metadata);
  std::ostringstream digest_line;
  per_edge_affine_detail::append_string(digest_line, "semantic_tensor_digest",
                                        metadata.semantic_tensor_digest);
  text += digest_line.str();
  return text;
}

namespace per_edge_affine_detail {

inline void append_u64_be(std::string &bytes, std::uint64_t value) {
  for (int shift = 56; shift >= 0; shift -= 8) {
    bytes.push_back(static_cast<char>((value >> shift) & 0xffU));
  }
}

inline void append_tensor_f64_be(std::string &bytes, const std::string &name,
                                 const torch::Tensor &tensor) {
  auto flat = tensor.detach()
                  .to(torch::kCPU)
                  .to(torch::kFloat64)
                  .contiguous()
                  .view({-1});
  bytes += "tensor=" + name + "\n";
  bytes += "count=" + std::to_string(flat.numel()) + "\n";
  const auto accessor = flat.accessor<double, 1>();
  for (std::int64_t i = 0; i < flat.numel(); ++i) {
    append_u64_be(bytes, std::bit_cast<std::uint64_t>(accessor[i]));
  }
}

} // namespace per_edge_affine_detail

inline std::string semantic_tensor_digest(
    const PerEdgeAffineReturnHead &head,
    const PerEdgeAffineReturnHeadArtifactMetadata &metadata) {
  TORCH_CHECK(!head.is_empty(),
              "[PerEdgeAffineReturnHead] cannot digest a null head");
  head->validate_registered_state();
  per_edge_affine_detail::validate_metadata(metadata, &head->options(),
                                            /*require_digest=*/false);
  const auto state = head->affine_state();
  std::string bytes =
      per_edge_affine_detail::canonical_metadata_without_digest(metadata);
  bytes += "semantic_tensor_encoding=ieee754_binary64_big_endian.v1\n";
  per_edge_affine_detail::append_tensor_f64_be(bytes, "feature_mean",
                                               state.feature_mean);
  per_edge_affine_detail::append_tensor_f64_be(bytes, "feature_inv_std",
                                               state.feature_inv_std);
  per_edge_affine_detail::append_tensor_f64_be(bytes, "weights", state.weights);
  per_edge_affine_detail::append_tensor_f64_be(bytes, "bias", state.bias);
  return cuwacunu::piaabo::digest::sha256_hex(bytes);
}

namespace per_edge_affine_detail {

inline void
write_metadata(torch::serialize::OutputArchive &root,
               const PerEdgeAffineReturnHeadArtifactMetadata &metadata) {
  root.write("meta/schema_version", int64_tensor(metadata.schema_version));
  write_string(root, "meta/artifact_family_bytes", metadata.artifact_family);
  root.write("meta/diagnostic_only",
             int64_tensor(metadata.diagnostic_only ? 1 : 0));
  root.write("meta/feature_dim", int64_tensor(metadata.feature_dim));
  root.write("meta/edge_count", int64_tensor(metadata.edge_count));
  root.write("meta/readout_feature_dim",
             int64_tensor(metadata.readout_feature_dim));
  root.write("meta/channel_count", int64_tensor(metadata.channel_count));
  root.write("meta/quote_node_index", int64_tensor(metadata.quote_node_index));
  write_string(root, "meta/feature_layout_bytes", metadata.feature_layout);
  write_string(root, "meta/suffix_policy_bytes", metadata.suffix_policy);
  write_string(root, "meta/normalization_layout_bytes",
               metadata.normalization_layout);
  write_string(root, "meta/coefficient_layout_bytes",
               metadata.coefficient_layout);
  write_string(root, "meta/coefficient_source_bytes",
               metadata.coefficient_source);
  write_string(root, "meta/statistics_scope_bytes", metadata.statistics_scope);
  write_string(root, "meta/compute_device_bytes", metadata.compute_device);
  write_string(root, "meta/compute_dtype_bytes", metadata.compute_dtype);
  write_string(root, "meta/output_dtype_policy_bytes",
               metadata.output_dtype_policy);
  root.write("meta/selected_alpha", float64_tensor(metadata.selected_alpha));
  root.write("meta/selection_train_begin",
             int64_tensor(metadata.selection_train_begin));
  root.write("meta/selection_train_end",
             int64_tensor(metadata.selection_train_end));
  root.write("meta/validation_purge_begin",
             int64_tensor(metadata.validation_purge_begin));
  root.write("meta/validation_purge_end",
             int64_tensor(metadata.validation_purge_end));
  root.write("meta/validation_begin", int64_tensor(metadata.validation_begin));
  root.write("meta/validation_end", int64_tensor(metadata.validation_end));
  root.write("meta/refit_begin", int64_tensor(metadata.refit_begin));
  root.write("meta/refit_end", int64_tensor(metadata.refit_end));
  root.write("meta/valid_from_anchor",
             int64_tensor(metadata.valid_from_anchor));
  write_string(root, "meta/graph_order_fingerprint_bytes",
               metadata.graph_order_fingerprint);
  write_string_vector(root, "meta/edge_ids", metadata.edge_ids);
  write_string_vector(root, "meta/node_ids", metadata.node_ids);
  write_string(root, "meta/fit_probe_schema_id_bytes",
               metadata.fit_probe_schema_id);
  write_string(root, "meta/fit_probe_sha256_bytes", metadata.fit_probe_sha256);
  write_string(root, "meta/representation_checkpoint_sha256_bytes",
               metadata.representation_checkpoint_sha256);
  write_string(root, "meta/mdn_checkpoint_sha256_bytes",
               metadata.mdn_checkpoint_sha256);
  write_string(root, "meta/legacy_capture_authority_bytes",
               metadata.legacy_capture_authority);
  write_string(root, "meta/semantic_tensor_digest_bytes",
               metadata.semantic_tensor_digest);
}

inline PerEdgeAffineReturnHeadArtifactMetadata
read_metadata(torch::serialize::InputArchive &root) {
  PerEdgeAffineReturnHeadArtifactMetadata metadata;
  const auto read_tensor = [&](const std::string &key) {
    torch::Tensor value;
    root.read(key, value);
    return value;
  };
  metadata.schema_version =
      tensor_i64(read_tensor("meta/schema_version"), "schema_version");
  metadata.artifact_family = read_string(root, "meta/artifact_family_bytes");
  const auto diagnostic_only =
      tensor_i64(read_tensor("meta/diagnostic_only"), "diagnostic_only");
  TORCH_CHECK(diagnostic_only == 0 || diagnostic_only == 1,
              "[PerEdgeAffineReturnHead] diagnostic_only must be 0 or 1");
  metadata.diagnostic_only = diagnostic_only == 1;
  metadata.feature_dim =
      tensor_i64(read_tensor("meta/feature_dim"), "feature_dim");
  metadata.edge_count =
      tensor_i64(read_tensor("meta/edge_count"), "edge_count");
  metadata.readout_feature_dim = tensor_i64(
      read_tensor("meta/readout_feature_dim"), "readout_feature_dim");
  metadata.channel_count =
      tensor_i64(read_tensor("meta/channel_count"), "channel_count");
  metadata.quote_node_index =
      tensor_i64(read_tensor("meta/quote_node_index"), "quote_node_index");
  metadata.feature_layout = read_string(root, "meta/feature_layout_bytes");
  metadata.suffix_policy = read_string(root, "meta/suffix_policy_bytes");
  metadata.normalization_layout =
      read_string(root, "meta/normalization_layout_bytes");
  metadata.coefficient_layout =
      read_string(root, "meta/coefficient_layout_bytes");
  metadata.coefficient_source =
      read_string(root, "meta/coefficient_source_bytes");
  metadata.statistics_scope = read_string(root, "meta/statistics_scope_bytes");
  metadata.compute_device = read_string(root, "meta/compute_device_bytes");
  metadata.compute_dtype = read_string(root, "meta/compute_dtype_bytes");
  metadata.output_dtype_policy =
      read_string(root, "meta/output_dtype_policy_bytes");
  metadata.selected_alpha =
      tensor_f64(read_tensor("meta/selected_alpha"), "selected_alpha");
  metadata.selection_train_begin = tensor_i64(
      read_tensor("meta/selection_train_begin"), "selection_train_begin");
  metadata.selection_train_end = tensor_i64(
      read_tensor("meta/selection_train_end"), "selection_train_end");
  metadata.validation_purge_begin = tensor_i64(
      read_tensor("meta/validation_purge_begin"), "validation_purge_begin");
  metadata.validation_purge_end = tensor_i64(
      read_tensor("meta/validation_purge_end"), "validation_purge_end");
  metadata.validation_begin =
      tensor_i64(read_tensor("meta/validation_begin"), "validation_begin");
  metadata.validation_end =
      tensor_i64(read_tensor("meta/validation_end"), "validation_end");
  metadata.refit_begin =
      tensor_i64(read_tensor("meta/refit_begin"), "refit_begin");
  metadata.refit_end = tensor_i64(read_tensor("meta/refit_end"), "refit_end");
  metadata.valid_from_anchor =
      tensor_i64(read_tensor("meta/valid_from_anchor"), "valid_from_anchor");
  metadata.graph_order_fingerprint =
      read_string(root, "meta/graph_order_fingerprint_bytes");
  metadata.edge_ids = read_string_vector(root, "meta/edge_ids");
  metadata.node_ids = read_string_vector(root, "meta/node_ids");
  metadata.fit_probe_schema_id =
      read_string(root, "meta/fit_probe_schema_id_bytes");
  metadata.fit_probe_sha256 = read_string(root, "meta/fit_probe_sha256_bytes");
  metadata.representation_checkpoint_sha256 =
      read_string(root, "meta/representation_checkpoint_sha256_bytes");
  metadata.mdn_checkpoint_sha256 =
      read_string(root, "meta/mdn_checkpoint_sha256_bytes");
  metadata.legacy_capture_authority =
      read_string(root, "meta/legacy_capture_authority_bytes");
  metadata.semantic_tensor_digest =
      read_string(root, "meta/semantic_tensor_digest_bytes");
  return metadata;
}

// Internal writer used by the checked save path and focused corruption tests.
inline void write_artifact_archive_unchecked(
    const std::filesystem::path &path, const PerEdgeAffineReturnHead &head,
    const PerEdgeAffineReturnHeadArtifactMetadata &metadata,
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

} // namespace per_edge_affine_detail

inline void save_per_edge_affine_return_head(
    const std::filesystem::path &path, const PerEdgeAffineReturnHead &head,
    const PerEdgeAffineReturnHeadArtifactMetadata &metadata) {
  TORCH_CHECK(!path.empty(),
              "[PerEdgeAffineReturnHead] artifact path must not be empty");
  TORCH_CHECK(!head.is_empty(),
              "[PerEdgeAffineReturnHead] cannot save a null head");
  head->validate_registered_state();
  per_edge_affine_detail::validate_metadata(metadata, &head->options(),
                                            /*require_digest=*/false);
  auto stored = metadata;
  const auto digest = semantic_tensor_digest(head, stored);
  TORCH_CHECK(stored.semantic_tensor_digest.empty() ||
                  stored.semantic_tensor_digest == digest,
              "[PerEdgeAffineReturnHead] supplied semantic digest mismatch");
  stored.semantic_tensor_digest = digest;
  per_edge_affine_detail::write_artifact_archive_unchecked(
      path, head, stored, canonical_metadata_text(stored));
}

struct LoadedPerEdgeAffineReturnHead {
  PerEdgeAffineReturnHead head{nullptr};
  PerEdgeAffineReturnHeadArtifactMetadata metadata{};
};

inline LoadedPerEdgeAffineReturnHead
load_per_edge_affine_return_head(const std::filesystem::path &path) {
  TORCH_CHECK(
      !path.empty() && std::filesystem::exists(path),
      "[PerEdgeAffineReturnHead] artifact does not exist: ", path.string());
  torch::serialize::InputArchive root;
  root.load_from(path.string(), torch::Device(torch::kCPU));
  auto metadata = per_edge_affine_detail::read_metadata(root);
  per_edge_affine_detail::validate_metadata(metadata, nullptr,
                                            /*require_digest=*/true);
  const auto stored_canonical = per_edge_affine_detail::read_string(
      root, "meta/canonical_metadata_text_bytes");
  TORCH_CHECK(stored_canonical == canonical_metadata_text(metadata),
              "[PerEdgeAffineReturnHead] canonical metadata text mismatch");
  const PerEdgeAffineReturnHeadOptions options{
      .feature_dim = metadata.feature_dim,
      .edge_count = metadata.edge_count,
      .readout_feature_dim = metadata.readout_feature_dim,
      .channel_count = metadata.channel_count,
      .quote_node_index = metadata.quote_node_index,
  };
  auto head = PerEdgeAffineReturnHead(options);
  torch::serialize::InputArchive model;
  root.read("model", model);
  head->load(model);
  head->eval();
  head->validate_registered_state();
  const auto digest = semantic_tensor_digest(head, metadata);
  TORCH_CHECK(digest == metadata.semantic_tensor_digest,
              "[PerEdgeAffineReturnHead] semantic tensor digest mismatch");
  return {.head = std::move(head), .metadata = std::move(metadata)};
}

} // namespace cuwacunu::wikimyei::inference::expected_value::mdn
