// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/lattice/runtime_report/component_runtime_lls.h"
#include "kikijyeba/protocol/component_stream.h"
#include "ujcamei/source/retrieval/dataloader/graph_anchor_edge_batch.h"
#include "ujcamei/source/retrieval/dataloader/graph_anchor_edge_dataset.h"
#include "wikimyei/expression/nodelift/srl/synthetic_reference_lift.h"

namespace cuwacunu::wikimyei::expression::nodelift::srl::stream {

namespace node_lifted_stream_detail {

namespace lls =
    cuwacunu::kikijyeba::lattice::runtime_report;

inline double tensor_mean_or_nan(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return tensor.detach()
      .to(torch::kCPU)
      .to(torch::kFloat64)
      .mean()
      .item<double>();
}

inline double bool_fraction_or_nan(const torch::Tensor &mask) {
  if (!mask.defined() || mask.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return mask.to(torch::kFloat64).mean().to(torch::kCPU).item<double>();
}

inline void append_finite_double(lls::runtime_lls_document_t &document,
                                 std::string key, double value,
                                 std::string domain = "(-inf,+inf)") {
  if (std::isfinite(value)) {
    document.entries.push_back(lls::make_component_runtime_lls_double_entry(
        std::move(key), value, std::move(domain)));
  }
}

inline double elapsed_millis(std::chrono::steady_clock::time_point begin,
                             std::chrono::steady_clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

template <typename KeyT>
[[nodiscard]] std::vector<KeyT>
anchor_keys_from_tensor(const torch::Tensor &anchor_keys) {
  TORCH_CHECK(anchor_keys.defined() && anchor_keys.dim() == 1,
              "[node_lifted_stream] anchor_keys must be [B]");
  auto cpu = anchor_keys.to(torch::kCPU).to(torch::kInt64).contiguous();
  std::vector<KeyT> out;
  out.reserve(static_cast<std::size_t>(cpu.numel()));
  auto accessor = cpu.accessor<int64_t, 1>();
  for (int64_t i = 0; i < cpu.numel(); ++i) {
    out.push_back(static_cast<KeyT>(accessor[i]));
  }
  return out;
}

template <typename KeyT>
[[nodiscard]] cuwacunu::ujcamei::source::retrieval::dataloader::
    graph_anchor_cursor_t<KeyT>
    effective_graph_anchor_cursor(
        const cuwacunu::ujcamei::source::retrieval::dataloader::
            graph_anchor_edge_batch_t<KeyT> &graph_batch,
        const std::string &graph_fingerprint) {
  if (!graph_batch.cursor.empty() &&
      !graph_batch.cursor.graph_order_fingerprint.empty()) {
    return graph_batch.cursor;
  }
  const auto anchors = anchor_keys_from_tensor<KeyT>(graph_batch.anchor_keys);
  return cuwacunu::ujcamei::source::retrieval::dataloader::
      make_graph_anchor_cursor<KeyT>(graph_fingerprint, 0, anchors.size(),
                                     anchors.size(), anchors);
}

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

template <typename KeyT>
[[nodiscard]] std::string make_nodelift_runtime_lls(
    const cuwacunu::ujcamei::source::retrieval::dataloader::
        graph_anchor_edge_batch_t<KeyT> &graph_batch,
    const cuwacunu::ujcamei::source::retrieval::dataloader::
        graph_anchor_cursor_t<KeyT> &batch_cursor,
    const cuwacunu::wikimyei::expression::nodelift::srl::graph_t &graph,
    const cuwacunu::wikimyei::expression::nodelift::srl::nodelift_output_t
        &observed,
    const std::optional<
        cuwacunu::wikimyei::expression::nodelift::srl::nodelift_output_t>
        &future,
    const std::string &graph_fingerprint, bool lift_future, double elapsed_ms,
    const std::string &component_id = "nodelift_srl_v1",
    const std::string &assembly_token = "wikimyei.expression.nodelift.srl.v1",
    const std::string &dock_binding_token = {},
    const cuwacunu::kikijyeba::protocol::component_stream_wave_t &stream_wave =
        {}) {
  auto stream_report =
      cuwacunu::kikijyeba::protocol::make_component_stream_report(
          cuwacunu::kikijyeba::protocol::component_stream_identity_t{
              .component_family = "wikimyei.expression.nodelift.srl",
              .component_id = component_id,
              .assembly_token = assembly_token,
              .dock_binding_token = dock_binding_token,
              .graph_order_fingerprint = graph_fingerprint,
          },
          cuwacunu::kikijyeba::protocol::make_component_stream_cursor(
              stream_wave, batch_cursor),
          cuwacunu::kikijyeba::lattice::runtime_report::
              runtime_report_mode_t::debug,
          elapsed_ms,
          static_cast<std::uint64_t>(graph_batch.edge_features.numel()),
          static_cast<std::uint64_t>(observed.node_features.numel()));
  auto document =
      cuwacunu::kikijyeba::protocol::make_component_stream_runtime_document(
          "wikimyei.expression.nodelift.srl.runtime.v1", stream_report);
  lls::append_graph_anchor_cursor_entries(document, batch_cursor, "batch",
                                          false);
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "anchor_count",
      static_cast<std::uint64_t>(graph_batch.anchor_keys.size(0))));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "node_count", static_cast<std::uint64_t>(graph.node_ids.size())));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "edge_count", static_cast<std::uint64_t>(graph.edge_ids.size())));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "channel_count",
      static_cast<std::uint64_t>(graph_batch.edge_features.size(2))));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "input_length",
      static_cast<std::uint64_t>(graph_batch.edge_features.size(3))));
  const std::uint64_t future_length =
      graph_batch.future_features.defined()
          ? static_cast<std::uint64_t>(graph_batch.future_features.size(3))
          : 0U;
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "future_length", future_length));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "feature_width",
      static_cast<std::uint64_t>(
          cuwacunu::wikimyei::expression::nodelift::srl::kFeatureWidth)));
  document.entries.push_back(
      lls::make_component_runtime_lls_bool_entry("lift_future", lift_future));
  document.entries.push_back(lls::make_component_runtime_lls_bool_entry(
      "future_lift_emitted", future.has_value()));

  append_finite_double(document, "observed_node_mask_fraction",
                       bool_fraction_or_nan(observed.node_mask), "[0,1]");
  append_finite_double(document, "observed_residual_energy_mean",
                       tensor_mean_or_nan(observed.diagnostics.residual_energy),
                       "[0,+inf)");
  append_finite_double(
      document, "observed_valid_edge_count_mean",
      tensor_mean_or_nan(observed.diagnostics.valid_edge_count), "[0,+inf)");
  if (future.has_value()) {
    append_finite_double(document, "future_node_mask_fraction",
                         bool_fraction_or_nan(future->node_mask), "[0,1]");
    append_finite_double(
        document, "future_residual_energy_mean",
        tensor_mean_or_nan(future->diagnostics.residual_energy), "[0,+inf)");
    append_finite_double(
        document, "future_valid_edge_count_mean",
        tensor_mean_or_nan(future->diagnostics.valid_edge_count), "[0,+inf)");
  }
  return lls::emit_component_runtime_lls_canonical(document);
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
  cuwacunu::ujcamei::source::retrieval::dataloader::graph_anchor_cursor_t<KeyT>
      cursor{};
  cuwacunu::kikijyeba::protocol::component_stream_report_t stream_report{};
  std::string runtime_lls{};

  cuwacunu::wikimyei::expression::nodelift::srl::nodelift_diagnostics_t
      diagnostics{};
  cuwacunu::wikimyei::expression::nodelift::srl::nodelift_diagnostics_t
      future_diagnostics{};
  cuwacunu::ujcamei::source::retrieval::dataloader::
      graph_anchor_edge_alignment_diagnostics_t alignment_diagnostics{};
};

