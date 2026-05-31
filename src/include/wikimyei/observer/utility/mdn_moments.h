// SPDX-License-Identifier: MIT
#pragma once

#include <torch/torch.h>

#include "wikimyei/inference/expected_value/mdn/mixture_density_network_types.h"

namespace cuwacunu::wikimyei::observer {

struct mdn_moments_t {
  torch::Tensor pi{};                   // [B,N,C,Df,K]
  torch::Tensor mean{};                 // [B,N,C,Df]
  torch::Tensor variance{};             // [B,N,C,Df]
  torch::Tensor aleatoric_variance{};   // [B,N,C,Df]
  torch::Tensor mixture_disagreement{}; // [B,N,C,Df]
  torch::Tensor second_moment{};        // [B,N,C,Df]
};

inline void validate_mdn_out(
    const cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut &out) {
  TORCH_CHECK(out.log_pi.defined() && out.mu.defined() && out.sigma.defined(),
              "[mdn_moments] log_pi/mu/sigma must be defined");
  TORCH_CHECK(out.log_pi.dim() == 5,
              "[mdn_moments] log_pi must be [B,N,C,Df,K]");
  TORCH_CHECK(out.mu.sizes() == out.log_pi.sizes(),
              "[mdn_moments] mu shape mismatch");
  TORCH_CHECK(out.sigma.sizes() == out.log_pi.sizes(),
              "[mdn_moments] sigma shape mismatch");
}

[[nodiscard]] inline mdn_moments_t compute_mdn_moments(
    const cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut &out) {
  validate_mdn_out(out);
  auto pi = out.log_pi.exp();
  auto mean = (pi * out.mu).sum(/*dim=*/-1);
  auto sigma2 = out.sigma.pow(2);
  auto second = (pi * (sigma2 + out.mu.pow(2))).sum(/*dim=*/-1);
  auto variance = (second - mean.pow(2)).clamp_min(0.0);
  auto aleatoric = (pi * sigma2).sum(/*dim=*/-1);
  auto disagreement =
      (pi * (out.mu - mean.unsqueeze(-1)).pow(2)).sum(/*dim=*/-1);

  mdn_moments_t result{};
  result.pi = std::move(pi);
  result.mean = std::move(mean);
  result.variance = std::move(variance);
  result.aleatoric_variance = std::move(aleatoric);
  result.mixture_disagreement = std::move(disagreement);
  result.second_moment = std::move(second);
  return result;
}

} // namespace cuwacunu::wikimyei::observer
