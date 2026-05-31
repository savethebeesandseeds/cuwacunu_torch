// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>

#include <torch/torch.h>

#include "wikimyei/observer/utility/channel_consensus.h"
#include "wikimyei/observer/utility/feature_semantics.h"

namespace cuwacunu::wikimyei::observer {

struct affine_target_transform_t {
  bool enabled{false};
  double scale{1.0};
  double loc{0.0};
};

struct nodelift_potential_surface_options_t {
  std::int64_t close_coord{kline_coord(registry::kline_feature_e::close)};
  affine_target_transform_t close_transform{};
};

struct nodelift_potential_surface_t {
  torch::Tensor log_weight{};         // [B,G,Kc]
  torch::Tensor potential_mu{};       // [B,G,Kc]
  torch::Tensor potential_sigma{};    // [B,G,Kc]
  torch::Tensor active_mask{};        // [B,G], bool
  torch::Tensor expected_potential{}; // [B,G]
  torch::Tensor potential_variance{}; // [B,G]
  std::int64_t source_coord{kline_coord(registry::kline_feature_e::close)};
};

[[nodiscard]] inline nodelift_potential_surface_t from_channel_consensus(
    const channel_consensus_t &consensus,
    const nodelift_potential_surface_options_t &options = {}) {
  TORCH_CHECK(consensus.log_weight.defined() && consensus.mu.defined() &&
                  consensus.sigma.defined(),
              "[nodelift_potential_surface] consensus mixture tensors must be "
              "defined");
  TORCH_CHECK(consensus.log_weight.dim() == 4,
              "[nodelift_potential_surface] consensus log_weight must be "
              "[B,G,Df,Kc]");
  TORCH_CHECK(consensus.mu.sizes() == consensus.log_weight.sizes() &&
                  consensus.sigma.sizes() == consensus.log_weight.sizes(),
              "[nodelift_potential_surface] consensus mixture shape mismatch");
  TORCH_CHECK(options.close_coord >= 0 &&
                  options.close_coord < consensus.log_weight.size(2),
              "[nodelift_potential_surface] close_coord out of range");
  TORCH_CHECK(options.close_transform.scale != 0.0,
              "[nodelift_potential_surface] affine scale must be nonzero");

  auto log_weight = consensus.log_weight.select(/*dim=*/2, options.close_coord);
  auto mu = consensus.mu.select(/*dim=*/2, options.close_coord);
  auto sigma = consensus.sigma.select(/*dim=*/2, options.close_coord);
  auto active =
      consensus.active_mask.defined()
          ? consensus.active_mask.select(/*dim=*/2, options.close_coord)
          : torch::ones({log_weight.size(0), log_weight.size(1)},
                        torch::TensorOptions()
                            .dtype(torch::kBool)
                            .device(log_weight.device()));

  if (options.close_transform.enabled) {
    mu = mu * options.close_transform.scale + options.close_transform.loc;
    sigma = sigma * std::abs(options.close_transform.scale);
  }

  auto pi = log_weight.exp();
  auto expected = (pi * mu).sum(/*dim=*/-1);
  auto second = (pi * (sigma.pow(2) + mu.pow(2))).sum(/*dim=*/-1);
  auto variance = (second - expected.pow(2)).clamp_min(0.0);

  expected = expected.masked_fill(active.logical_not(), 0.0);
  variance = variance.masked_fill(active.logical_not(), 0.0);

  nodelift_potential_surface_t result{};
  result.log_weight = std::move(log_weight);
  result.potential_mu = std::move(mu);
  result.potential_sigma = std::move(sigma);
  result.active_mask = std::move(active);
  result.expected_potential = std::move(expected);
  result.potential_variance = std::move(variance);
  result.source_coord = options.close_coord;
  return result;
}

[[nodiscard]] inline torch::Tensor
sample_independent_potentials(const nodelift_potential_surface_t &surface,
                              std::int64_t sample_count) {
  TORCH_CHECK(sample_count > 0,
              "[nodelift_potential_surface] sample_count must be positive");
  TORCH_CHECK(surface.log_weight.defined() && surface.potential_mu.defined() &&
                  surface.potential_sigma.defined(),
              "[nodelift_potential_surface] potential mixture tensors are "
              "required");
  TORCH_CHECK(surface.log_weight.dim() == 3,
              "[nodelift_potential_surface] log_weight must be [B,G,Kc]");
  const auto B = surface.log_weight.size(0);
  const auto N = surface.log_weight.size(1);
  const auto Kc = surface.log_weight.size(2);
  auto pi = surface.log_weight.exp().reshape({B * N, Kc});
  auto component = torch::multinomial(pi, sample_count, /*replacement=*/true)
                       .transpose(0, 1)
                       .contiguous(); // [S,B*N]
  auto mu_flat = surface.potential_mu.reshape({B * N, Kc});
  auto sigma_flat = surface.potential_sigma.reshape({B * N, Kc});
  auto gather_index = component.transpose(0, 1).contiguous(); // [B*N,S]
  auto selected_mu = mu_flat.gather(/*dim=*/1, gather_index);
  auto selected_sigma = sigma_flat.gather(/*dim=*/1, gather_index);
  auto potential =
      selected_mu + selected_sigma * torch::randn_like(selected_mu);
  return potential.transpose(0, 1).contiguous().view({sample_count, B, N});
}

} // namespace cuwacunu::wikimyei::observer
