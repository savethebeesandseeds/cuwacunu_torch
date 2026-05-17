// SPDX-License-Identifier: MIT
#pragma once

#include <optional>
#include <string>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/expression/nodelift/srl/stream/node_lifted_stream.h"
#include "wikimyei/representation/encoding/vicreg/node_stream_adapter.h"

namespace cuwacunu::wikimyei::representation::encoding::vicreg::stream {

template <typename KeyT> struct node_representation_batch_t {
  torch::Tensor node_encoding{};      // [B_anchor,N,D_e] or [B_anchor,N,Hx,D_e]
  torch::Tensor node_encoding_mask{}; // [B_anchor,N], bool

  torch::Tensor anchor_keys{}; // [B_anchor]
  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  std::string graph_order_fingerprint{};

  torch::Tensor price_residual{};      // [B_anchor,C,Hx,L,4]
  torch::Tensor price_residual_mask{}; // [B_anchor,C,Hx,L,4], bool

  std::optional<torch::Tensor> activity_total{};
  std::optional<torch::Tensor> activity_support{};
  std::optional<torch::Tensor> activity_coverage{};

  torch::Tensor future_edge_features{}; // [B_anchor,L,C,Hf,9]
  torch::Tensor future_edge_mask{};     // [B_anchor,L,C,Hf]

  // Target-side future NodeLift sidecars. These are carried through for
  // downstream target experiments and diagnostics, not used to build
  // `node_encoding`.
  torch::Tensor future_node_features{}; // [B_anchor,C,Hf,N,9]
  torch::Tensor future_node_mask{};     // [B_anchor,C,Hf,N,9], bool
  std::optional<torch::Tensor> future_node_mask_any{};
  std::optional<torch::Tensor> future_node_mask_all{};
  torch::Tensor future_price_residual{};      // [B_anchor,C,Hf,L,4]
  torch::Tensor future_price_residual_mask{}; // [B_anchor,C,Hf,L,4], bool
  std::optional<torch::Tensor> future_activity_total{};
  std::optional<torch::Tensor> future_activity_support{};
  std::optional<torch::Tensor> future_activity_coverage{};

  cuwacunu::wikimyei::representation::encoding::vicreg::
      node_encoder_input_diagnostics_t adapter_diagnostics{};
  cuwacunu::wikimyei::expression::nodelift::srl::nodelift_diagnostics_t
      nodelift_diagnostics{};
  cuwacunu::wikimyei::expression::nodelift::srl::nodelift_diagnostics_t
      future_nodelift_diagnostics{};
  cuwacunu::ujcamei::source::dataloader::
      graph_anchor_edge_alignment_diagnostics_t alignment_diagnostics{};
};

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg::stream
