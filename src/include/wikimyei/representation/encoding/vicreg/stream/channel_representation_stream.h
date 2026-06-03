// SPDX-License-Identifier: MIT
#pragma once

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <torch/torch.h>

#include "kikijyeba/lattice/runtime_report/component_runtime_lls.h"
#include "kikijyeba/protocol/component_stream.h"
#include "wikimyei/expression/nodelift/srl/stream/node_lifted_stream.h"
#include "wikimyei/representation/encoding/vicreg/channel_node_stream_adapter.h"
#include "wikimyei/representation/encoding/vicreg/channel_preserving_encoder.h"
#include "wikimyei/representation/encoding/vicreg/channel_representation_adapter.h"
#include "wikimyei/representation/encoding/vicreg/stream/channel_representation_batch.h"

namespace cuwacunu::wikimyei::representation::encoding::vicreg::stream {

namespace channel_representation_stream_detail {

namespace lls = cuwacunu::kikijyeba::lattice::runtime_report;

inline double elapsed_millis(std::chrono::steady_clock::time_point begin,
                             std::chrono::steady_clock::time_point end) {
  return std::chrono::duration<double, std::milli>(end - begin).count();
}

inline double bool_fraction_or_nan(const torch::Tensor &mask) {
  if (!mask.defined() || mask.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return mask.to(torch::kFloat64).mean().to(torch::kCPU).item<double>();
}

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

inline double reducer_weight_entropy_or_nan(const torch::Tensor &weights,
                                            const torch::Tensor &mask) {
  if (!weights.defined() || !mask.defined() || weights.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  auto mask_bool = mask.to(
      torch::TensorOptions().dtype(torch::kBool).device(weights.device()));
  auto w = weights.masked_fill(mask_bool.logical_not(), 0.0);
  auto entropy = -(w * w.clamp_min(1e-12).log()).sum(/*dim=*/-1);
  auto valid = mask_bool.any(/*dim=*/-1);
  if (valid.sum().item<int64_t>() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return entropy.masked_select(valid).mean().to(torch::kCPU).item<double>();
}

inline double
reducer_last_valid_weight_mean_or_nan(const torch::Tensor &weights,
                                      const torch::Tensor &mask) {
  if (!weights.defined() || !mask.defined() || weights.numel() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  auto mask_bool = mask.to(
      torch::TensorOptions().dtype(torch::kBool).device(weights.device()));
  if (mask_bool.sizes() != weights.sizes()) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  const auto history_length = weights.size(-1);
  std::vector<int64_t> index_shape(static_cast<std::size_t>(weights.dim()), 1);
  index_shape.back() = history_length;
  auto index_grid =
      torch::arange(
          history_length,
          torch::TensorOptions().dtype(torch::kInt64).device(weights.device()))
          .view(index_shape)
          .expand_as(mask_bool);
  auto last_plus_one = std::get<0>(
      (mask_bool.to(torch::kInt64) * (index_grid + 1)).max(/*dim=*/-1));
  auto valid = last_plus_one > 0;
  if (valid.sum().item<int64_t>() == 0) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  auto last_index = (last_plus_one - 1).clamp_min(0);
  auto last_valid_weight =
      weights.gather(/*dim=*/-1, last_index.unsqueeze(-1)).squeeze(-1);
  return last_valid_weight.masked_select(valid)
      .mean()
      .to(torch::kCPU)
      .item<double>();
}

inline void append_finite_double(lls::runtime_lls_document_t &document,
                                 std::string key, double value,
                                 std::string domain = "(-inf,+inf)") {
  if (std::isfinite(value)) {
    document.entries.push_back(lls::make_component_runtime_lls_double_entry(
        std::move(key), value, std::move(domain)));
  }
}

template <typename EncoderT>
inline void move_encoder_input_tensors_for_encoder(EncoderT &encoder,
                                                   torch::Tensor &data,
                                                   torch::Tensor &mask) {
  if constexpr (requires(EncoderT e) { e->options(); }) {
    data = data.to(torch::TensorOptions()
                       .dtype(encoder->options().dtype)
                       .device(encoder->options().device));
    mask = mask.to(torch::TensorOptions()
                       .dtype(torch::kBool)
                       .device(encoder->options().device));
  } else if constexpr (requires(EncoderT e) { e.options(); }) {
    data = data.to(torch::TensorOptions()
                       .dtype(encoder.options().dtype)
                       .device(encoder.options().device));
    mask = mask.to(torch::TensorOptions()
                       .dtype(torch::kBool)
                       .device(encoder.options().device));
  }
}

template <typename EncoderT>
[[nodiscard]] channel_preserving_encoder_output_t
encode_channel_rows(EncoderT &encoder, const torch::Tensor &data,
                    const torch::Tensor &mask, bool detach_to_cpu) {
  if constexpr (requires(EncoderT e, torch::Tensor d, torch::Tensor m, bool c) {
                  e.encode(d, m, c);
                }) {
    return encoder.encode(data, mask, detach_to_cpu);
  } else if constexpr (requires(EncoderT e, torch::Tensor d, torch::Tensor m) {
                         e->forward(d, m);
                       }) {
    auto out = encoder->forward(data, mask);
    if (detach_to_cpu) {
      out.sequence = out.sequence.detach().to(torch::kCPU);
      out.sequence_mask = out.sequence_mask.detach().to(torch::kCPU);
      out.reduced = out.reduced.detach().to(torch::kCPU);
      out.reduced_mask = out.reduced_mask.detach().to(torch::kCPU);
      out.reducer_weights = out.reducer_weights.detach().to(torch::kCPU);
    }
    return out;
  } else if constexpr (requires(EncoderT e, torch::Tensor d, torch::Tensor m) {
                         e.forward(d, m);
                       }) {
    auto out = encoder.forward(data, mask);
    if (detach_to_cpu) {
      out.sequence = out.sequence.detach().to(torch::kCPU);
      out.sequence_mask = out.sequence_mask.detach().to(torch::kCPU);
      out.reduced = out.reduced.detach().to(torch::kCPU);
      out.reduced_mask = out.reduced_mask.detach().to(torch::kCPU);
      out.reducer_weights = out.reducer_weights.detach().to(torch::kCPU);
    }
    return out;
  } else {
    static_assert(sizeof(EncoderT) == 0,
                  "channel_representation_stream requires encode(data, mask, "
                  "detach_to_cpu) or forward(data, mask)");
  }
}

template <typename KeyT>
[[nodiscard]] std::string make_channel_representation_runtime_lls(
    const cuwacunu::wikimyei::expression::nodelift::srl::stream::
        node_lifted_batch_t<KeyT> &lifted,
    const channel_node_encoder_input_t &input,
    const torch::Tensor &node_encoding, const torch::Tensor &node_encoding_mask,
    const torch::Tensor &reducer_weights, bool detach_to_cpu, double elapsed_ms,
    const std::string &component_assembly_id, const std::string &assembly_token,
    const std::string &dock_binding_token,
    const std::string &component_family_id,
    const std::string &runtime_document_schema_id,
    const cuwacunu::kikijyeba::protocol::component_stream_wave_t &stream_wave,
    cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t
        runtime_report_mode) {
  auto stream_report =
      cuwacunu::kikijyeba::protocol::make_component_stream_report(
          cuwacunu::kikijyeba::protocol::component_stream_identity_t{
              .component_family_id = component_family_id,
              .component_assembly_id = component_assembly_id,
              .assembly_token = assembly_token,
              .dock_binding_token = dock_binding_token,
              .graph_order_fingerprint = lifted.graph_order_fingerprint,
          },
          cuwacunu::kikijyeba::protocol::make_component_stream_cursor(
              stream_wave, lifted.cursor),
          runtime_report_mode, elapsed_ms,
          static_cast<std::uint64_t>(input.data.numel()),
          static_cast<std::uint64_t>(node_encoding.numel()));
  auto document =
      cuwacunu::kikijyeba::protocol::make_component_stream_runtime_document(
          runtime_document_schema_id, stream_report);
  lls::append_graph_anchor_cursor_entries(document, lifted.cursor, "batch",
                                          false);
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "anchor_count", static_cast<std::uint64_t>(input.B_anchor)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "node_count", static_cast<std::uint64_t>(input.N)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "encoded_rank", static_cast<std::uint64_t>(node_encoding.dim())));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "channel_count", static_cast<std::uint64_t>(node_encoding.size(2))));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "encoding_dim", static_cast<std::uint64_t>(node_encoding.size(3))));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "valid_cell_any_count",
      static_cast<std::uint64_t>(input.diagnostics.valid_cell_any_count)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "valid_cell_all_count",
      static_cast<std::uint64_t>(input.diagnostics.valid_cell_all_count)));
  document.entries.push_back(lls::make_component_runtime_lls_bool_entry(
      "detach_to_cpu", detach_to_cpu));
  append_finite_double(document, "valid_feature_fraction",
                       input.diagnostics.valid_feature_fraction, "[0,1]");
  append_finite_double(document, "node_encoding_mask_fraction",
                       bool_fraction_or_nan(node_encoding_mask), "[0,1]");
  append_finite_double(document, "per_channel_mask_fraction",
                       bool_fraction_or_nan(node_encoding_mask), "[0,1]");
  append_finite_double(document, "node_encoding_mean",
                       tensor_mean_or_nan(node_encoding));
  const auto graph_cell_mask =
      reshape_channel_mask(input.cell_mask_all, make_graph_row_index(input));
  append_finite_double(
      document, "reducer_weight_entropy",
      reducer_weight_entropy_or_nan(reducer_weights, graph_cell_mask));
  append_finite_double(
      document, "reducer_last_valid_weight_mean",
      reducer_last_valid_weight_mean_or_nan(reducer_weights, graph_cell_mask),
      "[0,1]");
  return lls::emit_component_runtime_lls_canonical(document);
}

} // namespace channel_representation_stream_detail

