// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>

#include <torch/torch.h>

#include "wikimyei/observer/belief/types.h"

namespace cuwacunu::wikimyei::observer {

struct projection_validation_options_t {
  double interval_alpha{0.90};
  double warning_mae{std::numeric_limits<double>::infinity()};
  double warning_abs_bias{std::numeric_limits<double>::infinity()};
  double warning_min_directional_accuracy{0.0};
  double eps{1.0e-12};
};

struct projection_validation_t {
  bool available{false};
  bool valid{false};

  torch::Tensor predicted_log_return{}; // [A]
  torch::Tensor realized_log_return{};  // [A]
  torch::Tensor active_mask{};          // [A], bool
  torch::Tensor error{};                // [A], predicted - realized
  torch::Tensor abs_error{};            // [A]
  torch::Tensor squared_error{};        // [A]
  torch::Tensor zero_baseline_error{};  // [A], 0 - realized
  torch::Tensor zero_baseline_abs_error{};
  torch::Tensor zero_baseline_squared_error{};
  torch::Tensor score{};          // [A], higher is better
  torch::Tensor interval_hit{};   // optional [A], bool
  torch::Tensor interval_width{}; // optional [A]

  double signed_bias{0.0};
  double mae{0.0};
  double rmse{0.0};
  double directional_accuracy{0.0};
  double correlation{0.0};
  double interval_coverage{0.0};
  double mean_interval_width{0.0};
  double zero_baseline_signed_bias{0.0};
  double zero_baseline_mae{0.0};
  double zero_baseline_rmse{0.0};
  double zero_baseline_directional_accuracy{0.0};
  double model_skill_vs_zero_mae{0.0};
  double model_skill_vs_zero_rmse{0.0};

