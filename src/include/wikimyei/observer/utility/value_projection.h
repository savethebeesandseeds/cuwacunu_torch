// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/observer/utility/channel_consensus.h"

namespace cuwacunu::wikimyei::observer {

struct value_projection_options_t {
  std::vector<double> scale{}; // optional [Df], identity when empty
  std::vector<double> loc{};   // optional [Df], zero when empty

  // Coordinates whose projected support should be nonnegative, such as raw
  // volume-like targets. Log-return targets should not be listed here.
  std::vector<std::int64_t> nonnegative_support_coords{};
};

struct projected_mixture_t {
  torch::Tensor log_weight{};  // [B,N,Df,Kc]
  torch::Tensor mu{};          // [B,N,Df,Kc], projected units
  torch::Tensor sigma{};       // [B,N,Df,Kc], projected units
  torch::Tensor active_mask{}; // [B,N,Df], bool

  torch::Tensor projected_mean{};                    // [B,N,Df]
  torch::Tensor projected_variance{};                // [B,N,Df]
  torch::Tensor invalid_lower_support_probability{}; // [B,N,Df]
};

[[nodiscard]] inline torch::Tensor
value_projection_standard_normal_cdf(const torch::Tensor &x) {
  return 0.5 * (1.0 + torch::erf(x / std::sqrt(2.0)));
}

[[nodiscard]] inline torch::Tensor
vector_to_feature_tensor(const std::vector<double> &values, std::int64_t Df,
                         double fallback, torch::TensorOptions options) {
  if (values.empty()) {
    return torch::full({Df}, fallback, options);
  }
  TORCH_CHECK(static_cast<std::int64_t>(values.size()) == Df,
              "[value_projection] transform vector must have Df entries");
  return torch::tensor(values, options);
}

[[nodiscard]] inline projected_mixture_t
project_channel_consensus(const channel_consensus_t &consensus,
                          const value_projection_options_t &options = {}) {
  TORCH_CHECK(consensus.log_weight.defined() && consensus.mu.defined() &&
                  consensus.sigma.defined(),
              "[value_projection] consensus mixture tensors required");
  TORCH_CHECK(consensus.log_weight.dim() == 4,
              "[value_projection] consensus mixture must be [B,N,Df,Kc]");
  TORCH_CHECK(consensus.mu.sizes() == consensus.log_weight.sizes() &&
                  consensus.sigma.sizes() == consensus.log_weight.sizes(),
              "[value_projection] consensus mixture shape mismatch");

  const auto Df = consensus.log_weight.size(2);
  auto tensor_options = consensus.mu.options().dtype(torch::kFloat64);
  auto scale = vector_to_feature_tensor(options.scale, Df, 1.0, tensor_options)
                   .to(consensus.mu.device());
  auto loc = vector_to_feature_tensor(options.loc, Df, 0.0, tensor_options)
                 .to(consensus.mu.device());
  TORCH_CHECK((scale != 0.0).all().item<bool>(),
              "[value_projection] affine scale entries must be nonzero");

  auto mu = consensus.mu.to(tensor_options) * scale.view({1, 1, Df, 1}) +
            loc.view({1, 1, Df, 1});
  auto sigma =
      consensus.sigma.to(tensor_options) * scale.abs().view({1, 1, Df, 1});
  auto log_weight = consensus.log_weight.to(tensor_options);
  auto pi = log_weight.exp();
  auto mean = (pi * mu).sum(/*dim=*/-1);
  auto second = (pi * (sigma.pow(2) + mu.pow(2))).sum(/*dim=*/-1);
  auto variance = (second - mean.pow(2)).clamp_min(0.0);

  auto active =
      consensus.active_mask.defined()
          ? consensus.active_mask.to(torch::TensorOptions()
                                         .dtype(torch::kBool)
                                         .device(consensus.log_weight.device()))
          : torch::ones({consensus.log_weight.size(0),
                         consensus.log_weight.size(1), Df},
                        torch::TensorOptions()
                            .dtype(torch::kBool)
                            .device(consensus.log_weight.device()));
  mean = mean.masked_fill(active.logical_not(), 0.0);
  variance = variance.masked_fill(active.logical_not(), 0.0);

  auto invalid_lower = torch::zeros(
      {consensus.log_weight.size(0), consensus.log_weight.size(1), Df},
      tensor_options.device(consensus.log_weight.device()));
  for (const auto coord : options.nonnegative_support_coords) {
    TORCH_CHECK(coord >= 0 && coord < Df,
                "[value_projection] nonnegative support coord out of range");
    auto z =
        (0.0 - mu.select(2, coord)) / sigma.select(2, coord).clamp_min(1.0e-12);
    auto prob = (log_weight.select(2, coord).exp() *
                 value_projection_standard_normal_cdf(z))
                    .sum(-1);
    invalid_lower.select(2, coord).copy_(prob);
  }
  invalid_lower = invalid_lower.masked_fill(active.logical_not(), 0.0);

  projected_mixture_t out{};
  out.log_weight = std::move(log_weight);
  out.mu = std::move(mu);
  out.sigma = std::move(sigma);
  out.active_mask = std::move(active);
  out.projected_mean = std::move(mean);
  out.projected_variance = std::move(variance);
  out.invalid_lower_support_probability = std::move(invalid_lower);
  return out;
}

} // namespace cuwacunu::wikimyei::observer
