// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

namespace cuwacunu::wikimyei::representation::encoding::vicreg {

struct graph_row_index_t {
  torch::Tensor anchor_index{}; // [M], int64
  torch::Tensor node_index{};   // [M], int64
  int64_t B_anchor{0};
  int64_t N{0};
};

struct channel_representation_batch_t {
  torch::Tensor node_encoding{};       // [B,N,C,De] or [B,N,C,Hx,De]
  torch::Tensor node_encoding_mask{};  // [B,N,C] or [B,N,C,Hx]
  torch::Tensor reducer_weights{};     // optional [B,N,C,Hx]
  torch::Tensor anchor_keys{};         // [B]
  std::vector<std::string> node_ids{}; // [N]
};

namespace channel_representation_adapter_detail {

inline void validate_index(const graph_row_index_t &index, int64_t rows,
                           const torch::Device &device) {
  TORCH_CHECK(index.anchor_index.defined() && index.node_index.defined(),
              "[channel_representation_adapter] anchor/node index required");
  TORCH_CHECK(index.anchor_index.dim() == 1 && index.node_index.dim() == 1,
              "[channel_representation_adapter] anchor/node index must be [M]");
  TORCH_CHECK(index.anchor_index.size(0) == rows &&
                  index.node_index.size(0) == rows,
              "[channel_representation_adapter] index row count mismatch");
  TORCH_CHECK(index.B_anchor > 0 && index.N > 0,
              "[channel_representation_adapter] B_anchor and N must be > 0");
  auto anchor = index.anchor_index.to(
      torch::TensorOptions().dtype(torch::kInt64).device(device));
  auto node = index.node_index.to(
      torch::TensorOptions().dtype(torch::kInt64).device(device));
  TORCH_CHECK((anchor >= 0).all().item<bool>() &&
                  (anchor < index.B_anchor).all().item<bool>(),
              "[channel_representation_adapter] anchor index out of range");
  TORCH_CHECK((node >= 0).all().item<bool>() &&
                  (node < index.N).all().item<bool>(),
              "[channel_representation_adapter] node index out of range");
}

} // namespace channel_representation_adapter_detail

[[nodiscard]] inline torch::Tensor
reshape_channel_representation(const torch::Tensor &encoded,
                               const graph_row_index_t &index) {
  TORCH_CHECK(encoded.defined(),
              "[channel_representation_adapter] encoded tensor is undefined");
  TORCH_CHECK(encoded.dim() == 3 || encoded.dim() == 4,
              "[channel_representation_adapter] encoded must be [M,C,De] or "
              "[M,C,Hx,De]");
  channel_representation_adapter_detail::validate_index(index, encoded.size(0),
                                                        encoded.device());
  using torch::indexing::Slice;
  auto anchor = index.anchor_index.to(
      torch::TensorOptions().dtype(torch::kInt64).device(encoded.device()));
  auto node = index.node_index.to(
      torch::TensorOptions().dtype(torch::kInt64).device(encoded.device()));
  if (encoded.dim() == 3) {
    auto out = torch::zeros(
        {index.B_anchor, index.N, encoded.size(1), encoded.size(2)},
        encoded.options());
    out.index_put_({anchor, node, Slice(), Slice()}, encoded);
    return out;
  }
  auto out = torch::zeros({index.B_anchor, index.N, encoded.size(1),
                           encoded.size(2), encoded.size(3)},
                          encoded.options());
  out.index_put_({anchor, node, Slice(), Slice(), Slice()}, encoded);
  return out;
}

[[nodiscard]] inline torch::Tensor
reshape_channel_mask(const torch::Tensor &mask,
                     const graph_row_index_t &index) {
  TORCH_CHECK(mask.defined(),
              "[channel_representation_adapter] mask tensor is undefined");
  TORCH_CHECK(mask.dim() == 2 || mask.dim() == 3,
              "[channel_representation_adapter] mask must be [M,C] or "
              "[M,C,Hx]");
  channel_representation_adapter_detail::validate_index(index, mask.size(0),
                                                        mask.device());
  using torch::indexing::Slice;
  auto anchor = index.anchor_index.to(
      torch::TensorOptions().dtype(torch::kInt64).device(mask.device()));
  auto node = index.node_index.to(
      torch::TensorOptions().dtype(torch::kInt64).device(mask.device()));
  auto mask_bool = mask.to(torch::kBool);
  if (mask.dim() == 2) {
    auto out = torch::zeros(
        {index.B_anchor, index.N, mask.size(1)},
        torch::TensorOptions().dtype(torch::kBool).device(mask.device()));
    out.index_put_({anchor, node, Slice()}, mask_bool);
    return out;
  }
  auto out = torch::zeros(
      {index.B_anchor, index.N, mask.size(1), mask.size(2)},
      torch::TensorOptions().dtype(torch::kBool).device(mask.device()));
  out.index_put_({anchor, node, Slice(), Slice()}, mask_bool);
  return out;
}

[[nodiscard]] inline torch::Tensor
reshape_channel_sidecar(const torch::Tensor &tensor,
                        const graph_row_index_t &index) {
  TORCH_CHECK(tensor.defined(),
              "[channel_representation_adapter] sidecar tensor is undefined");
  TORCH_CHECK(tensor.dim() == 2 || tensor.dim() == 3,
              "[channel_representation_adapter] sidecar must be [M,C] or "
              "[M,C,Hx]");
  channel_representation_adapter_detail::validate_index(index, tensor.size(0),
                                                        tensor.device());
  using torch::indexing::Slice;
  auto anchor = index.anchor_index.to(
      torch::TensorOptions().dtype(torch::kInt64).device(tensor.device()));
  auto node = index.node_index.to(
      torch::TensorOptions().dtype(torch::kInt64).device(tensor.device()));
  if (tensor.dim() == 2) {
    auto out = torch::zeros({index.B_anchor, index.N, tensor.size(1)},
                            tensor.options());
    out.index_put_({anchor, node, Slice()}, tensor);
    return out;
  }
  auto out =
      torch::zeros({index.B_anchor, index.N, tensor.size(1), tensor.size(2)},
                   tensor.options());
  out.index_put_({anchor, node, Slice(), Slice()}, tensor);
  return out;
}

[[nodiscard]] inline channel_representation_batch_t
make_channel_representation_batch(const torch::Tensor &encoded,
                                  const torch::Tensor &mask,
                                  const graph_row_index_t &index,
                                  const torch::Tensor &anchor_keys = {},
                                  std::vector<std::string> node_ids = {},
                                  const torch::Tensor &reducer_weights = {}) {
  channel_representation_batch_t out{};
  out.node_encoding = reshape_channel_representation(encoded, index);
  out.node_encoding_mask = reshape_channel_mask(mask, index);
  if (reducer_weights.defined()) {
    out.reducer_weights = reshape_channel_sidecar(reducer_weights, index);
  }
  out.anchor_keys = anchor_keys;
  out.node_ids = std::move(node_ids);
  return out;
}

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg
