// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

#include <torch/torch.h>

#include "ujcamei/source/types/kline_feature_registry.h"
#include "wikimyei/expression/nodelift/srl/stream/node_lifted_stream.h"

namespace cuwacunu::wikimyei::representation::encoding::vicreg {

enum class node_mask_reduce_policy_t {
  all_features,
  any_feature,
  required_features,
};

struct node_vicreg_adapter_options_t {
  node_mask_reduce_policy_t mask_policy{
      node_mask_reduce_policy_t::required_features};
  std::vector<int64_t> required_feature_coords{0, 1, 2, 3, 4, 5, 6, 7, 8};
  bool compact_valid_nodes_for_training{false};
  bool require_finite_features{true};
};

struct node_encoder_input_diagnostics_t {
  int64_t original_rows{0};
  int64_t kept_rows{0};
  int64_t dropped_rows{0};
  int64_t valid_channel_time_count{0};
  double valid_channel_time_fraction{0.0};
  int64_t kept_valid_channel_time_count{0};
  double kept_valid_channel_time_fraction{0.0};
};

struct node_encoder_input_t {
  torch::Tensor data{}; // [M,C,Hx,9]
  torch::Tensor mask{}; // [M,C,Hx], bool

  torch::Tensor anchor_index{}; // [M], int64
  torch::Tensor node_index{};   // [M], int64

  int64_t B_anchor{0};
  int64_t N{0};