template <typename KeyT>
[[nodiscard]] node_lifted_batch_t<KeyT> node_lift_graph_anchor_edge_batch(
    const cuwacunu::ujcamei::source::retrieval::dataloader::
        graph_anchor_edge_batch_t<KeyT> &graph_batch,
    const cuwacunu::wikimyei::expression::nodelift::srl::graph_t &graph,
    const cuwacunu::wikimyei::expression::nodelift::srl::nodelift_options_t
        &options = {},
    bool compute_alignment_diagnostics = true, bool lift_future = true,
    cuwacunu::kikijyeba::lattice::runtime_report::
        runtime_report_mode_t runtime_report_mode =
            cuwacunu::kikijyeba::lattice::runtime_report::
                runtime_report_mode_t::normal,
    std::string component_id = "nodelift_srl_v1",
    std::string assembly_token = "wikimyei.expression.nodelift.srl.v1",
    std::string dock_binding_token = {},
    cuwacunu::kikijyeba::protocol::component_stream_wave_t stream_wave = {}) {
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
  const auto batch_cursor =
      node_lifted_stream_detail::effective_graph_anchor_cursor<KeyT>(
          graph_batch, graph_fingerprint);

  cuwacunu::wikimyei::expression::nodelift::srl::nodelift_input_t input{};
  input.edge_features = graph_batch.edge_features;
  input.edge_mask = graph_batch.edge_mask;
  input.edge_coord_mask = std::nullopt;

  const auto lift_begin = std::chrono::steady_clock::now();
  const auto lifted =
      cuwacunu::wikimyei::expression::nodelift::srl::featurewise_node_lift(
          graph, input, options);
  std::optional<
      cuwacunu::wikimyei::expression::nodelift::srl::nodelift_output_t>
      future_lifted_for_report{std::nullopt};

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
    auto future_lifted =
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
    future_lifted_for_report = std::move(future_lifted);
  }
  const auto lift_end = std::chrono::steady_clock::now();
  out.anchor_keys = graph_batch.anchor_keys;
  out.edge_present = graph_batch.edge_present;
  out.past_keys = graph_batch.past_keys;
  out.future_keys = graph_batch.future_keys;
  out.node_ids = graph.node_ids;
  out.edge_ids = graph.edge_ids;
  out.graph_order_fingerprint = graph_fingerprint;
  out.cursor = batch_cursor;
  out.diagnostics = lifted.diagnostics;
  if (compute_alignment_diagnostics && graph_batch.past_keys.defined()) {
    out.alignment_diagnostics =
        cuwacunu::ujcamei::source::retrieval::dataloader::
            compute_graph_anchor_edge_alignment_diagnostics<KeyT>(graph_batch);
  }
  if (cuwacunu::kikijyeba::lattice::runtime_report::
          runtime_report_enabled(runtime_report_mode)) {
    out.runtime_lls = node_lifted_stream_detail::make_nodelift_runtime_lls(
        graph_batch, batch_cursor, graph, lifted, future_lifted_for_report,
        graph_fingerprint, lift_future,
        node_lifted_stream_detail::elapsed_millis(lift_begin, lift_end),
        component_id, assembly_token, dock_binding_token, stream_wave);
  }
  out.stream_report =
      cuwacunu::kikijyeba::protocol::make_component_stream_report(
          cuwacunu::kikijyeba::protocol::component_stream_identity_t{
              .component_family = "wikimyei.expression.nodelift.srl",
              .component_id = std::move(component_id),
              .assembly_token = std::move(assembly_token),
              .dock_binding_token = std::move(dock_binding_token),
              .graph_order_fingerprint = graph_fingerprint,
          },
          cuwacunu::kikijyeba::protocol::make_component_stream_cursor(
              stream_wave, batch_cursor),
          runtime_report_mode,
          node_lifted_stream_detail::elapsed_millis(lift_begin, lift_end),
          static_cast<std::uint64_t>(graph_batch.edge_features.numel()),
          static_cast<std::uint64_t>(out.node_features.numel()));
  out.stream_report.runtime_lls = out.runtime_lls;
  return out;
}