template <typename EncoderT, typename KeyT>
[[nodiscard]] channel_representation_batch_t<KeyT>
make_channel_representation_stream_batch(
    EncoderT &encoder,
    const cuwacunu::wikimyei::expression::nodelift::srl::stream::
        node_lifted_batch_t<KeyT> &lifted,
    bool require_finite_valid_features = true, bool detach_to_cpu = false,
    cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t
        runtime_report_mode = cuwacunu::kikijyeba::lattice::runtime_report::
            runtime_report_mode_t::normal,
    std::string component_assembly_id = "vicreg_v1",
    std::string assembly_token = "wikimyei.representation.vicreg.v1",
    std::string dock_binding_token = {},
    std::string component_family_id = "wikimyei.representation.encoding.vicreg",
    std::string runtime_document_schema_id =
        "wikimyei.representation.vicreg.runtime.v1",
    cuwacunu::kikijyeba::protocol::component_stream_wave_t stream_wave = {}) {
  auto input =
      make_channel_node_encoder_input(lifted, require_finite_valid_features);
  channel_representation_stream_detail::move_encoder_input_tensors_for_encoder(
      encoder, input.data, input.feature_mask);
  const auto begin = std::chrono::steady_clock::now();
  auto encoded = channel_representation_stream_detail::encode_channel_rows(
      encoder, input.data, input.feature_mask, detach_to_cpu);
  auto adapted = cuwacunu::wikimyei::representation::encoding::vicreg::
      make_channel_representation_batch(
          encoded.reduced, encoded.reduced_mask, make_graph_row_index(input),
          input.anchor_keys, input.node_ids, encoded.reducer_weights);
  const auto end = std::chrono::steady_clock::now();

  channel_representation_batch_t<KeyT> out{};
  out.node_encoding = std::move(adapted.node_encoding);
  out.node_encoding_mask = std::move(adapted.node_encoding_mask);
  out.reducer_weights = std::move(adapted.reducer_weights);
  out.anchor_keys = lifted.anchor_keys;
  out.node_ids = lifted.node_ids;
  out.edge_ids = lifted.edge_ids;
  out.graph_order_fingerprint = lifted.graph_order_fingerprint;
  out.cursor = lifted.cursor;
  out.nodelift_stream_report = lifted.stream_report;
  out.nodelift_runtime_lls = lifted.runtime_lls;
  out.price_residual = lifted.price_residual;
  out.price_residual_mask = lifted.price_residual_mask;
  out.activity_total = lifted.activity_total;
  out.activity_support = lifted.activity_support;
  out.activity_coverage = lifted.activity_coverage;
  out.future_edge_features = lifted.future_edge_features;
  out.future_edge_mask = lifted.future_edge_mask;
  out.future_keys = lifted.future_keys;
  out.future_node_features = lifted.future_node_features;
  out.future_node_mask = lifted.future_node_mask;
  out.future_node_mask_any = lifted.future_node_mask_any;
  out.future_node_mask_all = lifted.future_node_mask_all;
  out.future_price_residual = lifted.future_price_residual;
  out.future_price_residual_mask = lifted.future_price_residual_mask;
  out.future_activity_total = lifted.future_activity_total;
  out.future_activity_support = lifted.future_activity_support;
  out.future_activity_coverage = lifted.future_activity_coverage;
  out.adapter_diagnostics = input.diagnostics;
  out.nodelift_diagnostics = lifted.diagnostics;
  out.future_nodelift_diagnostics = lifted.future_diagnostics;
  out.alignment_diagnostics = lifted.alignment_diagnostics;

  const auto elapsed_ms =
      channel_representation_stream_detail::elapsed_millis(begin, end);
  if (cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_enabled(
          runtime_report_mode)) {
    out.runtime_lls = channel_representation_stream_detail::
        make_channel_representation_runtime_lls(
            lifted, input, out.node_encoding, out.node_encoding_mask,
            out.reducer_weights, detach_to_cpu, elapsed_ms,
            component_assembly_id, assembly_token, dock_binding_token,
            component_family_id, runtime_document_schema_id, stream_wave,
            runtime_report_mode);
  }
  out.stream_report =
      cuwacunu::kikijyeba::protocol::make_component_stream_report(
          cuwacunu::kikijyeba::protocol::component_stream_identity_t{
              .component_family_id = std::move(component_family_id),
              .component_assembly_id = std::move(component_assembly_id),
              .assembly_token = std::move(assembly_token),
              .dock_binding_token = std::move(dock_binding_token),
              .graph_order_fingerprint = out.graph_order_fingerprint,
          },
          cuwacunu::kikijyeba::protocol::make_component_stream_cursor(
              stream_wave, lifted.cursor),
          runtime_report_mode, elapsed_ms,
          static_cast<std::uint64_t>(input.data.numel()),
          static_cast<std::uint64_t>(out.node_encoding.numel()));
  out.stream_report.runtime_lls = out.runtime_lls;
  return out;
}

