// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <utility>

#include <torch/torch.h>

namespace cuwacunu::wikimyei::observer {

struct scenario_bank_t {
  torch::Tensor base_scenarios{};                // [S,A]
  torch::Tensor high_correlation_scenarios{};    // optional [S,A]
  torch::Tensor volatility_inflated_scenarios{}; // optional [S,A]
  torch::Tensor left_tail_shifted_scenarios{};   // optional [S,A]
  torch::Tensor tail_thickened_scenarios{};      // optional [S,A]
};

struct scenario_bank_options_t {
  double high_correlation_loading{0.90};
  double volatility_multiplier{1.50};
  double left_tail_shift{0.0};
  double left_tail_shift_volatility_multiplier{0.25};
  double left_tail_multiplier{1.50};
};

inline void validate_scenarios(const torch::Tensor &scenarios,
                               const char *name) {
  if (!scenarios.defined()) {
    return;
  }
  TORCH_CHECK(scenarios.dim() == 2, "[scenario_bank] ", name, " must be [S,A]");
  TORCH_CHECK(torch::isfinite(scenarios).all().template item<bool>(),
              "[scenario_bank] ", name, " contains non-finite values");
}

inline void validate_scenario_bank(const scenario_bank_t &bank) {
  TORCH_CHECK(bank.base_scenarios.defined(),
              "[scenario_bank] base_scenarios are required");
  validate_scenarios(bank.base_scenarios, "base_scenarios");
  validate_scenarios(bank.high_correlation_scenarios,
                     "high_correlation_scenarios");
  validate_scenarios(bank.volatility_inflated_scenarios,
                     "volatility_inflated_scenarios");
  validate_scenarios(bank.left_tail_shifted_scenarios,
                     "left_tail_shifted_scenarios");
  validate_scenarios(bank.tail_thickened_scenarios, "tail_thickened_scenarios");
  const auto base_sizes = bank.base_scenarios.sizes();
  auto require_same_shape = [&](const torch::Tensor &scenarios,
                                const char *name) {
    if (scenarios.defined()) {
      TORCH_CHECK(scenarios.sizes() == base_sizes, "[scenario_bank] ", name,
                  " must match base_scenarios shape");
    }
  };
  require_same_shape(bank.high_correlation_scenarios,
                     "high_correlation_scenarios");
  require_same_shape(bank.volatility_inflated_scenarios,
                     "volatility_inflated_scenarios");
  require_same_shape(bank.left_tail_shifted_scenarios,
                     "left_tail_shifted_scenarios");
  require_same_shape(bank.tail_thickened_scenarios, "tail_thickened_scenarios");
}

[[nodiscard]] inline scenario_bank_t make_base_bank(torch::Tensor scenarios) {
  scenario_bank_t out{};
  out.base_scenarios = std::move(scenarios);
  validate_scenario_bank(out);
  return out;
}

[[nodiscard]] inline torch::Tensor
left_tail_shift(const torch::Tensor &scenarios, double shift) {
  validate_scenarios(scenarios, "scenarios");
  TORCH_CHECK(shift >= 0.0, "[scenario_bank] left-tail shift must be >= 0");
  return scenarios - shift;
}

[[nodiscard]] inline torch::Tensor
left_tail_shift_by_asset(const torch::Tensor &scenarios,
                         const torch::Tensor &shift) {
  validate_scenarios(scenarios, "scenarios");
  TORCH_CHECK(shift.defined() && shift.dim() == 1 &&
                  shift.size(0) == scenarios.size(1),
              "[scenario_bank] asset shift must be [A]");
  auto s = shift.to(scenarios.options()).clamp_min(0.0);
  return scenarios - s.unsqueeze(0);
}

[[nodiscard]] inline torch::Tensor
inflate_volatility(const torch::Tensor &scenarios, double multiplier) {
  validate_scenarios(scenarios, "scenarios");
  TORCH_CHECK(multiplier >= 1.0,
              "[scenario_bank] volatility multiplier must be >= 1");
  auto mean = scenarios.mean(/*dim=*/0, /*keepdim=*/true);
  return mean + multiplier * (scenarios - mean);
}

[[nodiscard]] inline torch::Tensor
high_correlation_stress(const torch::Tensor &scenarios, double loading = 0.90) {
  validate_scenarios(scenarios, "scenarios");
  TORCH_CHECK(loading >= 0.0 && loading <= 1.0,
              "[scenario_bank] high-correlation loading must be in [0,1]");
  if (scenarios.size(1) <= 1) {
    return scenarios.clone();
  }
  auto x = scenarios.to(torch::kFloat64);
  auto mean = x.mean(/*dim=*/0, /*keepdim=*/true);
  auto centered = x - mean;
  auto variance = centered.pow(2).mean(/*dim=*/0, /*keepdim=*/true);
  auto scale = variance.clamp_min(1.0e-18).sqrt();
  auto z = centered / scale;
  auto factor = z.mean(/*dim=*/1, /*keepdim=*/true);
  factor = factor - factor.mean(/*dim=*/0, /*keepdim=*/true);
  auto factor_scale = factor.pow(2).mean().clamp_min(1.0e-18).sqrt();
  factor = factor / factor_scale;
  const double residual_loading =
      std::sqrt(std::max(0.0, 1.0 - loading * loading));
  auto stressed_z = loading * factor.expand_as(z) + residual_loading * z;
  return (mean + scale * stressed_z).to(scenarios.options());
}

[[nodiscard]] inline torch::Tensor
thicken_left_tail(const torch::Tensor &scenarios, double multiplier = 1.50) {
  validate_scenarios(scenarios, "scenarios");
  TORCH_CHECK(multiplier >= 1.0,
              "[scenario_bank] left-tail multiplier must be >= 1");
  auto mean = scenarios.mean(/*dim=*/0, /*keepdim=*/true);
  auto centered = scenarios - mean;
  auto stressed_centered =
      torch::where(centered < 0.0, centered * multiplier, centered);
  return mean + stressed_centered;
}

[[nodiscard]] inline scenario_bank_t
make_stress_bank(const torch::Tensor &base_scenarios,
                 const scenario_bank_options_t &options = {}) {
  validate_scenarios(base_scenarios, "base_scenarios");
  TORCH_CHECK(options.volatility_multiplier >= 1.0,
              "[scenario_bank] volatility multiplier must be >= 1");
  TORCH_CHECK(options.left_tail_shift >= 0.0,
              "[scenario_bank] left-tail shift must be >= 0");
  TORCH_CHECK(options.left_tail_shift_volatility_multiplier >= 0.0,
              "[scenario_bank] left-tail shift volatility multiplier must be "
              ">= 0");

  auto x = base_scenarios.to(torch::kFloat64);
  auto mean = x.mean(/*dim=*/0, /*keepdim=*/true);
  auto centered = x - mean;
  auto vol = centered.pow(2).mean(/*dim=*/0).clamp_min(0.0).sqrt();
  auto shift = torch::full({base_scenarios.size(1)}, options.left_tail_shift,
                           x.options()) +
               options.left_tail_shift_volatility_multiplier * vol;

  scenario_bank_t bank{};
  bank.base_scenarios = base_scenarios.clone();
  bank.high_correlation_scenarios =
      high_correlation_stress(base_scenarios, options.high_correlation_loading);
  bank.volatility_inflated_scenarios =
      inflate_volatility(base_scenarios, options.volatility_multiplier);
  bank.left_tail_shifted_scenarios =
      left_tail_shift_by_asset(base_scenarios, shift);
  bank.tail_thickened_scenarios =
      thicken_left_tail(base_scenarios, options.left_tail_multiplier);
  validate_scenario_bank(bank);
  return bank;
}

} // namespace cuwacunu::wikimyei::observer