  belief::BeliefDiagnostics diagnostics{};
};

namespace projection_validation_detail {

[[nodiscard]] inline torch::Tensor bool_mask_or_true(const torch::Tensor &mask,
                                                     std::int64_t A,
                                                     torch::Device device) {
  if (!mask.defined()) {
    return torch::ones(
        {A}, torch::TensorOptions().dtype(torch::kBool).device(device));
  }
  TORCH_CHECK(mask.dim() == 1 && mask.size(0) == A,
              "[projection_validation] active_mask must be [A]");
  TORCH_CHECK(mask.scalar_type() == torch::kBool,
              "[projection_validation] active_mask must be bool");
  return mask.to(torch::TensorOptions().dtype(torch::kBool).device(device));
}

[[nodiscard]] inline double masked_mean_scalar(const torch::Tensor &values,
                                               const torch::Tensor &mask) {
  auto selected = values.masked_select(mask);
  if (selected.numel() == 0) {
    return 0.0;
  }
  return selected.mean().item<double>();
}

[[nodiscard]] inline double masked_correlation(const torch::Tensor &x,
                                               const torch::Tensor &y,
                                               const torch::Tensor &mask,
                                               double eps) {
  auto xs = x.masked_select(mask);
  auto ys = y.masked_select(mask);
  if (xs.numel() < 2) {
    return 0.0;
  }
  auto xc = xs - xs.mean();
  auto yc = ys - ys.mean();
  const double denom =
      std::sqrt((xc.pow(2).sum() * yc.pow(2).sum()).item<double>());
  if (denom <= eps) {
    return 0.0;
  }
  return (xc * yc).sum().item<double>() / denom;
}

} // namespace projection_validation_detail

[[nodiscard]] inline projection_validation_t validate_projected_log_return(
    const torch::Tensor &projected_log_return_scenarios,
    const torch::Tensor &realized_log_return,
    const torch::Tensor &active_mask = torch::Tensor(),
    const projection_validation_options_t &options = {}) {
  TORCH_CHECK(
      projected_log_return_scenarios.defined(),
      "[projection_validation] projected_log_return_scenarios required");
  TORCH_CHECK(realized_log_return.defined(),
              "[projection_validation] realized_log_return required");
  TORCH_CHECK(
      projected_log_return_scenarios.dim() == 2,
      "[projection_validation] projected_log_return_scenarios must be [S,A]");
  TORCH_CHECK(realized_log_return.dim() == 1,
              "[projection_validation] realized_log_return must be [A]");
  TORCH_CHECK(projected_log_return_scenarios.size(1) ==
                  realized_log_return.size(0),
              "[projection_validation] scenario asset count mismatch");
  TORCH_CHECK(options.interval_alpha > 0.0 && options.interval_alpha < 1.0,
              "[projection_validation] interval_alpha must be in (0,1)");
  TORCH_CHECK(options.eps > 0.0,
              "[projection_validation] eps must be positive");

  const auto A = realized_log_return.size(0);
  const auto S = projected_log_return_scenarios.size(0);
  auto scenarios = projected_log_return_scenarios.to(torch::kFloat64);
  auto realized =
      realized_log_return.to(torch::kFloat64).to(scenarios.device());
  auto finite = torch::isfinite(realized).logical_and(
      torch::isfinite(scenarios).all(/*dim=*/0));
  auto active = projection_validation_detail::bool_mask_or_true(
                    active_mask, A, scenarios.device())
                    .logical_and(finite);

  projection_validation_t out{};
  out.available = true;
  out.predicted_log_return = scenarios.mean(/*dim=*/0);
  out.realized_log_return = realized;
  out.active_mask = active;
  out.error = out.predicted_log_return - realized;
  out.abs_error = out.error.abs();
  out.squared_error = out.error.pow(2);
  out.zero_baseline_error = -realized;
  out.zero_baseline_abs_error = realized.abs();
  out.zero_baseline_squared_error = realized.pow(2);
  out.score = 1.0 / (1.0 + out.abs_error);

  if (active.sum().item<std::int64_t>() <= 0) {
    out.diagnostics.failures.push_back(
        "projection_validation.no_active_assets");
    return out;
  }

  out.signed_bias =
      projection_validation_detail::masked_mean_scalar(out.error, active);
  out.mae =
      projection_validation_detail::masked_mean_scalar(out.abs_error, active);
  out.rmse = std::sqrt(projection_validation_detail::masked_mean_scalar(
      out.error.pow(2), active));
  out.zero_baseline_signed_bias =
      projection_validation_detail::masked_mean_scalar(out.zero_baseline_error,
                                                       active);
  out.zero_baseline_mae = projection_validation_detail::masked_mean_scalar(
      out.zero_baseline_abs_error, active);
  out.zero_baseline_rmse =
      std::sqrt(projection_validation_detail::masked_mean_scalar(
          out.zero_baseline_squared_error, active));
  if (out.zero_baseline_mae > options.eps) {
    out.model_skill_vs_zero_mae = 1.0 - (out.mae / out.zero_baseline_mae);
  }
  if (out.zero_baseline_rmse > options.eps) {
    out.model_skill_vs_zero_rmse = 1.0 - (out.rmse / out.zero_baseline_rmse);
  }

  auto predicted_sign = out.predicted_log_return.sign();
  auto realized_sign = realized.sign();
  out.directional_accuracy = projection_validation_detail::masked_mean_scalar(
      predicted_sign.eq(realized_sign).to(torch::kFloat64), active);
  out.zero_baseline_directional_accuracy =
      projection_validation_detail::masked_mean_scalar(
          torch::zeros_like(realized_sign)
              .eq(realized_sign)
              .to(torch::kFloat64),
          active);
  out.correlation = projection_validation_detail::masked_correlation(
      out.predicted_log_return, realized, active, options.eps);

  auto sorted = std::get<0>(scenarios.sort(/*dim=*/0));
  const double tail = (1.0 - options.interval_alpha) * 0.5;
  const auto low_index = std::clamp<std::int64_t>(
      static_cast<std::int64_t>(std::floor(tail * static_cast<double>(S))), 0,
      std::max<std::int64_t>(S - 1, 0));
  const auto high_index =
      std::clamp<std::int64_t>(static_cast<std::int64_t>(std::ceil(
                                   (1.0 - tail) * static_cast<double>(S))) -
                                   1,
                               0, std::max<std::int64_t>(S - 1, 0));
  auto lower = sorted.select(/*dim=*/0, low_index);
  auto upper = sorted.select(/*dim=*/0, high_index);
  out.interval_hit = realized.ge(lower).logical_and(realized.le(upper));
  out.interval_width = upper - lower;
  out.interval_coverage = projection_validation_detail::masked_mean_scalar(
      out.interval_hit.to(torch::kFloat64), active);
  out.mean_interval_width = projection_validation_detail::masked_mean_scalar(
      out.interval_width, active);

  if (out.mae > options.warning_mae + options.eps) {
    out.diagnostics.warnings.push_back(
        "projection_validation.mae_above_warning");
  }
  if (std::abs(out.signed_bias) > options.warning_abs_bias + options.eps) {
    out.diagnostics.warnings.push_back(
        "projection_validation.abs_bias_above_warning");
  }
  if (out.directional_accuracy + options.eps <
      options.warning_min_directional_accuracy) {
    out.diagnostics.warnings.push_back(
        "projection_validation.directional_accuracy_below_warning");
  }

  out.valid = out.diagnostics.failures.empty();
  return out;
}

} // namespace cuwacunu::wikimyei::observer