template <typename DatatypeT, typename EncoderT>
class channel_representation_stream_t {
public:
  using lifted_stream_t = cuwacunu::wikimyei::expression::nodelift::srl::
      stream::node_lifted_stream_t<DatatypeT>;
  using key_t = typename DatatypeT::key_type_t;
  using batch_t = channel_representation_batch_t<key_t>;

  channel_representation_stream_t(
      lifted_stream_t &&lifted_stream, EncoderT &encoder,
      bool require_finite_valid_features = true, bool detach_to_cpu = false,
      cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t
          runtime_report_mode = cuwacunu::kikijyeba::lattice::runtime_report::
              runtime_report_mode_t::normal,
      std::string component_assembly_id = "vicreg_v1",
      std::string assembly_token = "wikimyei.representation.vicreg.v1",
      std::string dock_binding_token = {},
      std::string component_family_id =
          "wikimyei.representation.encoding.vicreg",
      std::string runtime_document_schema_id =
          "wikimyei.representation.vicreg.runtime.v1",
      cuwacunu::kikijyeba::protocol::component_stream_wave_t stream_wave = {})
      : lifted_stream_(std::move(lifted_stream)), encoder_(&encoder),
        require_finite_valid_features_(require_finite_valid_features),
        detach_to_cpu_(detach_to_cpu),
        runtime_report_mode_(runtime_report_mode),
        component_assembly_id_(std::move(component_assembly_id)),
        assembly_token_(std::move(assembly_token)),
        dock_binding_token_(std::move(dock_binding_token)),
        component_family_id_(std::move(component_family_id)),
        runtime_document_schema_id_(std::move(runtime_document_schema_id)),
        stream_wave_(std::move(stream_wave)) {
    TORCH_CHECK(encoder_ != nullptr,
                "[channel_representation_stream_t] encoder must not be null");
  }

