// SPDX-License-Identifier: MIT
#pragma once

#include <cstdint>
#include <utility>

#include <torch/torch.h>

namespace cuwacunu::wikimyei::representation::encoding::vicreg {

struct channel_global_fusion_options_t {
  int64_t channel_count{0};
  int64_t encoding_dim{0};
  int64_t global_dim{0};
  int64_t hidden_dim{0};
  torch::Dtype dtype{torch::kFloat32};
  torch::Device device{torch::kCPU};
};

struct channel_global_fusion_output_t {
  torch::Tensor global_context{};  // [B,N,Dg]
  torch::Tensor global_mask{};     // [B,N], bool
  torch::Tensor channel_weights{}; // [B,N,C]
};

namespace channel_global_fusion_detail {

inline void validate_options(const channel_global_fusion_options_t &opts) {
  TORCH_CHECK(opts.channel_count > 0 && opts.encoding_dim > 0 &&
                  opts.global_dim > 0 && opts.hidden_dim > 0,
              "[channel_global_fusion] invalid dimensions");
}

inline void validate_input(const torch::Tensor &channel_context,
                           const torch::Tensor &channel_mask,
                           const channel_global_fusion_options_t &opts) {
  TORCH_CHECK(channel_context.defined() && channel_context.dim() == 4,
              "[channel_global_fusion] channel_context must be [B,N,C,De]");
  TORCH_CHECK(channel_mask.defined() && channel_mask.dim() == 3,
              "[channel_global_fusion] channel_mask must be [B,N,C]");
  TORCH_CHECK(channel_context.size(2) == opts.channel_count &&
                  channel_context.size(3) == opts.encoding_dim,
              "[channel_global_fusion] channel_context shape does not match "
              "options");
  TORCH_CHECK(channel_mask.size(0) == channel_context.size(0) &&
                  channel_mask.size(1) == channel_context.size(1) &&
                  channel_mask.size(2) == channel_context.size(2),
              "[channel_global_fusion] channel mask shape mismatch");
}

} // namespace channel_global_fusion_detail

class ChannelGlobalFusionImpl : public torch::nn::Module {
public:
  explicit ChannelGlobalFusionImpl(channel_global_fusion_options_t options)
      : options_(std::move(options)) {
    channel_global_fusion_detail::validate_options(options_);
    channel_score_ = register_module(
        "channel_score", torch::nn::Linear(options_.encoding_dim, 1));
    value_in_ =
        register_module("value_in", torch::nn::Linear(options_.encoding_dim,
                                                      options_.hidden_dim));
    value_out_ =
        register_module("value_out", torch::nn::Linear(options_.hidden_dim,
                                                       options_.global_dim));
    this->to(options_.device, options_.dtype);
  }

  [[nodiscard]] const channel_global_fusion_options_t &options() const {
    return options_;
  }

  [[nodiscard]] channel_global_fusion_output_t
  forward(const torch::Tensor &channel_context,
          const torch::Tensor &channel_mask) {
    channel_global_fusion_detail::validate_input(channel_context, channel_mask,
                                                 options_);
    auto z = channel_context.to(
        torch::TensorOptions().dtype(options_.dtype).device(options_.device));
    auto mask = channel_mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(options_.device));
    auto finite_or_masked =
        torch::isfinite(z).logical_or(mask.unsqueeze(-1).logical_not());
    TORCH_CHECK(finite_or_masked.all().template item<bool>(),
                "[channel_global_fusion] non-finite context value under true "
                "channel mask");

    const auto B = z.size(0);
    const auto N = z.size(1);
    const auto C = z.size(2);
    const auto De = z.size(3);
    auto clean = torch::where(mask.unsqueeze(-1), z, torch::zeros_like(z));
    auto flat = clean.reshape({B * N * C, De});
    auto scores = channel_score_->forward(flat).view({B, N, C});
    scores = scores.masked_fill(mask.logical_not(), -1.0e30);
    auto weights = torch::softmax(scores, /*dim=*/2) * mask.to(z.dtype());
    auto denom = weights.sum(/*dim=*/2, /*keepdim=*/true);
    weights = torch::where(denom > 0.0, weights / denom.clamp_min(1e-12),
                           torch::zeros_like(weights));

    auto pooled = (clean * weights.unsqueeze(-1)).sum(/*dim=*/2);
    auto hidden = torch::gelu(value_in_->forward(pooled.reshape({B * N, De})));
    auto global = value_out_->forward(hidden).view({B, N, options_.global_dim});
    auto global_mask = mask.any(/*dim=*/2);
    global = global.masked_fill(global_mask.logical_not().unsqueeze(-1), 0.0);

    channel_global_fusion_output_t out{};
    out.global_context = std::move(global);
    out.global_mask = std::move(global_mask);
    out.channel_weights = std::move(weights);
    return out;
  }

private:
  channel_global_fusion_options_t options_{};
  torch::nn::Linear channel_score_{nullptr};
  torch::nn::Linear value_in_{nullptr};
  torch::nn::Linear value_out_{nullptr};
};

TORCH_MODULE(ChannelGlobalFusion);

} // namespace cuwacunu::wikimyei::representation::encoding::vicreg
