// SPDX-License-Identifier: MIT
#pragma once

#include <cmath>
#include <limits>

#include <torch/torch.h>

#include "wikimyei/observer/belief/types.h"

namespace cuwacunu::wikimyei::observer {

struct nodelift_residual_quality_options_t {
  double warning_mean_residual_energy{0.10};
  double failure_mean_residual_energy{std::numeric_limits<double>::infinity()};
  double eps{1.0e-12};
};

struct nodelift_residual_quality_t {
  bool available{false};
  bool valid{false};

  double finite_fraction{0.0};
  double mean_residual_energy{0.0};
  double max_residual_energy{0.0};

  torch::Tensor mean_residual_energy_by_coord{};   // [D]
  torch::Tensor residual_quality_score_by_coord{}; // [D], higher is better

  belief::BeliefDiagnostics diagnostics{};
};

[[nodiscard]] inline nodelift_residual_quality_t
compute_nodelift_residual_quality(
    const torch::Tensor &residual_energy,
    const torch::Tensor &residual_mask = torch::Tensor(),
    const nodelift_residual_quality_options_t &options = {}) {
  nodelift_residual_quality_t out{};
  if (!residual_energy.defined()) {
    out.diagnostics.warnings.push_back("nodelift_residual_quality.unavailable");
    return out;
  }
  TORCH_CHECK(
      residual_energy.dim() >= 1,
      "[nodelift_residual_quality] residual_energy must have rank >= 1");
  TORCH_CHECK(options.eps > 0.0,
              "[nodelift_residual_quality] eps must be positive");

  auto energy = residual_energy.to(torch::kFloat64).clamp_min(0.0);
  auto finite = torch::isfinite(energy);
  if (residual_mask.defined()) {
    TORCH_CHECK(residual_mask.sizes() == residual_energy.sizes(),
                "[nodelift_residual_quality] residual_mask shape mismatch");
    TORCH_CHECK(residual_mask.scalar_type() == torch::kBool,
                "[nodelift_residual_quality] residual_mask must be bool");
    finite = finite.logical_and(residual_mask.to(torch::kBool));
  }

  const double total_count = static_cast<double>(energy.numel());
  const double finite_count = finite.sum().item<double>();
  out.available = true;
  out.finite_fraction = total_count > 0.0 ? finite_count / total_count : 0.0;
  if (finite_count <= 0.0) {
    out.diagnostics.failures.push_back(
        "nodelift_residual_quality.no_finite_residual_energy");
    return out;
  }

  auto values = energy.masked_select(finite);
  out.mean_residual_energy = values.mean().item<double>();
  out.max_residual_energy = values.max().item<double>();

  const auto coord_count = energy.size(-1);
  auto flat_energy = energy.reshape({-1, coord_count});
  auto flat_mask = finite.reshape({-1, coord_count});
  auto coord_count_tensor = flat_mask.to(torch::kFloat64).sum(/*dim=*/0);
  auto coord_sum =
      flat_energy.masked_fill(flat_mask.logical_not(), 0.0).sum(/*dim=*/0);
  out.mean_residual_energy_by_coord =
      coord_sum / coord_count_tensor.clamp_min(1.0);
  out.residual_quality_score_by_coord =
      1.0 / (1.0 + out.mean_residual_energy_by_coord.clamp_min(0.0));

  if (out.mean_residual_energy >
      options.warning_mean_residual_energy + options.eps) {
    out.diagnostics.warnings.push_back(
        "nodelift_residual_quality.mean_residual_energy_above_warning");
  }
  if (out.mean_residual_energy >
      options.failure_mean_residual_energy + options.eps) {
    out.diagnostics.failures.push_back(
        "nodelift_residual_quality.mean_residual_energy_above_failure");
  }
  out.valid = out.diagnostics.failures.empty();
  return out;
}

} // namespace cuwacunu::wikimyei::observer
