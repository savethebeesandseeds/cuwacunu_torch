// SPDX-License-Identifier: MIT
#pragma once

#include <cstddef>
#include <type_traits>
#include <utility>

#include <torch/torch.h>

#include "wikimyei/expression/nodelift/srl/stream/node_lifted_stream.h"
#include "wikimyei/representation/encoding/vicreg/node_stream_adapter.h"
#include "wikimyei/representation/encoding/vicreg/stream/node_representation_batch.h"

namespace cuwacunu::wikimyei::representation::encoding::vicreg::stream {

namespace node_representation_stream_detail {

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
    bool use_swa = true, bool detach_to_cpu = false) {
  TORCH_CHECK(!adapter_options.compact_valid_nodes_for_training,
              "[node_representation_stream] representation encoding requires "
              "non-compacting adapter options");

  auto input = cuwacunu::wikimyei::representation::encoding::vicreg::
      make_node_encoder_input(lifted, adapter_options);
  node_representation_stream_detail::move_encoder_input_tensors_for_encoder(
      encoder, input.data, input.mask);
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

  node_representation_batch_t<KeyT> out{};
  out.node_encoding = std::move(node_encoding);
  out.node_encoding_mask = std::move(node_encoding_mask);
  out.anchor_keys = lifted.anchor_keys;
  out.node_ids = lifted.node_ids;
  out.edge_ids = lifted.edge_ids;
  out.graph_order_fingerprint = lifted.graph_order_fingerprint;
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
      bool use_swa = true, bool detach_to_cpu = false)
      : lifted_stream_(std::move(lifted_stream)), encoder_(&encoder),
        adapter_options_(std::move(adapter_options)), use_swa_(use_swa),
        detach_to_cpu_(detach_to_cpu) {
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
        *encoder_, lifted, adapter_options_, use_swa_, detach_to_cpu_);
  }

private:
  lifted_stream_t lifted_stream_;
  EncoderT *encoder_{nullptr};
  cuwacunu::wikimyei::representation::encoding::vicreg::
      node_vicreg_adapter_options_t adapter_options_{};
  bool use_swa_{true};
  bool detach_to_cpu_{false};
};

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg::stream
