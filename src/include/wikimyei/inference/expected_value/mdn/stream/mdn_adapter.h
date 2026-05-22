// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/protocol/component_stream.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "wikimyei/representation/encoding/vicreg/stream/node_representation_batch.h"

namespace cuwacunu::wikimyei::inference::expected_value::mdn::stream {

enum class context_reduction_policy_t {
  last,
  mean,
};

enum class target_mask_policy_t {
  all_target_features_valid,
};

struct mdn_adapter_options_t {
  context_reduction_policy_t context_reduction{
      context_reduction_policy_t::last};
  target_mask_policy_t target_mask_policy{
      target_mask_policy_t::all_target_features_valid};
  std::vector<int64_t> target_coords{0, 1, 2, 3, 4, 5, 6, 7, 8};
};

template <typename KeyT> struct mdn_input_batch_t {
  torch::Tensor context{};      // [B_node,D_context]
  torch::Tensor context_mask{}; // [B_node], bool

  torch::Tensor future{};      // [B_node,C,Hf,Df]
  torch::Tensor future_mask{}; // [B_node,C,Hf], bool

  torch::Tensor node_index{};   // [B_node], int64
  torch::Tensor anchor_index{}; // [B_node], int64
  torch::Tensor anchor_keys{};  // [B_anchor]

  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  std::vector<int64_t> target_coords{};
  std::string graph_order_fingerprint{};
  cuwacunu::ujcamei::source::retrieval::dataloader::graph_anchor_cursor_t<KeyT>
      cursor{};
  cuwacunu::kikijyeba::protocol::component_stream_report_t
      nodelift_stream_report{};
  cuwacunu::kikijyeba::protocol::component_stream_report_t
      representation_stream_report{};
  std::string nodelift_runtime_lls{};
  std::string representation_runtime_lls{};
  std::string target_domain{"node_future"};
  std::string activity_target_semantics{"node_feature_support_mean"};
};

namespace mdn_adapter_detail {

inline void validate_target_feature_indices(const std::vector<int64_t> &indices,
                                            int64_t feature_width) {
  TORCH_CHECK(!indices.empty(),
              "[mdn_adapter] target feature indices are empty");
  std::unordered_set<int64_t> seen;
  seen.reserve(indices.size());
  for (const int64_t index : indices) {
    TORCH_CHECK(index >= 0 && index < feature_width,
                "[mdn_adapter] target feature index out of range: ", index,
                " for future width ", feature_width);
    TORCH_CHECK(seen.insert(index).second,
                "[mdn_adapter] duplicate target feature index: ", index);
  }
}

inline torch::Tensor reduce_node_context(const torch::Tensor &node_encoding,
                                         context_reduction_policy_t policy) {
  TORCH_CHECK(node_encoding.defined(),
              "[mdn_adapter] node_encoding is undefined");
  TORCH_CHECK(node_encoding.dim() == 3 || node_encoding.dim() == 4,
              "[mdn_adapter] node_encoding must be [B,N,D] or "
              "[B,N,H,D]");
  if (node_encoding.dim() == 3) {
    return node_encoding;
  }
  switch (policy) {
  case context_reduction_policy_t::last:
    return node_encoding.select(/*dim=*/2, node_encoding.size(2) - 1);
  case context_reduction_policy_t::mean:
    return node_encoding.mean(/*dim=*/2);
  }
  TORCH_CHECK(false, "[mdn_adapter] unknown context reduction policy");
}

} // namespace mdn_adapter_detail

template <typename KeyT>
[[nodiscard]] mdn_input_batch_t<KeyT> make_mdn_input_batch(
    const cuwacunu::wikimyei::representation::encoding::vicreg::stream::
        node_representation_batch_t<KeyT> &nodes,
    const mdn_adapter_options_t &options = {}) {
  auto context = mdn_adapter_detail::reduce_node_context(
      nodes.node_encoding, options.context_reduction);
  TORCH_CHECK(context.dim() == 3,
              "[mdn_adapter] reduced context must be [B,N,D]");
  TORCH_CHECK(nodes.node_encoding_mask.defined(),
              "[mdn_adapter] node_encoding_mask is undefined");
  TORCH_CHECK(nodes.node_encoding_mask.dim() == 2,
              "[mdn_adapter] node_encoding_mask must be [B,N]");
  TORCH_CHECK(nodes.future_node_features.defined(),
              "[mdn_adapter] future_node_features is undefined");
  TORCH_CHECK(nodes.future_node_mask.defined(),
              "[mdn_adapter] future_node_mask is undefined");
  TORCH_CHECK(nodes.future_node_features.dim() == 5,
              "[mdn_adapter] future_node_features must be "
              "[B,C,Hf,N,D]");
  TORCH_CHECK(nodes.future_node_mask.dim() == 5,
              "[mdn_adapter] future_node_mask must be "
              "[B,C,Hf,N,D]");

  const int64_t B = context.size(0);
  const int64_t N = context.size(1);
  const int64_t D_context = context.size(2);
  TORCH_CHECK(B > 0 && N > 0 && D_context > 0,
              "[mdn_adapter] context dimensions must be positive");
  TORCH_CHECK(nodes.node_encoding_mask.sizes() == torch::IntArrayRef({B, N}),
              "[mdn_adapter] node_encoding_mask shape mismatch");
  TORCH_CHECK(static_cast<int64_t>(nodes.node_ids.size()) == N,
              "[mdn_adapter] node_ids size must match N");

  const auto future_width = nodes.future_node_features.size(4);
  mdn_adapter_detail::validate_target_feature_indices(options.target_coords,
                                                      future_width);
  TORCH_CHECK(
      future_width ==
          cuwacunu::ujcamei::source::registry::types::kKlineFeatureWidth,
      "[mdn_adapter] MDN v1 requires future width 9");
  TORCH_CHECK(nodes.future_node_features.size(0) == B &&
                  nodes.future_node_features.size(3) == N,
              "[mdn_adapter] future node feature shape mismatch");
  TORCH_CHECK(nodes.future_node_mask.sizes() ==
                  nodes.future_node_features.sizes(),
              "[mdn_adapter] future node mask shape mismatch");

  const auto device = context.device();
  auto future_features =
      nodes.future_node_features.to(context.options()).contiguous();
  auto future_mask = nodes.future_node_mask.to(torch::kBool).to(device);

  auto target_index =
      torch::tensor(options.target_coords,
                    torch::TensorOptions().dtype(torch::kInt64).device(device));

  // [B,C,Hf,N,9] -> [B,N,C,Hf,9] -> select Df -> [B*N,C,Hf,Df]
  auto future_bn = future_features.permute({0, 3, 1, 2, 4}).contiguous();
  auto selected_future =
      future_bn.index_select(/*dim=*/4, target_index).contiguous();

  // [B,C,Hf,N,9] -> [B,N,C,Hf,9] -> select Df -> all Df valid.
  auto mask_bn = future_mask.permute({0, 3, 1, 2, 4}).contiguous();
  auto selected_mask = mask_bn.index_select(/*dim=*/4, target_index);
  auto reduced_future_mask = selected_mask.all(/*dim=*/4).contiguous();

  const auto index_opts =
      torch::TensorOptions().dtype(torch::kInt64).device(device);
  auto anchor_index =
      torch::arange(B, index_opts).unsqueeze(1).repeat({1, N}).reshape({B * N});
  auto node_index =
      torch::arange(N, index_opts).unsqueeze(0).repeat({B, 1}).reshape({B * N});

  mdn_input_batch_t<KeyT> out{};
  out.context = context.contiguous().reshape({B * N, D_context});
  out.context_mask =
      nodes.node_encoding_mask.to(torch::kBool).to(device).reshape({B * N});
  out.future = selected_future.reshape({B * N, selected_future.size(2),
                                        selected_future.size(3),
                                        selected_future.size(4)});
  out.future_mask = reduced_future_mask.reshape(
      {B * N, reduced_future_mask.size(2), reduced_future_mask.size(3)});
  out.node_index = std::move(node_index);
  out.anchor_index = std::move(anchor_index);
  out.anchor_keys = nodes.anchor_keys;
  out.node_ids = nodes.node_ids;
  out.edge_ids = nodes.edge_ids;
  out.target_coords = options.target_coords;
  out.graph_order_fingerprint = nodes.graph_order_fingerprint;
  out.cursor = nodes.cursor;
  out.nodelift_stream_report = nodes.nodelift_stream_report;
  out.representation_stream_report = nodes.stream_report;
  out.nodelift_runtime_lls = nodes.nodelift_runtime_lls;
  out.representation_runtime_lls = nodes.runtime_lls;
  return out;
}

[[nodiscard]] inline torch::Tensor
combine_mdn_context_and_future_mask(const torch::Tensor &context_mask,
                                    const torch::Tensor &future_mask) {
  TORCH_CHECK(context_mask.defined(),
              "[mdn_adapter] context_mask is undefined");
  TORCH_CHECK(future_mask.defined(), "[mdn_adapter] future_mask is undefined");
  TORCH_CHECK(context_mask.dim() == 1,
              "[mdn_adapter] context_mask must be [B_node]");
  TORCH_CHECK(future_mask.dim() == 3,
              "[mdn_adapter] future_mask must be [B_node,C,Hf]");
  TORCH_CHECK(context_mask.size(0) == future_mask.size(0),
              "[mdn_adapter] context/future batch mismatch");
  return future_mask.to(torch::kBool)
      .logical_and(
          context_mask.to(torch::kBool).view({context_mask.size(0), 1, 1}));
}

template <typename KeyT>
[[nodiscard]] torch::Tensor
mdn_rows_for_node(const mdn_input_batch_t<KeyT> &batch, int64_t node_slot) {
  TORCH_CHECK(batch.node_index.defined(),
              "[mdn_adapter] node_index is undefined");
  TORCH_CHECK(batch.node_index.dim() == 1,
              "[mdn_adapter] node_index must be [B_node]");
  TORCH_CHECK(node_slot >= 0, "[mdn_adapter] node slot is negative");
  if (!batch.node_ids.empty()) {
    TORCH_CHECK(node_slot < static_cast<int64_t>(batch.node_ids.size()),
                "[mdn_adapter] node slot out of node_ids range");
  }
  auto rows = torch::nonzero(batch.node_index == node_slot).reshape({-1});
  TORCH_CHECK(rows.numel() > 0, "[mdn_adapter] no rows found for node slot ",
              node_slot);
  return rows.to(torch::TensorOptions()
                     .dtype(torch::kInt64)
                     .device(batch.node_index.device()));
}

template <typename KeyT>
[[nodiscard]] torch::Tensor
mdn_rows_for_node_id(const mdn_input_batch_t<KeyT> &batch,
                     const std::string &node_id) {
  TORCH_CHECK(!batch.node_ids.empty(),
              "[mdn_adapter] node_ids are required for node-id "
              "routing");
  const auto it =
      std::find(batch.node_ids.begin(), batch.node_ids.end(), node_id);
  TORCH_CHECK(it != batch.node_ids.end(),
              "[mdn_adapter] unknown node_id: ", node_id);
  return mdn_rows_for_node(
      batch, static_cast<int64_t>(std::distance(batch.node_ids.begin(), it)));
}

} // namespace cuwacunu::wikimyei::inference::expected_value::mdn::stream
