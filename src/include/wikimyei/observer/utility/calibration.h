// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <cstdint>
#include <vector>

#include <torch/torch.h>

#include "wikimyei/observer/utility/forecast_artifact.h"
#include "wikimyei/observer/utility/surprise.h"

namespace cuwacunu::wikimyei::observer {

struct calibration_options_t {
  double sigma_floor{1.0e-12};
  double nll_soft_limit{4.0};
  double nll_scale{4.0};
  double coverage_weight{1.0};
  double pit_center_weight{0.25};
};

struct calibration_observation_t {
  torch::Tensor nll{};           // [A]
  torch::Tensor pit{};           // [A], probability integral transform
  torch::Tensor hit_50{};        // [A]
  torch::Tensor hit_80{};        // [A]
  torch::Tensor hit_95{};        // [A]
  torch::Tensor lower_tail_05{}; // [A]
  torch::Tensor upper_tail_95{}; // [A]
  torch::Tensor valid_mask{};    // [A], bool
};

struct calibration_summary_t {
  torch::Tensor observation_count{};  // [A]
  torch::Tensor mean_nll{};           // [A]
  torch::Tensor pit_mean{};           // [A]
  torch::Tensor coverage_50{};        // [A]
  torch::Tensor coverage_80{};        // [A]
  torch::Tensor coverage_95{};        // [A]
  torch::Tensor lower_tail_05_rate{}; // [A]
  torch::Tensor upper_tail_95_rate{}; // [A]
  torch::Tensor score{};              // [A], [0,1]
};

[[nodiscard]] inline torch::Tensor neutral_calibration_score(
    std::int64_t asset_count,
    torch::TensorOptions options =
        torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU)) {
  TORCH_CHECK(asset_count >= 0,
              "[calibration] asset_count must be nonnegative");
  return torch::ones({asset_count}, options);
}

[[nodiscard]] inline torch::Tensor
calibration_standard_normal_cdf(const torch::Tensor &x) {
  return 0.5 * (1.0 + torch::erf(x / std::sqrt(2.0)));
}

[[nodiscard]] inline torch::Tensor
mixture_cdf(const torch::Tensor &log_weight, const torch::Tensor &mu,
            const torch::Tensor &sigma,
            const torch::Tensor &realized_log_return,
            double sigma_floor = 1.0e-12) {
  TORCH_CHECK(log_weight.defined() && log_weight.dim() == 2,
              "[calibration] log_weight must be [A,Kc]");
  TORCH_CHECK(mu.sizes() == log_weight.sizes() &&
                  sigma.sizes() == log_weight.sizes(),
              "[calibration] mixture tensor shape mismatch");
  TORCH_CHECK(realized_log_return.defined() && realized_log_return.dim() == 1 &&
                  realized_log_return.size(0) == log_weight.size(0),
              "[calibration] realized_log_return must be [A]");
  auto options = log_weight.options().dtype(torch::kFloat64);
  auto lw = log_weight.to(options);
  auto m = mu.to(options);
  auto s = sigma.to(options).clamp_min(sigma_floor);
  auto x = realized_log_return.to(options).unsqueeze(-1);
  return (lw.exp() * calibration_standard_normal_cdf((x - m) / s))
      .sum(/*dim=*/-1)
      .clamp(0.0, 1.0);
}

[[nodiscard]] inline calibration_observation_t observe_from_marginal_log_return(
    const torch::Tensor &log_weight, const torch::Tensor &mu,
    const torch::Tensor &sigma, const torch::Tensor &realized_log_return,
    const torch::Tensor &realized_mask = torch::Tensor(),
    const torch::Tensor &active_mask = torch::Tensor(),
    const calibration_options_t &options = {}) {
  TORCH_CHECK(options.sigma_floor > 0.0,
              "[calibration] sigma_floor must be > 0");
  auto surprise_result = compute_from_marginal_log_return(
      log_weight, mu, sigma, realized_log_return, realized_mask, active_mask,
      {.sigma_floor = options.sigma_floor,
       .score_soft_nll = options.nll_soft_limit,
       .score_scale = options.nll_scale});
  auto pit = mixture_cdf(log_weight, mu, sigma, realized_log_return,
                         options.sigma_floor);
  auto valid = surprise_result.valid_mask.logical_and(torch::isfinite(pit));

  auto hit_50 = pit.ge(0.25).logical_and(pit.le(0.75)).to(torch::kFloat64);
  auto hit_80 = pit.ge(0.10).logical_and(pit.le(0.90)).to(torch::kFloat64);
  auto hit_95 = pit.ge(0.025).logical_and(pit.le(0.975)).to(torch::kFloat64);
  auto lower_05 = pit.lt(0.05).to(torch::kFloat64);
  auto upper_95 = pit.gt(0.95).to(torch::kFloat64);

  pit = torch::where(valid, pit, torch::zeros_like(pit));
  auto valid_f = valid.to(torch::kFloat64);
  calibration_observation_t out{};
  out.nll = surprise_result.surprise;
  out.pit = std::move(pit);
  out.hit_50 = hit_50 * valid_f;
  out.hit_80 = hit_80 * valid_f;
  out.hit_95 = hit_95 * valid_f;
  out.lower_tail_05 = lower_05 * valid_f;
  out.upper_tail_95 = upper_95 * valid_f;
  out.valid_mask = std::move(valid);
  return out;
}

[[nodiscard]] inline calibration_observation_t
observe_from_artifact(const marginal_forecast_artifact_t &artifact,
                      const torch::Tensor &realized_log_return,
                      const torch::Tensor &realized_mask = torch::Tensor(),
                      const calibration_options_t &options = {}) {
  validate_forecast_artifact(artifact);
  return observe_from_marginal_log_return(
      artifact.log_weight, artifact.mu, artifact.sigma, realized_log_return,
      realized_mask, artifact.active_mask, options);
}

[[nodiscard]] inline calibration_summary_t summarize_observations(
    const std::vector<calibration_observation_t> &observations,
    const calibration_options_t &options = {}) {
  TORCH_CHECK(!observations.empty(),
              "[calibration] observations must not be empty");
  TORCH_CHECK(options.nll_scale > 0.0, "[calibration] nll_scale must be > 0");
  TORCH_CHECK(options.coverage_weight >= 0.0,
              "[calibration] coverage_weight must be >= 0");
  TORCH_CHECK(options.pit_center_weight >= 0.0,
              "[calibration] pit_center_weight must be >= 0");

  const auto A = observations.front().nll.size(0);
  auto tensor_options =
      observations.front().nll.options().dtype(torch::kFloat64);
  auto count = torch::zeros({A}, tensor_options);
  auto sum_nll = torch::zeros({A}, tensor_options);
  auto sum_pit = torch::zeros({A}, tensor_options);
  auto sum_50 = torch::zeros({A}, tensor_options);
  auto sum_80 = torch::zeros({A}, tensor_options);
  auto sum_95 = torch::zeros({A}, tensor_options);
  auto sum_lower_05 = torch::zeros({A}, tensor_options);
  auto sum_upper_95 = torch::zeros({A}, tensor_options);

  for (const auto &obs : observations) {
    TORCH_CHECK(obs.nll.defined() && obs.nll.dim() == 1 && obs.nll.size(0) == A,
                "[calibration] observation nll must be [A]");
    TORCH_CHECK(obs.pit.sizes() == obs.nll.sizes() &&
                    obs.hit_50.sizes() == obs.nll.sizes() &&
                    obs.hit_80.sizes() == obs.nll.sizes() &&
                    obs.hit_95.sizes() == obs.nll.sizes() &&
                    obs.lower_tail_05.sizes() == obs.nll.sizes() &&
                    obs.upper_tail_95.sizes() == obs.nll.sizes(),
                "[calibration] observation shape mismatch");
    TORCH_CHECK(obs.valid_mask.defined() && obs.valid_mask.dim() == 1 &&
                    obs.valid_mask.size(0) == A,
                "[calibration] observation valid_mask must be [A]");
    auto valid = obs.valid_mask.to(tensor_options);
    count = count + valid;
    sum_nll = sum_nll + obs.nll.to(tensor_options) * valid;
    sum_pit = sum_pit + obs.pit.to(tensor_options) * valid;
    sum_50 = sum_50 + obs.hit_50.to(tensor_options);
    sum_80 = sum_80 + obs.hit_80.to(tensor_options);
    sum_95 = sum_95 + obs.hit_95.to(tensor_options);
    sum_lower_05 = sum_lower_05 + obs.lower_tail_05.to(tensor_options);
    sum_upper_95 = sum_upper_95 + obs.upper_tail_95.to(tensor_options);
  }

  auto denom = count.clamp_min(1.0);
  auto mean_nll = sum_nll / denom;
  auto pit_mean = sum_pit / denom;
  auto coverage_50 = sum_50 / denom;
  auto coverage_80 = sum_80 / denom;
  auto coverage_95 = sum_95 / denom;
  auto lower_tail = sum_lower_05 / denom;
  auto upper_tail = sum_upper_95 / denom;

  auto nll_penalty =
      (mean_nll - options.nll_soft_limit).clamp_min(0.0) / options.nll_scale;
  auto coverage_penalty = (coverage_50 - 0.50).abs() +
                          (coverage_80 - 0.80).abs() +
                          (coverage_95 - 0.95).abs();
  auto pit_penalty = (pit_mean - 0.50).abs();
  auto score =
      torch::exp(-nll_penalty - options.coverage_weight * coverage_penalty -
                 options.pit_center_weight * pit_penalty)
          .clamp(0.0, 1.0);
  score = torch::where(count.gt(0.0), score, torch::ones_like(score));

  calibration_summary_t out{};
  out.observation_count = std::move(count);
  out.mean_nll = std::move(mean_nll);
  out.pit_mean = std::move(pit_mean);
  out.coverage_50 = std::move(coverage_50);
  out.coverage_80 = std::move(coverage_80);
  out.coverage_95 = std::move(coverage_95);
  out.lower_tail_05_rate = std::move(lower_tail);
  out.upper_tail_95_rate = std::move(upper_tail);
  out.score = std::move(score);
  return out;
}

} // namespace cuwacunu::wikimyei::observer
