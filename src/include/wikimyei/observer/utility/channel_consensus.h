// SPDX-License-Identifier: MIT
#pragma once

#include <torch/torch.h>

#include "wikimyei/inference/expected_value/mdn/mixture_density_network_types.h"
#include "wikimyei/observer/utility/mdn_moments.h"

namespace cuwacunu::wikimyei::observer {

inline constexpr double kMaskedLogWeight = -1.0e30;

struct channel_consensus_t {
  torch::Tensor log_weight{};           // [B,N,Df,C*K]
  torch::Tensor mu{};                   // [B,N,Df,C*K]
  torch::Tensor sigma{};                // [B,N,Df,C*K]
  torch::Tensor active_mask{};          // [B,N,Df], bool
  torch::Tensor active_channel_count{}; // [B,N]
  torch::Tensor channel_weights{};      // [B,N,C]
  torch::Tensor channel_mean{};         // [B,N,C,Df]
  torch::Tensor combined_mean{};        // [B,N,Df]
  torch::Tensor combined_variance{};    // [B,N,Df]
  torch::Tensor channel_disagreement{}; // [B,N,Df]
};

inline void validate_channel_mask(
    const torch::Tensor &channel_mask,
    const cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut &out) {
  if (!channel_mask.defined()) {
    return;
  }
  TORCH_CHECK(channel_mask.dim() == 3,
              "[channel_consensus] channel_mask must be [B,N,C]");
  TORCH_CHECK(channel_mask.size(0) == out.log_pi.size(0) &&
                  channel_mask.size(1) == out.log_pi.size(1) &&
                  channel_mask.size(2) == out.log_pi.size(2),
              "[channel_consensus] channel_mask shape mismatch");
}

[[nodiscard]] inline channel_consensus_t
compute_uniform_valid_channel_consensus(
    const cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut &out,
    const torch::Tensor &channel_mask = torch::Tensor()) {
  validate_mdn_out(out);
  validate_channel_mask(channel_mask, out);

  const auto B = out.log_pi.size(0);
  const auto N = out.log_pi.size(1);
  const auto C = out.log_pi.size(2);
  const auto Df = out.log_pi.size(3);
  const auto K = out.log_pi.size(4);
  const auto dtype = out.log_pi.dtype();

  auto mask = channel_mask.defined()
                  ? channel_mask.to(torch::TensorOptions()
                                        .dtype(torch::kBool)
                                        .device(out.log_pi.device()))
                  : torch::ones({B, N, C}, torch::TensorOptions()
                                               .dtype(torch::kBool)
                                               .device(out.log_pi.device()));
  auto active_count = mask.to(torch::kInt64).sum(/*dim=*/2); // [B,N]
  auto active_float = active_count.to(dtype).clamp_min(1.0);
  auto channel_weights = mask.to(dtype) / active_float.unsqueeze(-1); // [B,N,C]
  channel_weights =
      torch::where(mask, channel_weights, torch::zeros_like(channel_weights));

  auto pi = out.log_pi.exp();
  auto channel_mean = (pi * out.mu).sum(/*dim=*/-1); // [B,N,C,Df]
  auto combined_mean =
      (channel_mean * channel_weights.unsqueeze(-1)).sum(/*dim=*/2);
  auto channel_disagreement =
      ((channel_mean - combined_mean.unsqueeze(2)).pow(2) *
       channel_weights.unsqueeze(-1))
          .sum(/*dim=*/2);

  auto log_channel_weight =
      torch::where(channel_weights > 0.0, channel_weights.log(),
                   torch::full_like(channel_weights, kMaskedLogWeight));
  auto log_weight = out.log_pi + log_channel_weight.unsqueeze(-1).unsqueeze(-1);
  log_weight = log_weight.masked_fill(
      mask.logical_not().unsqueeze(-1).unsqueeze(-1), kMaskedLogWeight);
  log_weight =
      log_weight.permute({0, 1, 3, 2, 4}).contiguous().view({B, N, Df, C * K});
  auto normalizer = torch::logsumexp(log_weight, /*dim=*/-1, /*keepdim=*/true);
  log_weight = log_weight - normalizer;

  auto active = active_count.gt(0).unsqueeze(-1).expand({B, N, Df});
  log_weight = log_weight.masked_fill(active.logical_not().unsqueeze(-1),
                                      kMaskedLogWeight);
  auto mu =
      out.mu.permute({0, 1, 3, 2, 4}).contiguous().view({B, N, Df, C * K});
  auto sigma =
      out.sigma.permute({0, 1, 3, 2, 4}).contiguous().view({B, N, Df, C * K});
  mu = mu.masked_fill(active.logical_not().unsqueeze(-1), 0.0);
  sigma = sigma.masked_fill(active.logical_not().unsqueeze(-1), 0.0);

  auto combined_pi = log_weight.exp();
  auto second = (combined_pi * (sigma.pow(2) + mu.pow(2))).sum(/*dim=*/-1);
  auto combined_variance = (second - combined_mean.pow(2)).clamp_min(0.0);
  combined_mean = combined_mean.masked_fill(active.logical_not(), 0.0);
  combined_variance = combined_variance.masked_fill(active.logical_not(), 0.0);
  channel_disagreement =
      channel_disagreement.masked_fill(active.logical_not(), 0.0);

  channel_consensus_t result{};
  result.log_weight = std::move(log_weight);
  result.mu = std::move(mu);
  result.sigma = std::move(sigma);
  result.active_mask = std::move(active);
  result.active_channel_count = std::move(active_count);
  result.channel_weights = std::move(channel_weights);
  result.channel_mean = std::move(channel_mean);
  result.combined_mean = std::move(combined_mean);
  result.combined_variance = std::move(combined_variance);
  result.channel_disagreement = std::move(channel_disagreement);
  return result;
}

} // namespace cuwacunu::wikimyei::observer
