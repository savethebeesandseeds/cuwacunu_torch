// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/protocol/component_stream.h"
#include "ujcamei/source/registry/types/kline_feature_registry.h"
#include "wikimyei/inference/expected_value/mdn/channel_context_mdn.h"
#include "wikimyei/representation/encoding/vicreg/stream/channel_representation_batch.h"

namespace cuwacunu::wikimyei::inference::expected_value::mdn::stream {

enum class channel_target_mask_policy_t {
  per_target_feature_valid,
};

struct channel_mdn_adapter_options_t {
  channel_target_mask_policy_t target_mask_policy{
      channel_target_mask_policy_t::per_target_feature_valid};
  std::vector<int64_t> target_coords{0, 1, 2, 3, 4, 5, 6, 7, 8};
};

template <typename KeyT> struct channel_mdn_input_batch_t {
  torch::Tensor context{};      // [B,N,C,De]
  torch::Tensor context_mask{}; // [B,N,C], bool
  torch::Tensor future{};       // [B,N,C,Df]
  torch::Tensor future_mask{};  // [B,N,C,Df], bool

  torch::Tensor node_index{};   // [B,N], int64
  torch::Tensor anchor_index{}; // [B,N], int64
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
  std::string target_domain{"channel_node_future"};
  std::string context_mode{"channel_context_strict"};
};

namespace channel_mdn_adapter_detail {

inline void validate_context_rank(const torch::Tensor &node_encoding,
                                  const torch::Tensor &node_encoding_mask) {
  TORCH_CHECK(node_encoding.defined() && node_encoding.dim() == 4,
              "[channel_mdn_adapter] node_encoding must be [B,N,C,De]");
  TORCH_CHECK(node_encoding_mask.defined() && node_encoding_mask.dim() == 3,
              "[channel_mdn_adapter] node_encoding_mask must be [B,N,C]");
  TORCH_CHECK(node_encoding.size(0) == node_encoding_mask.size(0) &&
                  node_encoding.size(1) == node_encoding_mask.size(1) &&
                  node_encoding.size(2) == node_encoding_mask.size(2),
              "[channel_mdn_adapter] context/mask shape mismatch");
}

inline void validate_target_feature_indices(const std::vector<int64_t> &indices,
                                            int64_t feature_width) {
  channel_context_mdn_detail::validate_target_coords(indices, feature_width);
}

} // namespace channel_mdn_adapter_detail

template <typename KeyT>
[[nodiscard]] channel_mdn_input_batch_t<KeyT> make_channel_mdn_input_batch(
    const cuwacunu::wikimyei::representation::encoding::vicreg::stream::
        channel_representation_batch_t<KeyT> &nodes,
    const channel_mdn_adapter_options_t &options = {}) {
  channel_mdn_adapter_detail::validate_context_rank(nodes.node_encoding,
                                                    nodes.node_encoding_mask);
  TORCH_CHECK(nodes.future_node_features.defined(),
              "[channel_mdn_adapter] future_node_features is undefined");
  TORCH_CHECK(nodes.future_node_mask.defined(),
              "[channel_mdn_adapter] future_node_mask is undefined");
  TORCH_CHECK(nodes.future_node_features.dim() == 5,
              "[channel_mdn_adapter] future_node_features must be "
              "[B,C,Hf,N,Dx]");
  TORCH_CHECK(nodes.future_node_mask.sizes() ==
                  nodes.future_node_features.sizes(),
              "[channel_mdn_adapter] future node mask shape mismatch");
  const auto B = nodes.node_encoding.size(0);
  const auto N = nodes.node_encoding.size(1);
  const auto C = nodes.node_encoding.size(2);
  TORCH_CHECK(nodes.future_node_features.size(0) == B &&
                  nodes.future_node_features.size(1) == C &&
                  nodes.future_node_features.size(3) == N,
              "[channel_mdn_adapter] future shape does not match context");
  channel_mdn_adapter_detail::validate_target_feature_indices(
      options.target_coords, nodes.future_node_features.size(4));

  auto base = make_channel_mdn_input(
      nodes.node_encoding, nodes.node_encoding_mask, nodes.future_node_features,
      nodes.future_node_mask, options.target_coords, nodes.anchor_keys,
      nodes.node_ids);

  channel_mdn_input_batch_t<KeyT> out{};
  out.context = std::move(base.context);
  out.context_mask = std::move(base.context_mask);
  out.future = std::move(base.future);
  out.future_mask = std::move(base.future_mask);
  out.node_index = std::move(base.node_index);
  out.anchor_index = std::move(base.anchor_index);
  out.anchor_keys = std::move(base.anchor_keys);
  out.node_ids = std::move(base.node_ids);
  out.edge_ids = nodes.edge_ids;
  out.target_coords = std::move(base.target_coords);
  out.graph_order_fingerprint = nodes.graph_order_fingerprint;
  out.cursor = nodes.cursor;
  out.nodelift_stream_report = nodes.nodelift_stream_report;
  out.representation_stream_report = nodes.stream_report;
  out.nodelift_runtime_lls = nodes.nodelift_runtime_lls;
  out.representation_runtime_lls = nodes.runtime_lls;
  return out;
}

template <typename KeyT>
[[nodiscard]] channel_mdn_input_t
to_channel_mdn_input(const channel_mdn_input_batch_t<KeyT> &batch) {
  channel_mdn_input_t out{};
  out.context = batch.context;
  out.context_mask = batch.context_mask;
  out.future = batch.future;
  out.future_mask = batch.future_mask;
  out.anchor_index = batch.anchor_index;
  out.node_index = batch.node_index;
  out.anchor_keys = batch.anchor_keys;
  out.target_coords = batch.target_coords;
  out.node_ids = batch.node_ids;
  return out;
}

template <typename KeyT>
[[nodiscard]] torch::Tensor
channel_mdn_anchor_slots_for_node(const channel_mdn_input_batch_t<KeyT> &batch,
                                  int64_t node_slot) {
  TORCH_CHECK(batch.node_index.defined(),
              "[channel_mdn_adapter] node_index is undefined");
  TORCH_CHECK(batch.node_index.dim() == 2,
              "[channel_mdn_adapter] node_index must be [B,N]");
  TORCH_CHECK(node_slot >= 0, "[channel_mdn_adapter] node slot is negative");
  if (!batch.node_ids.empty()) {
    TORCH_CHECK(node_slot < static_cast<int64_t>(batch.node_ids.size()),
                "[channel_mdn_adapter] node slot out of node_ids range");
  }
  auto slots = torch::nonzero(batch.node_index == node_slot);
  TORCH_CHECK(slots.numel() > 0,
              "[channel_mdn_adapter] no anchor slots for node slot ",
              node_slot);
  return slots.to(torch::TensorOptions()
                      .dtype(torch::kInt64)
                      .device(batch.node_index.device()));
}

template <typename KeyT>
[[nodiscard]] torch::Tensor channel_mdn_anchor_slots_for_node_id(
    const channel_mdn_input_batch_t<KeyT> &batch, const std::string &node_id) {
  TORCH_CHECK(!batch.node_ids.empty(),
              "[channel_mdn_adapter] node_ids are required for node-id "
              "routing");
  const auto it =
      std::find(batch.node_ids.begin(), batch.node_ids.end(), node_id);
  TORCH_CHECK(it != batch.node_ids.end(),
              "[channel_mdn_adapter] unknown node_id: ", node_id);
  return channel_mdn_anchor_slots_for_node(
      batch, static_cast<int64_t>(std::distance(batch.node_ids.begin(), it)));
}

} // namespace
  // cuwacunu::wikimyei::inference::expected_value::mdn::stream
