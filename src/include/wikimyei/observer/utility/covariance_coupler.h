// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <tuple>
#include <vector>

#include <torch/torch.h>

namespace cuwacunu::wikimyei::observer {

struct covariance_coupler_options_t {
  std::int64_t sample_count{512};
  std::int64_t quantile_bisection_steps{40};
  double shrinkage{0.10};
  double repair_epsilon{1.0e-6};
  double cholesky_jitter{1.0e-8};
};

struct coupled_samples_t {
  torch::Tensor samples{};     // [S,M], sampled marginal values
  torch::Tensor covariance{};  // [M,M], sample-derived
  torch::Tensor correlation{}; // [M,M], repaired/shrunk input correlation
  torch::Tensor uniforms{};    // [S,M]
};

[[nodiscard]] inline torch::Tensor
covariance_standard_normal_cdf(const torch::Tensor &x) {
  return 0.5 * (1.0 + torch::erf(x / std::sqrt(2.0)));
}

[[nodiscard]] inline torch::Tensor
repair_correlation_matrix(const torch::Tensor &input, double epsilon = 1.0e-6) {
  TORCH_CHECK(input.defined() && input.dim() == 2 &&
                  input.size(0) == input.size(1),
              "[covariance_coupler] correlation must be [A,A]");
  TORCH_CHECK(input.size(0) > 0,
              "[covariance_coupler] correlation must not be empty");
  auto corr = input.to(torch::kFloat64).contiguous();
  corr = 0.5 * (corr + corr.transpose(0, 1));
  corr.diagonal().fill_(1.0);

  auto eig = torch::linalg_eigh(corr);
  auto values = std::get<0>(eig).clamp_min(epsilon);
  auto vectors = std::get<1>(eig);
  auto repaired =
      vectors.matmul(torch::diag(values)).matmul(vectors.transpose(0, 1));
  repaired = 0.5 * (repaired + repaired.transpose(0, 1));
  auto diag = repaired.diagonal().clamp_min(epsilon).sqrt();
  repaired = repaired / (diag.unsqueeze(1) * diag.unsqueeze(0));
  repaired = 0.5 * (repaired + repaired.transpose(0, 1));
  repaired.diagonal().fill_(1.0);
  return repaired;
}

[[nodiscard]] inline torch::Tensor
shrink_and_repair_correlation(const torch::Tensor &empirical, double shrinkage,
                              double epsilon = 1.0e-6) {
  TORCH_CHECK(shrinkage >= 0.0 && shrinkage <= 1.0,
              "[covariance_coupler] shrinkage must be in [0,1]");
  auto repaired = repair_correlation_matrix(empirical, epsilon);
  auto eye = torch::eye(repaired.size(0), repaired.options());
  auto shrunk = (1.0 - shrinkage) * repaired + shrinkage * eye;
  return repair_correlation_matrix(shrunk, epsilon);
}

[[nodiscard]] inline torch::Tensor
sample_correlated_normals(const torch::Tensor &correlation,
                          std::int64_t sample_count,
                          double cholesky_jitter = 1.0e-8) {
  TORCH_CHECK(sample_count > 0,
              "[covariance_coupler] sample_count must be positive");
  auto corr = repair_correlation_matrix(correlation).to(torch::kFloat64);
  auto eye = torch::eye(corr.size(0), corr.options());
  auto chol = torch::linalg_cholesky(corr + cholesky_jitter * eye);
  auto z = torch::randn({sample_count, corr.size(0)}, corr.options());
  return z.matmul(chol.transpose(0, 1));
}

[[nodiscard]] inline torch::Tensor
mixture_quantile(const torch::Tensor &log_weight, const torch::Tensor &mu,
                 const torch::Tensor &sigma, const torch::Tensor &uniforms,
                 std::int64_t bisection_steps = 40) {
  TORCH_CHECK(log_weight.defined() && mu.defined() && sigma.defined(),
              "[covariance_coupler] mixture tensors must be defined");
  TORCH_CHECK(log_weight.dim() == 2 && mu.sizes() == log_weight.sizes() &&
                  sigma.sizes() == log_weight.sizes(),
              "[covariance_coupler] mixture tensors must be [A,K]");
  TORCH_CHECK(uniforms.defined() && uniforms.dim() == 2 &&
                  uniforms.size(1) == log_weight.size(0),
              "[covariance_coupler] uniforms must be [S,A]");
  TORCH_CHECK(bisection_steps > 0,
              "[covariance_coupler] bisection steps must be positive");

  auto lw = log_weight.to(torch::kFloat64);
  auto m = mu.to(torch::kFloat64);
  auto s = sigma.to(torch::kFloat64).clamp_min(1.0e-12);
  auto u = uniforms.to(torch::kFloat64).clamp(1.0e-8, 1.0 - 1.0e-8);
  auto pi = lw.exp();

  auto low = std::get<0>((m - 8.0 * s).min(/*dim=*/1))
                 .unsqueeze(0)
                 .expand_as(u)
                 .clone();
  auto high = std::get<0>((m + 8.0 * s).max(/*dim=*/1))
                  .unsqueeze(0)
                  .expand_as(u)
                  .clone();

  auto pi_b = pi.unsqueeze(0);
  auto m_b = m.unsqueeze(0);
  auto s_b = s.unsqueeze(0);
  for (std::int64_t step = 0; step < bisection_steps; ++step) {
    auto mid = 0.5 * (low + high);
    auto z = (mid.unsqueeze(-1) - m_b) / s_b;
    auto cdf = (pi_b * covariance_standard_normal_cdf(z)).sum(/*dim=*/-1);
    auto go_right = cdf < u;
    low = torch::where(go_right, mid, low);
    high = torch::where(go_right, high, mid);
  }
  return 0.5 * (low + high);
}

[[nodiscard]] inline torch::Tensor scenario_covariance(const torch::Tensor &x) {
  TORCH_CHECK(x.defined() && x.dim() == 2,
              "[covariance_coupler] scenarios must be [S,A]");
  const auto S = x.size(0);
  auto y = x.to(torch::kFloat64);
  auto centered = y - y.mean(/*dim=*/0, /*keepdim=*/true);
  const double denom = static_cast<double>(std::max<std::int64_t>(S - 1, 1));
  return centered.transpose(0, 1).matmul(centered) / denom;
}

[[nodiscard]] inline torch::Tensor
covariance_to_correlation(const torch::Tensor &covariance,
                          double epsilon = 1.0e-12) {
  TORCH_CHECK(covariance.defined() && covariance.dim() == 2 &&
                  covariance.size(0) == covariance.size(1),
              "[covariance_coupler] covariance must be [A,A]");
  auto cov = covariance.to(torch::kFloat64);
  auto scale = cov.diagonal().clamp_min(epsilon).sqrt();
  auto corr = cov / (scale.unsqueeze(1) * scale.unsqueeze(0));
  corr = 0.5 * (corr + corr.transpose(0, 1));
  corr.diagonal().fill_(1.0);
  return corr;
}

[[nodiscard]] inline coupled_samples_t sample_single_anchor_marginals(
    const torch::Tensor &log_weight, const torch::Tensor &mu,
    const torch::Tensor &sigma, std::int64_t anchor_slot,
    const std::vector<std::int64_t> &graph_indices,
    const torch::Tensor &empirical_correlation,
    const covariance_coupler_options_t &options = {}) {
  TORCH_CHECK(options.sample_count > 0,
              "[covariance_coupler] sample_count must be positive");
  TORCH_CHECK(!graph_indices.empty(),
              "[covariance_coupler] graph_indices must not be empty");
  TORCH_CHECK(log_weight.defined() && mu.defined() && sigma.defined(),
              "[covariance_coupler] marginal mixture tensors required");
  TORCH_CHECK(log_weight.dim() == 3 && mu.sizes() == log_weight.sizes() &&
                  sigma.sizes() == log_weight.sizes(),
              "[covariance_coupler] marginal mixtures must be [B,G,Kc]");
  TORCH_CHECK(anchor_slot >= 0 && anchor_slot < log_weight.size(0),
              "[covariance_coupler] anchor_slot out of range");
  auto index = torch::tensor(
      graph_indices,
      torch::TensorOptions().dtype(torch::kInt64).device(log_weight.device()));
  auto lw = log_weight.select(/*dim=*/0, anchor_slot).index_select(0, index);
  auto selected_mu = mu.select(/*dim=*/0, anchor_slot).index_select(0, index);
  auto selected_sigma =
      sigma.select(/*dim=*/0, anchor_slot).index_select(0, index);

  const auto M = static_cast<std::int64_t>(graph_indices.size());
  TORCH_CHECK(empirical_correlation.defined() &&
                  empirical_correlation.dim() == 2 &&
                  empirical_correlation.size(0) == M &&
                  empirical_correlation.size(1) == M,
              "[covariance_coupler] empirical_correlation must be [M,M]");
  auto corr = shrink_and_repair_correlation(
      empirical_correlation, options.shrinkage, options.repair_epsilon);
  auto normals = sample_correlated_normals(corr, options.sample_count,
                                           options.cholesky_jitter);
  auto uniforms =
      covariance_standard_normal_cdf(normals).clamp(1.0e-8, 1.0 - 1.0e-8);
  auto samples =
      mixture_quantile(lw.to(torch::kFloat64), selected_mu.to(torch::kFloat64),
                       selected_sigma.to(torch::kFloat64), uniforms,
                       options.quantile_bisection_steps);
  auto cov = scenario_covariance(samples);

  coupled_samples_t result{};
  result.samples = std::move(samples);
  result.covariance = std::move(cov);
  result.correlation = std::move(corr);
  result.uniforms = std::move(uniforms);
  return result;
}

} // namespace cuwacunu::wikimyei::observer