  [[nodiscard]] bool has_next() const { return lifted_stream_.has_next(); }

  [[nodiscard]] std::size_t cursor() const { return lifted_stream_.cursor(); }

  void reset() { lifted_stream_.reset(); }

  batch_t next() {
    TORCH_CHECK(has_next(),
                "[channel_representation_stream_t] stream is exhausted");
    auto lifted = lifted_stream_.next();
    return make_channel_representation_stream_batch<EncoderT, key_t>(
        *encoder_, lifted, require_finite_valid_features_, detach_to_cpu_,
        runtime_report_mode_, component_assembly_id_, assembly_token_,
        dock_binding_token_, component_family_id_, runtime_document_schema_id_,
        stream_wave_);
  }

private:
  lifted_stream_t lifted_stream_;
  EncoderT *encoder_{nullptr};
  bool require_finite_valid_features_{true};
  bool detach_to_cpu_{false};
  cuwacunu::kikijyeba::lattice::runtime_report::runtime_report_mode_t
      runtime_report_mode_{cuwacunu::kikijyeba::lattice::runtime_report::
                               runtime_report_mode_t::normal};
  std::string component_assembly_id_{"vicreg_v1"};
  std::string assembly_token_{"wikimyei.representation.vicreg.v1"};
  std::string dock_binding_token_{};
  std::string component_family_id_{"wikimyei.representation.encoding.vicreg"};
  std::string runtime_document_schema_id_{
      "wikimyei.representation.vicreg.runtime.v1"};
  cuwacunu::kikijyeba::protocol::component_stream_wave_t stream_wave_{};
};

} // namespace
  // cuwacunu::wikimyei::representation::encoding::vicreg::stream
