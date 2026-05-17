// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "ujcamei/source/dataloader/graph_anchor_edge_batch.h"
#include "ujcamei/source/dataloader/graph_anchor_edge_dataset.h"
#include "wikimyei/expression/nodelift/srl/synthetic_reference_lift.h"

namespace cuwacunu::wikimyei::expression::nodelift::srl::stream {

namespace node_lifted_stream_detail {

inline void
validate_target_side_future_lift_shapes(const torch::Tensor &edge_features,
                                        const torch::Tensor &future_features,
                                        const torch::Tensor &future_mask) {
  TORCH_CHECK(edge_features.defined() && edge_features.dim() == 5,
              "[node_lifted_stream] observed edge_features must be "
              "[B,L,C,Hx,9]");
  TORCH_CHECK(edge_features.size(4) ==
                  cuwacunu::wikimyei::expression::nodelift::srl::kFeatureWidth,
              "[node_lifted_stream] observed edge_features must have width 9");
  TORCH_CHECK(future_features.defined(),
              "[node_lifted_stream] lift_future=true requires "
              "future_edge_features when future_edge_mask is defined");
  TORCH_CHECK(future_mask.defined(),
              "[node_lifted_stream] lift_future=true requires "
              "future_edge_mask when future_edge_features is defined");
  TORCH_CHECK(future_features.dim() == 5,
              "[node_lifted_stream] future_edge_features must be "
              "[B,L,C,Hf,9]");
  TORCH_CHECK(future_mask.dim() == 4,
              "[node_lifted_stream] future_edge_mask must be [B,L,C,Hf]");
  TORCH_CHECK(future_features.size(0) == edge_features.size(0) &&
                  future_features.size(1) == edge_features.size(1) &&
                  future_features.size(2) == edge_features.size(2),
              "[node_lifted_stream] future_edge_features must share B,L,C "
              "with observed edge_features");
  TORCH_CHECK(future_mask.size(0) == future_features.size(0) &&
                  future_mask.size(1) == future_features.size(1) &&
                  future_mask.size(2) == future_features.size(2) &&
                  future_mask.size(3) == future_features.size(3),
              "[node_lifted_stream] future_edge_mask shape must match "
              "future_edge_features [B,L,C,Hf]");
  TORCH_CHECK(future_features.size(4) ==
                  cuwacunu::wikimyei::expression::nodelift::srl::kFeatureWidth,
              "[node_lifted_stream] future_edge_features must have width 9");
}

} // namespace node_lifted_stream_detail

template <typename KeyT> struct node_lifted_batch_t {
  torch::Tensor node_features{}; // [B,C,Hx,N,9]
  torch::Tensor node_mask{};     // [B,C,Hx,N,9], bool
  std::optional<torch::Tensor> node_mask_any{};
  std::optional<torch::Tensor> node_mask_all{};

  torch::Tensor price_residual{};      // [B,C,Hx,L,4]
  torch::Tensor price_residual_mask{}; // [B,C,Hx,L,4], bool

  std::optional<torch::Tensor> activity_total{};
  std::optional<torch::Tensor> activity_support{};
  std::optional<torch::Tensor> activity_coverage{};

  torch::Tensor future_edge_features{}; // [B,L,C,Hf,9], undefined if omitted
  torch::Tensor future_edge_mask{};     // [B,L,C,Hf], undefined if omitted

  // Stepwise target-side NodeLift of future edge evidence. These tensors are
  // carried for target/diagnostic use only; they must not feed representation
  // inputs for the same anchor and are not cumulative node trajectories.
  torch::Tensor future_node_features{}; // [B,C,Hf,N,9], undefined if omitted
  torch::Tensor future_node_mask{};     // [B,C,Hf,N,9], bool
  std::optional<torch::Tensor> future_node_mask_any{};
  std::optional<torch::Tensor> future_node_mask_all{};
  torch::Tensor future_price_residual{};      // [B,C,Hf,L,4]
  torch::Tensor future_price_residual_mask{}; // [B,C,Hf,L,4], bool
  std::optional<torch::Tensor> future_activity_total{};
  std::optional<torch::Tensor> future_activity_support{};
  std::optional<torch::Tensor> future_activity_coverage{};

  torch::Tensor anchor_keys{};  // [B]
  torch::Tensor edge_present{}; // [B,L]
  torch::Tensor past_keys{};    // [B,L,C,Hx], undefined if omitted
  torch::Tensor future_keys{};  // [B,L,C,Hf], undefined if omitted

  std::vector<std::string> node_ids{};
  std::vector<std::string> edge_ids{};
  std::string graph_order_fingerprint{};

  cuwacunu::wikimyei::expression::nodelift::srl::nodelift_diagnostics_t
      diagnostics{};
  cuwacunu::wikimyei::expression::nodelift::srl::nodelift_diagnostics_t
      future_diagnostics{};
  cuwacunu::ujcamei::source::dataloader::
      graph_anchor_edge_alignment_diagnostics_t alignment_diagnostics{};
};

template <typename KeyT>
[[nodiscard]] node_lifted_batch_t<KeyT> node_lift_graph_anchor_edge_batch(
    const cuwacunu::ujcamei::source::dataloader::graph_anchor_edge_batch_t<KeyT>
        &graph_batch,
    const cuwacunu::wikimyei::expression::nodelift::srl::graph_t &graph,
    const cuwacunu::wikimyei::expression::nodelift::srl::nodelift_options_t
        &options = {},
    bool compute_alignment_diagnostics = true, bool lift_future = true) {
  TORCH_CHECK(graph_batch.edge_ids == graph.edge_ids,
              "[node_lifted_stream] graph batch edge order does not match SRL "
              "graph edge order");
  const std::string graph_fingerprint =
      graph.graph_order_fingerprint.empty()
          ? graph.computed_graph_order_fingerprint()
          : graph.graph_order_fingerprint;
  if (!graph_batch.graph_order_fingerprint.empty()) {
    TORCH_CHECK(graph_batch.graph_order_fingerprint == graph_fingerprint,
                "[node_lifted_stream] graph batch fingerprint does not match "
                "SRL graph fingerprint");
  }

  cuwacunu::wikimyei::expression::nodelift::srl::nodelift_input_t input{};
  input.edge_features = graph_batch.edge_features;
  input.edge_mask = graph_batch.edge_mask;
  input.edge_coord_mask = std::nullopt;

  const auto lifted =
      cuwacunu::wikimyei::expression::nodelift::srl::featurewise_node_lift(
          graph, input, options);

  node_lifted_batch_t<KeyT> out{};
  out.node_features = lifted.node_features;
  out.node_mask = lifted.node_mask;
  out.node_mask_any = lifted.node_mask_any;
  out.node_mask_all = lifted.node_mask_all;
  out.price_residual = lifted.price_residual;
  out.price_residual_mask = lifted.price_residual_mask;
  out.activity_total = lifted.activity_total;
  out.activity_support = lifted.activity_support;
  out.activity_coverage = lifted.activity_coverage;
  out.future_edge_features = graph_batch.future_features;
  out.future_edge_mask = graph_batch.future_mask;
  if (lift_future && (graph_batch.future_features.defined() ||
                      graph_batch.future_mask.defined())) {
    node_lifted_stream_detail::validate_target_side_future_lift_shapes(
        graph_batch.edge_features, graph_batch.future_features,
        graph_batch.future_mask);
    cuwacunu::wikimyei::expression::nodelift::srl::nodelift_input_t
        future_input{};
    future_input.edge_features = graph_batch.future_features;
    future_input.edge_mask = graph_batch.future_mask;
    future_input.edge_coord_mask = std::nullopt;
    const auto future_lifted =
        cuwacunu::wikimyei::expression::nodelift::srl::featurewise_node_lift(
            graph, future_input, options);
    out.future_node_features = future_lifted.node_features;
    out.future_node_mask = future_lifted.node_mask;
    out.future_node_mask_any = future_lifted.node_mask_any;
    out.future_node_mask_all = future_lifted.node_mask_all;
    out.future_price_residual = future_lifted.price_residual;
    out.future_price_residual_mask = future_lifted.price_residual_mask;
    out.future_activity_total = future_lifted.activity_total;
    out.future_activity_support = future_lifted.activity_support;
    out.future_activity_coverage = future_lifted.activity_coverage;
    out.future_diagnostics = future_lifted.diagnostics;
  }
  out.anchor_keys = graph_batch.anchor_keys;
  out.edge_present = graph_batch.edge_present;
  out.past_keys = graph_batch.past_keys;
  out.future_keys = graph_batch.future_keys;
  out.node_ids = graph.node_ids;
  out.edge_ids = graph.edge_ids;
  out.graph_order_fingerprint = graph_fingerprint;
  out.diagnostics = lifted.diagnostics;
  if (compute_alignment_diagnostics && graph_batch.past_keys.defined()) {
    out.alignment_diagnostics = cuwacunu::ujcamei::source::dataloader::
        compute_graph_anchor_edge_alignment_diagnostics<KeyT>(graph_batch);
  }
  return out;
}

template <typename DatatypeT> struct node_lifted_stream_options_t {
  std::size_t batch_size{64};
  std::optional<
      cuwacunu::ujcamei::source::dataloader::graph_anchor_edge_batch_options_t>
      graph_batch_options{std::nullopt};
  cuwacunu::wikimyei::expression::nodelift::srl::nodelift_options_t
      nodelift_options{};
  bool compute_alignment_diagnostics{true};
  bool lift_future{true};
};

template <typename DatatypeT> class node_lifted_stream_t {
public:
  using key_t = typename DatatypeT::key_type_t;
  using source_t =
      cuwacunu::ujcamei::source::dataloader::graph_anchor_edge_dataset_t<
          DatatypeT>;
  using batch_t = node_lifted_batch_t<key_t>;

  node_lifted_stream_t(
      source_t &&source,
      cuwacunu::wikimyei::expression::nodelift::srl::graph_t graph,
      node_lifted_stream_options_t<DatatypeT> options = {})
      : source_(std::move(source)), graph_(std::move(graph)),
        options_(std::move(options)) {
    TORCH_CHECK(options_.batch_size > 0,
                "[node_lifted_stream_t] batch_size must be positive");
  }

  [[nodiscard]] bool has_next() const { return cursor_ < source_.size(); }

  [[nodiscard]] std::size_t cursor() const { return cursor_; }

  void reset() { cursor_ = 0; }

  [[nodiscard]] const source_t &source() const { return source_; }

  [[nodiscard]] const cuwacunu::wikimyei::expression::nodelift::srl::graph_t &
  graph() const {
    return graph_;
  }

  batch_t next() {
    TORCH_CHECK(has_next(), "[node_lifted_stream_t] stream is exhausted");
    auto graph_batch = source_.get_graph_batch(cursor_, options_.batch_size,
                                               options_.graph_batch_options);
    cursor_ += static_cast<std::size_t>(graph_batch.anchor_keys.size(0));
    return node_lift_graph_anchor_edge_batch<key_t>(
        graph_batch, graph_, options_.nodelift_options,
        options_.compute_alignment_diagnostics, options_.lift_future);
  }

private:
  source_t source_{};
  cuwacunu::wikimyei::expression::nodelift::srl::graph_t graph_{};
  node_lifted_stream_options_t<DatatypeT> options_{};
  std::size_t cursor_{0};
};

} // namespace cuwacunu::wikimyei::expression::nodelift::srl::stream
