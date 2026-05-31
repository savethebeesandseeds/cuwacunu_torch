// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>

#include <torch/torch.h>

#include "wikimyei/observer/utility/forecast_artifact.h"

namespace cuwacunu::wikimyei::observer {

struct surprise_options_t {
  double sigma_floor{1.0e-12};
  double score_soft_nll{4.0};
  double score_scale{4.0};
};

struct surprise_t {
  torch::Tensor surprise{};   // [A], -log p(realized)
  torch::Tensor log_prob{};   // [A]
  torch::Tensor score{};      // [A], [0,1], neutral for invalid observations
  torch::Tensor valid_mask{}; // [A], bool
};

[[nodiscard]] inline torch::Tensor neutral_surprise_score(
    std::int64_t asset_count,
    torch::TensorOptions options =
        torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU)) {
  TORCH_CHECK(asset_count >= 0, "[surprise] asset_count must be nonnegative");
  return torch::ones({asset_count}, options);
}

[[nodiscard]] inline torch::Tensor normal_log_prob(const torch::Tensor &x,
                                                   const torch::Tensor &mu,
                                                   const torch::Tensor &sigma) {
  static constexpr double kLogTwoPi = 1.83787706640934548356;
  auto s = sigma.clamp_min(1.0e-12);
  auto z = (x - mu) / s;
  return -0.5 * (z.pow(2) + kLogTwoPi) - s.log();
}

[[nodiscard]] inline torch::Tensor
mixture_log_prob(const torch::Tensor &log_weight, const torch::Tensor &mu,
                 const torch::Tensor &sigma,
                 const torch::Tensor &realized_log_return,
                 double sigma_floor = 1.0e-12) {
  TORCH_CHECK(log_weight.defined() && log_weight.dim() == 2,
              "[surprise] log_weight must be [A,Kc]");
  TORCH_CHECK(mu.sizes() == log_weight.sizes() &&
                  sigma.sizes() == log_weight.sizes(),
              "[surprise] mixture tensor shape mismatch");
  TORCH_CHECK(realized_log_return.defined() && realized_log_return.dim() == 1 &&
                  realized_log_return.size(0) == log_weight.size(0),
              "[surprise] realized_log_return must be [A]");
  auto options = log_weight.options().dtype(torch::kFloat64);
  auto lw = log_weight.to(options);
  auto m = mu.to(options);
  auto s = sigma.to(options).clamp_min(sigma_floor);
  auto x = realized_log_return.to(options).unsqueeze(-1);
  return torch::logsumexp(lw + normal_log_prob(x, m, s), /*dim=*/-1);
}

[[nodiscard]] inline surprise_t compute_from_marginal_log_return(
    const torch::Tensor &log_weight, const torch::Tensor &mu,
    const torch::Tensor &sigma, const torch::Tensor &realized_log_return,
    const torch::Tensor &realized_mask = torch::Tensor(),
    const torch::Tensor &active_mask = torch::Tensor(),
    const surprise_options_t &options = {}) {
  TORCH_CHECK(options.sigma_floor > 0.0, "[surprise] sigma_floor must be > 0");
  TORCH_CHECK(options.score_scale > 0.0, "[surprise] score_scale must be > 0");
  auto log_prob = mixture_log_prob(log_weight, mu, sigma, realized_log_return,
                                   options.sigma_floor);
  const auto A = log_weight.size(0);
  auto tensor_options = log_prob.options();
  auto valid = torch::isfinite(log_prob).logical_and(
      torch::isfinite(realized_log_return.to(tensor_options)));
  if (realized_mask.defined()) {
    TORCH_CHECK(realized_mask.dim() == 1 && realized_mask.size(0) == A,
                "[surprise] realized_mask must be [A]");
    valid = valid.logical_and(realized_mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(log_prob.device())));
  }
  if (active_mask.defined()) {
    TORCH_CHECK(active_mask.dim() == 1 && active_mask.size(0) == A,
                "[surprise] active_mask must be [A]");
    valid = valid.logical_and(active_mask.to(
        torch::TensorOptions().dtype(torch::kBool).device(log_prob.device())));
  }

  auto nll = -log_prob;
  auto excess = (nll - options.score_soft_nll).clamp_min(0.0);
  auto score = torch::exp(-excess / options.score_scale).clamp(0.0, 1.0);
  nll = torch::where(valid, nll, torch::zeros_like(nll));
  log_prob = torch::where(valid, log_prob, torch::zeros_like(log_prob));
  score = torch::where(valid, score, torch::ones_like(score));

  surprise_t result{};
  result.surprise = std::move(nll);
  result.log_prob = std::move(log_prob);
  result.score = std::move(score);
  result.valid_mask = std::move(valid);
  return result;
}

[[nodiscard]] inline surprise_t
compute_from_artifact(const marginal_forecast_artifact_t &artifact,
                      const torch::Tensor &realized_log_return,
                      const torch::Tensor &realized_mask = torch::Tensor(),
                      const surprise_options_t &options = {}) {
  validate_forecast_artifact(artifact);
  return compute_from_marginal_log_return(
      artifact.log_weight, artifact.mu, artifact.sigma, realized_log_return,
      realized_mask, artifact.active_mask, options);
}

} // namespace cuwacunu::wikimyei::observer
