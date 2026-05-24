// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/expression/nodelift/srl/stream/node_lifted_stream.h"
#include "wikimyei/representation/encoding/vicreg/channel_representation_adapter.h"

namespace cuwacunu::wikimyei::representation::encoding::vicreg {

struct channel_node_encoder_input_diagnostics_t {
  int64_t original_rows{0};
  int64_t valid_feature_count{0};
  int64_t finite_feature_count{0};
  double valid_feature_fraction{0.0};
  int64_t valid_cell_any_count{0};
  int64_t valid_cell_all_count{0};
};

struct channel_node_encoder_input_t {
  torch::Tensor data{};          // [M,C,Hx,Dx]
  torch::Tensor feature_mask{};  // [M,C,Hx,Dx], bool
  torch::Tensor cell_mask_any{}; // [M,C,Hx], derived, bool
  torch::Tensor cell_mask_all{}; // [M,C,Hx], derived, bool

  torch::Tensor anchor_index{}; // [M], int64
  torch::Tensor node_index{};   // [M], int64
  int64_t B_anchor{0};
  int64_t N{0};

  std::vector<std::string> node_ids{};
  torch::Tensor anchor_keys{};
  channel_node_encoder_input_diagnostics_t diagnostics{};
};

template <typename KeyT>
[[nodiscard]] channel_node_encoder_input_t make_channel_node_encoder_input(
    const cuwacunu::wikimyei::expression::nodelift::srl::stream::
        node_lifted_batch_t<KeyT> &batch,
    bool require_finite_valid_features = true) {
  TORCH_CHECK(batch.node_features.defined(),
              "[channel_node_stream_adapter] node_features is undefined");
  TORCH_CHECK(batch.node_mask.defined(),
              "[channel_node_stream_adapter] node_mask is undefined");
  TORCH_CHECK(batch.node_features.dim() == 5,
              "[channel_node_stream_adapter] node_features must be "
              "[B,C,Hx,N,Dx]");
  TORCH_CHECK(batch.node_mask.dim() == 5,
              "[channel_node_stream_adapter] node_mask must be "
              "[B,C,Hx,N,Dx]");
  TORCH_CHECK(batch.node_features.sizes() == batch.node_mask.sizes(),
              "[channel_node_stream_adapter] feature/mask shape mismatch");
  TORCH_CHECK(batch.node_features.is_floating_point(),
              "[channel_node_stream_adapter] node_features must be floating");

  const int64_t B = batch.node_features.size(0);
  const int64_t C = batch.node_features.size(1);
  const int64_t Hx = batch.node_features.size(2);
  const int64_t N = batch.node_features.size(3);
  const int64_t Dx = batch.node_features.size(4);
  TORCH_CHECK(B > 0 && C > 0 && Hx > 0 && N > 0 && Dx > 0,
              "[channel_node_stream_adapter] dimensions must be positive");
  TORCH_CHECK(batch.node_ids.empty() ||
                  static_cast<int64_t>(batch.node_ids.size()) == N,
              "[channel_node_stream_adapter] node_ids size must match N");

  auto raw_mask = batch.node_mask.to(torch::kBool);
  auto finite = torch::isfinite(batch.node_features);
  if (require_finite_valid_features) {
    const bool invalid_valid_feature =
        raw_mask.logical_and(finite.logical_not()).any().template item<bool>();
    TORCH_CHECK(!invalid_valid_feature,
                "[channel_node_stream_adapter] non-finite value under true "
                "feature mask");
  }
  auto feature_mask = raw_mask.logical_and(finite);
  auto clean_features = torch::where(feature_mask, batch.node_features,
                                     torch::zeros_like(batch.node_features));

  auto data = clean_features.permute({0, 3, 1, 2, 4})
                  .contiguous()
                  .reshape({B * N, C, Hx, Dx});
  auto mask = feature_mask.permute({0, 3, 1, 2, 4})
                  .contiguous()
                  .reshape({B * N, C, Hx, Dx});

  auto index_opts =
      torch::TensorOptions().dtype(torch::kInt64).device(data.device());
  auto anchor_index =
      torch::arange(B, index_opts).unsqueeze(1).repeat({1, N}).reshape({B * N});
  auto node_index =
      torch::arange(N, index_opts).unsqueeze(0).repeat({B, 1}).reshape({B * N});

  channel_node_encoder_input_t out{};
  out.data = std::move(data);
  out.feature_mask = std::move(mask);
  out.cell_mask_any = out.feature_mask.any(/*dim=*/3);
  out.cell_mask_all = out.feature_mask.all(/*dim=*/3);
  out.anchor_index = std::move(anchor_index);
  out.node_index = std::move(node_index);
  out.B_anchor = B;
  out.N = N;
  out.node_ids = batch.node_ids;
  out.anchor_keys = batch.anchor_keys;
  out.diagnostics.original_rows = B * N;
  out.diagnostics.valid_feature_count =
      out.feature_mask.sum().template item<int64_t>();
  out.diagnostics.finite_feature_count = finite.sum().template item<int64_t>();
  out.diagnostics.valid_feature_fraction =
      static_cast<double>(out.diagnostics.valid_feature_count) /
      static_cast<double>(B * N * C * Hx * Dx);
  out.diagnostics.valid_cell_any_count =
      out.cell_mask_any.sum().template item<int64_t>();
  out.diagnostics.valid_cell_all_count =
      out.cell_mask_all.sum().template item<int64_t>();
  return out;
}

[[nodiscard]] inline graph_row_index_t
make_graph_row_index(const channel_node_encoder_input_t &input) {
  graph_row_index_t out{};
  out.anchor_index = input.anchor_index;
  out.node_index = input.node_index;
  out.B_anchor = input.B_anchor;
  out.N = input.N;
  return out;
}

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg
