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

#include <torch/torch.h>

#include "kikijyeba/lattice/runtime_report/component_runtime_lls.h"
#include "kikijyeba/protocol/component_stream.h"
#include "wikimyei/expression/nodelift/srl/stream/node_lifted_stream.h"
#include "wikimyei/representation/encoding/vicreg/node_stream_adapter.h"
#include "wikimyei/representation/encoding/vicreg/stream/node_representation_batch.h"

namespace cuwacunu::wikimyei::representation::encoding::vicreg::stream {

namespace node_representation_stream_detail {

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

inline void move_encoder_input_tensors(torch::Tensor &data, torch::Tensor &mask,
                                       torch::Dtype dtype,
                                       const torch::Device &device) {
  data = data.to(torch::TensorOptions().dtype(dtype).device(device));
  mask = mask.to(torch::TensorOptions().dtype(torch::kBool).device(device));
}

template <typename EncoderT>
inline void move_encoder_input_tensors_for_encoder(EncoderT &encoder,
                                                   torch::Tensor &data,
                                                   torch::Tensor &mask) {
  if constexpr (requires(EncoderT e) {
                  e->dtype;
                  e->device;
                }) {
    move_encoder_input_tensors(data, mask, encoder->dtype, encoder->device);
  } else if constexpr (requires(EncoderT e) {
                         e.dtype;
                         e.device;
                       }) {
    move_encoder_input_tensors(data, mask, encoder.dtype, encoder.device);
  }
}

template <typename EncoderT>
[[nodiscard]] torch::Tensor
encode_node_rows(EncoderT &encoder, const torch::Tensor &data,
                 const torch::Tensor &mask, bool use_swa, bool detach_to_cpu) {
  if constexpr (requires(EncoderT e, torch::Tensor d, torch::Tensor m, bool u,
                         bool c) { e.encode(d, m, u, c); }) {
    return encoder.encode(data, mask, use_swa, detach_to_cpu);
  } else if constexpr (requires(EncoderT e, torch::Tensor d, torch::Tensor m) {
                         e->forward(d, m);
                       }) {
    auto out = encoder->forward(data, mask);
    if (detach_to_cpu) {
      out = out.detach().to(torch::kCPU);
    }
    return out;
  } else if constexpr (requires(EncoderT e, torch::Tensor d, torch::Tensor m) {
                         e.forward(d, m);
                       }) {
    auto out = encoder.forward(data, mask);
    if (detach_to_cpu) {
      out = out.detach().to(torch::kCPU);
    }
    return out;
  } else {
    static_assert(sizeof(EncoderT) == 0,
                  "node_representation_stream requires encode(data, mask, "
                  "use_swa, detach_to_cpu) or forward(data, mask)");
  }
}

template <typename KeyT>
[[nodiscard]] std::string make_node_representation_runtime_lls(
    const cuwacunu::wikimyei::expression::nodelift::srl::stream::
        node_lifted_batch_t<KeyT> &lifted,
    const cuwacunu::wikimyei::representation::encoding::vicreg::
        node_encoder_input_t &input,
    const torch::Tensor &node_encoding, const torch::Tensor &node_encoding_mask,
    bool use_swa, bool detach_to_cpu, double elapsed_ms,
    const std::string &component_id = "node_vicreg_v1",
    const std::string &assembly_token = "wikimyei.representation.vicreg.v1",
    const std::string &dock_binding_token = {},
    const cuwacunu::kikijyeba::protocol::component_stream_wave_t &stream_wave =
        {}) {
  auto stream_report =
      cuwacunu::kikijyeba::protocol::make_component_stream_report(
          cuwacunu::kikijyeba::protocol::component_stream_identity_t{
              .component_family = "wikimyei.representation.encoding.vicreg",
              .component_id = component_id,
              .assembly_token = assembly_token,
              .dock_binding_token = dock_binding_token,
              .graph_order_fingerprint = lifted.graph_order_fingerprint,
          },
          cuwacunu::kikijyeba::protocol::make_component_stream_cursor(
              stream_wave, lifted.cursor),
          cuwacunu::kikijyeba::lattice::runtime_report::
              runtime_report_mode_t::debug,
          elapsed_ms, static_cast<std::uint64_t>(input.data.numel()),
          static_cast<std::uint64_t>(node_encoding.numel()));
  auto document =
      cuwacunu::kikijyeba::protocol::make_component_stream_runtime_document(
          "wikimyei.representation.vicreg.runtime.v1", stream_report);
  lls::append_graph_anchor_cursor_entries(document, lifted.cursor, "batch",
                                          false);
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "anchor_count", static_cast<std::uint64_t>(input.B_anchor)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "node_count", static_cast<std::uint64_t>(input.N)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "encoded_rank", static_cast<std::uint64_t>(node_encoding.dim())));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "encoded_numel", static_cast<std::uint64_t>(node_encoding.numel())));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "original_rows",
      static_cast<std::uint64_t>(input.diagnostics.original_rows)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "kept_rows", static_cast<std::uint64_t>(input.diagnostics.kept_rows)));
  document.entries.push_back(lls::make_component_runtime_lls_uint_entry(
      "dropped_rows",
      static_cast<std::uint64_t>(input.diagnostics.dropped_rows)));
  document.entries.push_back(
      lls::make_component_runtime_lls_bool_entry("use_swa", use_swa));
  document.entries.push_back(lls::make_component_runtime_lls_bool_entry(
      "detach_to_cpu", detach_to_cpu));
  append_finite_double(document, "node_encoding_mask_fraction",
                       bool_fraction_or_nan(node_encoding_mask), "[0,1]");
  append_finite_double(document, "node_encoding_mean",
                       tensor_mean_or_nan(node_encoding));
  append_finite_double(document, "valid_channel_time_fraction",
                       input.diagnostics.valid_channel_time_fraction, "[0,1]");
  append_finite_double(document, "kept_valid_channel_time_fraction",
                       input.diagnostics.kept_valid_channel_time_fraction,
                       "[0,1]");
  return lls::emit_component_runtime_lls_canonical(document);
}

} // namespace node_representation_stream_detail

template <typename EncoderT, typename KeyT>
[[nodiscard]] node_representation_batch_t<KeyT> make_node_representation_batch(
    EncoderT &encoder,
    const cuwacunu::wikimyei::expression::nodelift::srl::stream::
        node_lifted_batch_t<KeyT> &lifted,
    const cuwacunu::wikimyei::representation::encoding::vicreg::
        node_vicreg_adapter_options_t &adapter_options =
            cuwacunu::wikimyei::representation::encoding::vicreg::
                node_vicreg_encoding_adapter_options(),
    bool use_swa = true, bool detach_to_cpu = false,
    cuwacunu::kikijyeba::lattice::runtime_report::
        runtime_report_mode_t runtime_report_mode =
            cuwacunu::kikijyeba::lattice::runtime_report::
                runtime_report_mode_t::normal,
    std::string component_id = "node_vicreg_v1",
    std::string assembly_token = "wikimyei.representation.vicreg.v1",
    std::string dock_binding_token = {},
    cuwacunu::kikijyeba::protocol::component_stream_wave_t stream_wave = {}) {
  TORCH_CHECK(!adapter_options.compact_valid_nodes_for_training,
              "[node_representation_stream] representation encoding requires "
              "non-compacting adapter options");

  auto input = cuwacunu::wikimyei::representation::encoding::vicreg::
      make_node_encoder_input(lifted, adapter_options);
  node_representation_stream_detail::move_encoder_input_tensors_for_encoder(
      encoder, input.data, input.mask);
  const auto encode_begin = std::chrono::steady_clock::now();
  auto encoded_rows =
      node_representation_stream_detail::encode_node_rows<EncoderT>(
          encoder, input.data, input.mask, use_swa, detach_to_cpu);
  auto node_encoding = cuwacunu::wikimyei::representation::encoding::vicreg::
      reshape_node_encoding(encoded_rows, input);
  auto node_encoding_mask = cuwacunu::wikimyei::representation::encoding::
                                vicreg::reshape_node_window_mask(input)
                                    .to(torch::TensorOptions()
                                            .dtype(torch::kBool)
                                            .device(node_encoding.device()));
  const auto encode_end = std::chrono::steady_clock::now();

  node_representation_batch_t<KeyT> out{};
  out.node_encoding = std::move(node_encoding);
  out.node_encoding_mask = std::move(node_encoding_mask);
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
  if (cuwacunu::kikijyeba::lattice::runtime_report::
          runtime_report_enabled(runtime_report_mode)) {
    out.runtime_lls =
        node_representation_stream_detail::make_node_representation_runtime_lls(
            lifted, input, out.node_encoding, out.node_encoding_mask, use_swa,
            detach_to_cpu,
            node_representation_stream_detail::elapsed_millis(encode_begin,
                                                              encode_end),
            component_id, assembly_token, dock_binding_token, stream_wave);
  }
  out.stream_report =
      cuwacunu::kikijyeba::protocol::make_component_stream_report(
          cuwacunu::kikijyeba::protocol::component_stream_identity_t{
              .component_family = "wikimyei.representation.encoding.vicreg",
              .component_id = std::move(component_id),
              .assembly_token = std::move(assembly_token),
              .dock_binding_token = std::move(dock_binding_token),
              .graph_order_fingerprint = out.graph_order_fingerprint,
          },
          cuwacunu::kikijyeba::protocol::make_component_stream_cursor(
              stream_wave, lifted.cursor),
          runtime_report_mode,
          node_representation_stream_detail::elapsed_millis(encode_begin,
                                                            encode_end),
          static_cast<std::uint64_t>(input.data.numel()),
          static_cast<std::uint64_t>(out.node_encoding.numel()));
  out.stream_report.runtime_lls = out.runtime_lls;
  return out;
}

template <typename DatatypeT, typename EncoderT>
class node_representation_stream_t {
public:
  using lifted_stream_t = cuwacunu::wikimyei::expression::nodelift::srl::
      stream::node_lifted_stream_t<DatatypeT>;
  using key_t = typename DatatypeT::key_type_t;
  using batch_t = node_representation_batch_t<key_t>;

  node_representation_stream_t(
      lifted_stream_t &&lifted_stream, EncoderT &encoder,
      cuwacunu::wikimyei::representation::encoding::vicreg::
          node_vicreg_adapter_options_t adapter_options =
              cuwacunu::wikimyei::representation::encoding::vicreg::
                  node_vicreg_encoding_adapter_options(),
      bool use_swa = true, bool detach_to_cpu = false,
      cuwacunu::kikijyeba::lattice::runtime_report::
          runtime_report_mode_t runtime_report_mode =
              cuwacunu::kikijyeba::lattice::
                  runtime_report::runtime_report_mode_t::normal,
      std::string component_id = "node_vicreg_v1",
      std::string assembly_token = "wikimyei.representation.vicreg.v1",
      std::string dock_binding_token = {},
      cuwacunu::kikijyeba::protocol::component_stream_wave_t stream_wave = {})
      : lifted_stream_(std::move(lifted_stream)), encoder_(&encoder),
        adapter_options_(std::move(adapter_options)), use_swa_(use_swa),
        detach_to_cpu_(detach_to_cpu),
        runtime_report_mode_(runtime_report_mode),
        component_id_(std::move(component_id)),
        assembly_token_(std::move(assembly_token)),
        dock_binding_token_(std::move(dock_binding_token)),
        stream_wave_(std::move(stream_wave)) {
    TORCH_CHECK(encoder_ != nullptr,
                "[node_representation_stream_t] encoder must not be null");
    TORCH_CHECK(!adapter_options_.compact_valid_nodes_for_training,
                "[node_representation_stream_t] representation stream requires "
                "non-compacting adapter options");
  }

  [[nodiscard]] bool has_next() const { return lifted_stream_.has_next(); }

  [[nodiscard]] std::size_t cursor() const { return lifted_stream_.cursor(); }

  void reset() { lifted_stream_.reset(); }

  batch_t next() {
    TORCH_CHECK(has_next(),
                "[node_representation_stream_t] stream is exhausted");
    auto lifted = lifted_stream_.next();
    return make_node_representation_batch<EncoderT, key_t>(
        *encoder_, lifted, adapter_options_, use_swa_, detach_to_cpu_,
        runtime_report_mode_, component_id_, assembly_token_,
        dock_binding_token_, stream_wave_);
  }

private:
  lifted_stream_t lifted_stream_;
  EncoderT *encoder_{nullptr};
  cuwacunu::wikimyei::representation::encoding::vicreg::
      node_vicreg_adapter_options_t adapter_options_{};
  bool use_swa_{true};
  bool detach_to_cpu_{false};
  cuwacunu::kikijyeba::lattice::runtime_report::
      runtime_report_mode_t runtime_report_mode_{
          cuwacunu::kikijyeba::lattice::runtime_report::
              runtime_report_mode_t::normal};
  std::string component_id_{"node_vicreg_v1"};
  std::string assembly_token_{"wikimyei.representation.vicreg.v1"};
  std::string dock_binding_token_{};
  cuwacunu::kikijyeba::protocol::component_stream_wave_t stream_wave_{};
};

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg::stream
