// SPDX-License-Identifier: MIT
#pragma once

#include <optional>
#include <string>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/protocol/component_stream.h"
#include "wikimyei/expression/nodelift/srl/stream/node_lifted_stream.h"
#include "wikimyei/representation/encoding/vicreg/channel_node_stream_adapter.h"

namespace cuwacunu::wikimyei::representation::encoding::vicreg::stream {

template <typename KeyT> struct channel_representation_batch_t {
  torch::Tensor node_encoding{};       // [B_anchor,N,C,De]
  torch::Tensor node_encoding_mask{};  // [B_anchor,N,C], bool
  torch::Tensor reducer_weights{};     // [B_anchor,N,C,Hx]
  torch::Tensor anchor_keys{};         // [B_anchor]
  std::vector<std::string> node_ids{}; // [N]
  std::vector<std::string> edge_ids{};
  std::string graph_order_fingerprint{};
  cuwacunu::ujcamei::source::retrieval::dataloader::graph_anchor_cursor_t<KeyT>
      cursor{};
  cuwacunu::kikijyeba::protocol::component_stream_report_t
      nodelift_stream_report{};
  cuwacunu::kikijyeba::protocol::component_stream_report_t stream_report{};
  std::string nodelift_runtime_lls{};
  std::string runtime_lls{};

  torch::Tensor price_residual{};      // [B_anchor,C,Hx,L,4]
  torch::Tensor price_residual_mask{}; // [B_anchor,C,Hx,L,4], bool

  std::optional<torch::Tensor> activity_total{};
  std::optional<torch::Tensor> activity_support{};
  std::optional<torch::Tensor> activity_coverage{};

  torch::Tensor future_edge_features{}; // [B_anchor,L,C,Hf,9]
  torch::Tensor future_edge_mask{};     // [B_anchor,L,C,Hf]
  torch::Tensor future_keys{};          // [B_anchor,L,C,Hf]

  torch::Tensor future_node_features{}; // [B_anchor,C,Hf,N,9]
  torch::Tensor future_node_mask{};     // [B_anchor,C,Hf,N,9], bool
  std::optional<torch::Tensor> future_node_mask_any{};
  std::optional<torch::Tensor> future_node_mask_all{};
  torch::Tensor future_price_residual{};      // [B_anchor,C,Hf,L,4]
  torch::Tensor future_price_residual_mask{}; // [B_anchor,C,Hf,L,4], bool
  std::optional<torch::Tensor> future_activity_total{};
  std::optional<torch::Tensor> future_activity_support{};
  std::optional<torch::Tensor> future_activity_coverage{};

  channel_node_encoder_input_diagnostics_t adapter_diagnostics{};
  cuwacunu::wikimyei::expression::nodelift::srl::nodelift_diagnostics_t
      nodelift_diagnostics{};
  cuwacunu::wikimyei::expression::nodelift::srl::nodelift_diagnostics_t
      future_nodelift_diagnostics{};
  cuwacunu::ujcamei::source::retrieval::dataloader::
      graph_anchor_edge_alignment_diagnostics_t alignment_diagnostics{};
};

} // namespace
  // cuwacunu::wikimyei::representation::encoding::vicreg::stream