template <typename DatatypeT> struct node_lifted_stream_options_t {
  std::size_t batch_size{64};
  std::optional<cuwacunu::ujcamei::source::retrieval::dataloader::
                    graph_anchor_edge_batch_options_t>
      graph_batch_options{std::nullopt};
  cuwacunu::wikimyei::expression::nodelift::srl::nodelift_options_t
      nodelift_options{};
  bool compute_alignment_diagnostics{true};
  bool lift_future{true};
  std::size_t begin_anchor_index{0};
  std::optional<std::size_t> end_anchor_index{std::nullopt};
  std::string component_id{"nodelift_srl_v1"};
  std::string assembly_token{"wikimyei.expression.nodelift.srl.v1"};
  std::string dock_binding_token{};
  cuwacunu::kikijyeba::protocol::component_stream_wave_t stream_wave{};
  cuwacunu::kikijyeba::lattice::runtime_report::
      runtime_report_mode_t runtime_report_mode{
          cuwacunu::kikijyeba::lattice::runtime_report::
              runtime_report_mode_t::normal};
};

template <typename DatatypeT> class node_lifted_stream_t {
public:
  using key_t = typename DatatypeT::key_type_t;
  using source_t = cuwacunu::ujcamei::source::retrieval::dataloader::
      graph_anchor_edge_dataset_t<DatatypeT>;
  using batch_t = node_lifted_batch_t<key_t>;

  node_lifted_stream_t(
      source_t &&source,
      cuwacunu::wikimyei::expression::nodelift::srl::graph_t graph,
      node_lifted_stream_options_t<DatatypeT> options = {})
      : source_(std::move(source)), graph_(std::move(graph)),
        options_(std::move(options)), cursor_(options_.begin_anchor_index),
        stream_end_anchor_index_(
            options_.end_anchor_index.value_or(source_.size())) {
    TORCH_CHECK(options_.batch_size > 0,
                "[node_lifted_stream_t] batch_size must be positive");
    TORCH_CHECK(options_.begin_anchor_index <= source_.size(),
                "[node_lifted_stream_t] begin_anchor_index is outside source "
                "anchor domain");
    TORCH_CHECK(stream_end_anchor_index_ <= source_.size(),
                "[node_lifted_stream_t] end_anchor_index is outside source "
                "anchor domain");
    TORCH_CHECK(stream_end_anchor_index_ >= options_.begin_anchor_index,
                "[node_lifted_stream_t] end_anchor_index must be >= "
                "begin_anchor_index");
  }

  [[nodiscard]] bool has_next() const {
    return cursor_ < stream_end_anchor_index_;
  }

  [[nodiscard]] std::size_t cursor() const { return cursor_; }

  [[nodiscard]] std::size_t begin_anchor_index() const {
    return options_.begin_anchor_index;
  }

  [[nodiscard]] std::size_t end_anchor_index() const {
    return stream_end_anchor_index_;
  }

  void reset() { cursor_ = options_.begin_anchor_index; }

  [[nodiscard]] const source_t &source() const { return source_; }

  [[nodiscard]] const cuwacunu::wikimyei::expression::nodelift::srl::graph_t &
  graph() const {
    return graph_;
  }

  batch_t next() {
    TORCH_CHECK(has_next(), "[node_lifted_stream_t] stream is exhausted");
    const auto remaining = stream_end_anchor_index_ - cursor_;
    const auto batch_size = std::min(options_.batch_size, remaining);
    auto graph_batch = source_.get_graph_batch(cursor_, batch_size,
                                               options_.graph_batch_options);
    cursor_ += static_cast<std::size_t>(graph_batch.anchor_keys.size(0));
    return node_lift_graph_anchor_edge_batch<key_t>(
        graph_batch, graph_, options_.nodelift_options,
        options_.compute_alignment_diagnostics, options_.lift_future,
        options_.runtime_report_mode, options_.component_id,
        options_.assembly_token, options_.dock_binding_token,
        options_.stream_wave);
  }

private:
  source_t source_{};
  cuwacunu::wikimyei::expression::nodelift::srl::graph_t graph_{};
  node_lifted_stream_options_t<DatatypeT> options_{};
  std::size_t cursor_{0};
  std::size_t stream_end_anchor_index_{0};
};

} // namespace cuwacunu::wikimyei::expression::nodelift::srl::stream