  std::vector<std::string> node_ids{};
  torch::Tensor anchor_keys{};
  node_encoder_input_diagnostics_t diagnostics{};
};

[[nodiscard]] inline node_vicreg_adapter_options_t
node_vicreg_encoding_adapter_options() {
  node_vicreg_adapter_options_t out{};
  out.compact_valid_nodes_for_training = false;
  return out;
}

[[nodiscard]] inline node_vicreg_adapter_options_t
node_vicreg_training_adapter_options() {
  node_vicreg_adapter_options_t out{};
  out.compact_valid_nodes_for_training = true;
  return out;
}

[[nodiscard]] inline std::vector<int64_t> node_vicreg_all_9_feature_coords() {
  return {0, 1, 2, 3, 4, 5, 6, 7, 8};
}

[[nodiscard]] inline std::vector<int64_t> node_vicreg_price_feature_coords() {
  return {0, 1, 2, 3};
}

[[nodiscard]] inline std::vector<int64_t> node_vicreg_close_feature_coords() {
  return {3};
}

[[nodiscard]] inline std::vector<int64_t>
node_vicreg_activity_feature_coords() {
  return {4, 5, 6, 7, 8};
}

namespace node_stream_adapter_detail {

inline void validate_required_feature_coords(const std::vector<int64_t> &coords,
                                             int64_t feature_width) {
  TORCH_CHECK(
      !coords.empty(),
      "[node_stream_adapter] required_feature_coords must not be empty");
  std::unordered_set<int64_t> seen;
  seen.reserve(coords.size());
  for (const int64_t coord : coords) {
    TORCH_CHECK(coord >= 0 && coord < feature_width,
                "[node_stream_adapter] required feature coord out of range: ",
                coord, " for feature width ", feature_width);
    TORCH_CHECK(
        seen.insert(coord).second,
        "[node_stream_adapter] duplicate required feature coord: ", coord);
  }
}

inline torch::Tensor reduce_node_mask(const torch::Tensor &mask_bnchd,
                                      const node_vicreg_adapter_options_t &opts,
                                      int64_t feature_width) {
  TORCH_CHECK(mask_bnchd.dim() == 5,
              "[node_stream_adapter] internal node mask must be [B,N,C,H,D]");
  switch (opts.mask_policy) {
  case node_mask_reduce_policy_t::all_features:
    return mask_bnchd.all(/*dim=*/4);
  case node_mask_reduce_policy_t::any_feature:
    return mask_bnchd.any(/*dim=*/4);
  case node_mask_reduce_policy_t::required_features: {
    validate_required_feature_coords(opts.required_feature_coords,
                                     feature_width);
    auto idx = torch::tensor(opts.required_feature_coords,
                             torch::TensorOptions()
                                 .dtype(torch::kInt64)
                                 .device(mask_bnchd.device()));
    return mask_bnchd.index_select(/*dim=*/4, idx).all(/*dim=*/4);
  }
  default:
    TORCH_CHECK(false, "[node_stream_adapter] unknown node mask reduce policy");
  }
}

} // namespace node_stream_adapter_detail

template <typename KeyT>
[[nodiscard]] node_encoder_input_t
make_node_encoder_input(const cuwacunu::wikimyei::expression::nodelift::srl::
                            stream::node_lifted_batch_t<KeyT> &batch,
                        const node_vicreg_adapter_options_t &options = {}) {
  TORCH_CHECK(batch.node_features.defined(),
              "[node_stream_adapter] node_features is undefined");
  TORCH_CHECK(batch.node_mask.defined(),
              "[node_stream_adapter] node_mask is undefined");
  TORCH_CHECK(batch.node_features.dim() == 5,
              "[node_stream_adapter] node_features must be [B,C,Hx,N,D]");
  TORCH_CHECK(batch.node_mask.dim() == 5,
              "[node_stream_adapter] node_mask must be [B,C,Hx,N,D]");
  TORCH_CHECK(batch.node_features.sizes() == batch.node_mask.sizes(),
              "[node_stream_adapter] node_features/node_mask shape mismatch");
  TORCH_CHECK(batch.node_features.is_floating_point(),
              "[node_stream_adapter] node_features must be floating point");

  const int64_t B = batch.node_features.size(0);
  const int64_t C = batch.node_features.size(1);
  const int64_t Hx = batch.node_features.size(2);
  const int64_t N = batch.node_features.size(3);
  const int64_t D = batch.node_features.size(4);

  TORCH_CHECK(
      B > 0 && C > 0 && Hx > 0 && N > 0 && D > 0,
      "[node_stream_adapter] node_features dimensions must be positive");
  TORCH_CHECK(D == cuwacunu::ujcamei::source::types::kKlineFeatureWidth,
              "[node_stream_adapter] v1 expects kline feature width 9, got ",
              D);
  TORCH_CHECK(batch.node_ids.empty() ||
                  static_cast<int64_t>(batch.node_ids.size()) == N,
              "[node_stream_adapter] node_ids size must match N");
  TORCH_CHECK(!batch.anchor_keys.defined() || batch.anchor_keys.dim() == 1,
              "[node_stream_adapter] anchor_keys must be [B] when defined");
  TORCH_CHECK(!batch.anchor_keys.defined() || batch.anchor_keys.size(0) == B,
              "[node_stream_adapter] anchor_keys size must match B");

  auto coord_mask = batch.node_mask.to(torch::kBool);
  const auto finite = torch::isfinite(batch.node_features);
  if (options.require_finite_features) {
    const bool invalid_valid_feature =
        (coord_mask.logical_and(finite.logical_not()))
            .any()
            .template item<bool>();
    TORCH_CHECK(!invalid_valid_feature,
                "[node_stream_adapter] non-finite value found under a true "
                "node_mask coordinate");
  }
  coord_mask = coord_mask.logical_and(finite);
  auto clean_features = torch::where(finite, batch.node_features,
                                     torch::zeros_like(batch.node_features));

  auto x_bnchd =
      clean_features.permute({0, 3, 1, 2, 4}).contiguous(); // [B,N,C,H,D]
  auto m_bnchd = coord_mask.permute({0, 3, 1, 2, 4}).contiguous();

  auto reduced_bnch = node_stream_adapter_detail::reduce_node_mask(
      m_bnchd, options, D); // [B,N,C,H]

  auto data = x_bnchd.reshape({B * N, C, Hx, D});
  auto mask = reduced_bnch.reshape({B * N, C, Hx}).to(torch::kBool);
  data =
      data.masked_fill(mask.logical_not().unsqueeze(-1).expand_as(data), 0.0);

  const int64_t original_rows = B * N;
  const int64_t original_valid_channel_time_count =
      mask.sum().template item<int64_t>();
  const double original_valid_channel_time_fraction =
      (original_rows > 0 && C > 0 && Hx > 0)
          ? static_cast<double>(original_valid_channel_time_count) /
                static_cast<double>(original_rows * C * Hx)
          : 0.0;

  auto index_opts =
      torch::TensorOptions().dtype(torch::kInt64).device(data.device());
  auto anchor_index =
      torch::arange(B, index_opts).unsqueeze(1).repeat({1, N}).reshape({B * N});
  auto node_index =
      torch::arange(N, index_opts).unsqueeze(0).repeat({B, 1}).reshape({B * N});

  if (options.compact_valid_nodes_for_training) {
    auto valid_node_window = mask.any(/*dim=*/1).any(/*dim=*/1);
    auto keep = valid_node_window.nonzero().reshape({-1});
    data = data.index_select(/*dim=*/0, keep);
    mask = mask.index_select(/*dim=*/0, keep);
    anchor_index = anchor_index.index_select(/*dim=*/0, keep);
    node_index = node_index.index_select(/*dim=*/0, keep);
  }

  const int64_t kept_rows = data.size(0);
  const int64_t kept_valid_channel_time_count =
      mask.sum().template item<int64_t>();
  const double kept_valid_channel_time_fraction =
      (kept_rows > 0 && C > 0 && Hx > 0)
          ? static_cast<double>(kept_valid_channel_time_count) /
                static_cast<double>(kept_rows * C * Hx)
          : 0.0;

  node_encoder_input_t out{};
  out.data = std::move(data);
  out.mask = std::move(mask);
  out.anchor_index = std::move(anchor_index);
  out.node_index = std::move(node_index);
  out.B_anchor = B;
  out.N = N;
  out.node_ids = batch.node_ids;
  out.anchor_keys = batch.anchor_keys;
  out.diagnostics.original_rows = original_rows;
  out.diagnostics.kept_rows = kept_rows;
  out.diagnostics.dropped_rows = original_rows - kept_rows;
  out.diagnostics.valid_channel_time_count = original_valid_channel_time_count;
  out.diagnostics.valid_channel_time_fraction =
      original_valid_channel_time_fraction;
  out.diagnostics.kept_valid_channel_time_count = kept_valid_channel_time_count;
  out.diagnostics.kept_valid_channel_time_fraction =
      kept_valid_channel_time_fraction;
  return out;
}

[[nodiscard]] inline torch::Tensor
reshape_node_window_mask(const node_encoder_input_t &input) {
  TORCH_CHECK(input.mask.defined(),
              "[node_stream_adapter] input mask is undefined");
  TORCH_CHECK(input.anchor_index.defined() && input.node_index.defined(),
              "[node_stream_adapter] anchor_index/node_index are required");
  TORCH_CHECK(input.B_anchor > 0 && input.N > 0,
              "[node_stream_adapter] B_anchor and N must be positive");
  TORCH_CHECK(input.mask.dim() == 3,
              "[node_stream_adapter] input mask must be [M,C,Hx]");
  TORCH_CHECK(input.mask.size(0) == input.anchor_index.size(0) &&
                  input.mask.size(0) == input.node_index.size(0),
              "[node_stream_adapter] mask row count does not match "
              "anchor/node index count");

  const auto opts =
      torch::TensorOptions().dtype(torch::kBool).device(input.mask.device());
  auto out = torch::zeros({input.B_anchor, input.N}, opts);
  auto anchor = input.anchor_index.to(
      torch::TensorOptions().dtype(torch::kInt64).device(input.mask.device()));
  auto node = input.node_index.to(
      torch::TensorOptions().dtype(torch::kInt64).device(input.mask.device()));
  auto row_valid = input.mask.to(torch::kBool).any(/*dim=*/1).any(/*dim=*/1);
  out.index_put_({anchor, node}, row_valid);
  return out;
}

[[nodiscard]] inline torch::Tensor
reshape_node_encoding(const torch::Tensor &encoded,
                      const node_encoder_input_t &input) {
  TORCH_CHECK(encoded.defined(),
              "[node_stream_adapter] encoded tensor is undefined");
  TORCH_CHECK(input.anchor_index.defined() && input.node_index.defined(),
              "[node_stream_adapter] anchor_index/node_index are required");
  TORCH_CHECK(input.B_anchor > 0 && input.N > 0,
              "[node_stream_adapter] B_anchor and N must be positive");
  TORCH_CHECK(encoded.size(0) == input.anchor_index.size(0) &&
                  encoded.size(0) == input.node_index.size(0),
              "[node_stream_adapter] encoded row count does not match "
              "anchor/node index count");

  auto anchor = input.anchor_index.to(
      torch::TensorOptions().dtype(torch::kInt64).device(encoded.device()));
  auto node = input.node_index.to(
      torch::TensorOptions().dtype(torch::kInt64).device(encoded.device()));
  TORCH_CHECK((anchor >= 0).all().item<bool>() &&
                  (anchor < input.B_anchor).all().item<bool>(),
              "[node_stream_adapter] anchor_index out of range");
  TORCH_CHECK((node >= 0).all().item<bool>() &&
                  (node < input.N).all().item<bool>(),
              "[node_stream_adapter] node_index out of range");

  using torch::indexing::Slice;
  if (encoded.dim() == 2) {
    auto out = torch::zeros({input.B_anchor, input.N, encoded.size(1)},
                            encoded.options());
    out.index_put_({anchor, node, Slice()}, encoded);
    return out;
  }
  if (encoded.dim() == 3) {
    auto out = torch::zeros(
        {input.B_anchor, input.N, encoded.size(1), encoded.size(2)},
        encoded.options());
    out.index_put_({anchor, node, Slice(), Slice()}, encoded);
    return out;
  }
  TORCH_CHECK(false,
              "[node_stream_adapter] encoded must be [M,D_e] or [M,Hx,D_e]");
}

template <typename ModelT, typename KeyT>
[[nodiscard]] auto train_vicreg_on_node_lifted_batch(
    ModelT &model,
    const cuwacunu::wikimyei::expression::nodelift::srl::stream::
        node_lifted_batch_t<KeyT> &batch,
    const node_vicreg_adapter_options_t &adapter_options = {},
    int swa_start_iter = -1, bool verbose = false) {
  auto input = make_node_encoder_input(batch, adapter_options);
  return model.train_one_batch(input.data, input.mask, swa_start_iter, verbose);
}

template <typename ModelT, typename KeyT>
[[nodiscard]] auto train_vicreg_on_node_lifted_batch(
    ModelT &model,
    const cuwacunu::wikimyei::expression::nodelift::srl::stream::
        node_lifted_batch_t<KeyT> &batch,
    int swa_start_iter = -1, bool verbose = false) {
  return train_vicreg_on_node_lifted_batch(
      model, batch, node_vicreg_training_adapter_options(), swa_start_iter,
      verbose);
}

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg
