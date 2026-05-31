// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cmath>
#include <exception>
#include <string>

#include <torch/torch.h>

#include "wikimyei/inference/expected_value/mdn/mixture_density_network_types.h"
#include "wikimyei/observer/belief/types.h"

namespace cuwacunu::wikimyei::observer {

struct data_quality_result_t {
  bool valid{true};
  belief::BeliefDiagnostics diagnostics{};
};

inline void add_failure(data_quality_result_t *result, std::string message) {
  result->valid = false;
  result->diagnostics.failures.push_back(std::move(message));
}

inline void require_finite(data_quality_result_t *result,
                           const torch::Tensor &tensor, const char *name) {
  if (!tensor.defined()) {
    add_failure(result, std::string(name) + " is undefined");
    return;
  }
  if (!torch::isfinite(tensor).all().template item<bool>()) {
    add_failure(result, std::string(name) + " contains non-finite values");
  }
}

inline void require_finite_if_defined(data_quality_result_t *result,
                                      const torch::Tensor &tensor,
                                      const char *name) {
  if (!tensor.defined()) {
    return;
  }
  require_finite(result, tensor, name);
}

inline void require_range_if_defined(data_quality_result_t *result,
                                     const torch::Tensor &tensor,
                                     const char *name, double low,
                                     double high) {
  if (!tensor.defined()) {
    return;
  }
  require_finite(result, tensor, name);
  if (!result->valid) {
    return;
  }
  auto x = tensor.to(torch::kFloat64);
  if (x.lt(low).any().template item<bool>() ||
      x.gt(high).any().template item<bool>()) {
    add_failure(result, std::string(name) + " outside allowed range");
  }
}

inline void require_nonnegative_if_defined(data_quality_result_t *result,
                                           const torch::Tensor &tensor,
                                           const char *name) {
  if (!tensor.defined()) {
    return;
  }
  require_finite(result, tensor, name);
  if (!result->valid) {
    return;
  }
  if (tensor.to(torch::kFloat64).lt(0.0).any().template item<bool>()) {
    add_failure(result, std::string(name) + " contains negative values");
  }
}

inline void require_symmetric_matrix(data_quality_result_t *result,
                                     const torch::Tensor &matrix,
                                     const char *name, double tolerance) {
  require_finite(result, matrix, name);
  if (!result->valid) {
    return;
  }
  auto m = matrix.to(torch::kFloat64);
  auto err = (m - m.transpose(0, 1)).abs().max().item<double>();
  if (err > tolerance) {
    add_failure(result, std::string(name) + " is not symmetric");
  }
}

inline void require_covariance_matrix(data_quality_result_t *result,
                                      const torch::Tensor &covariance,
                                      double symmetry_tolerance = 1.0e-8,
                                      double psd_tolerance = 1.0e-8) {
  require_symmetric_matrix(result, covariance, "covariance",
                           symmetry_tolerance);
  if (!result->valid) {
    return;
  }
  auto cov = covariance.to(torch::kFloat64);
  if (cov.diagonal().lt(-psd_tolerance).any().template item<bool>()) {
    add_failure(result, "covariance diagonal contains negative variance");
    return;
  }
  auto min_eigenvalue = torch::linalg_eigvalsh(cov).min().item<double>();
  if (min_eigenvalue < -psd_tolerance) {
    add_failure(result, "covariance is not positive semidefinite");
  }
}

inline void require_correlation_matrix(data_quality_result_t *result,
                                       const torch::Tensor &correlation,
                                       double symmetry_tolerance = 1.0e-8,
                                       double diagonal_tolerance = 1.0e-8) {
  require_symmetric_matrix(result, correlation, "correlation",
                           symmetry_tolerance);
  if (!result->valid) {
    return;
  }
  auto corr = correlation.to(torch::kFloat64);
  auto diag_err = (corr.diagonal() - 1.0).abs().max().item<double>();
  if (diag_err > diagonal_tolerance) {
    add_failure(result, "correlation diagonal is not one");
    return;
  }
  if (corr.lt(-1.0 - diagonal_tolerance).any().template item<bool>() ||
      corr.gt(1.0 + diagonal_tolerance).any().template item<bool>()) {
    add_failure(result, "correlation contains entries outside [-1,1]");
  }
}

[[nodiscard]] inline data_quality_result_t check_mdn_output(
    const cuwacunu::wikimyei::inference::expected_value::mdn::MdnOut &out,
    double sigma_floor_warning = 1.0e-6, double log_pi_tol = 1.0e-5) {
  data_quality_result_t result{};
  if (!out.log_pi.defined() || !out.mu.defined() || !out.sigma.defined()) {
    add_failure(&result, "MDN output tensors are undefined");
    return result;
  }
  if (out.log_pi.dim() != 5 || out.mu.sizes() != out.log_pi.sizes() ||
      out.sigma.sizes() != out.log_pi.sizes()) {
    add_failure(&result, "MDN output shape mismatch");
    return result;
  }
  require_finite(&result, out.log_pi, "log_pi");
  require_finite(&result, out.mu, "mu");
  require_finite(&result, out.sigma, "sigma");
  if (result.valid) {
    auto sum = out.log_pi.exp().sum(/*dim=*/-1);
    auto err = (sum - 1.0).abs().max().item<double>();
    if (err > log_pi_tol) {
      add_failure(&result, "log_pi does not normalize over K");
    }
    if (out.sigma.le(0.0).any().template item<bool>()) {
      add_failure(&result, "sigma contains non-positive values");
    }
    if (out.sigma.le(sigma_floor_warning).all().template item<bool>()) {
      result.diagnostics.warnings.push_back(
          "all sigma values are pinned near floor");
    }
  }
  return result;
}

[[nodiscard]] inline data_quality_result_t
check_allocation_belief(const belief::AllocationBelief &state) {
  data_quality_result_t result{};
  try {
    belief::validate_allocation_belief_contract(
        state, /*require_portfolio_fields=*/true);
  } catch (const std::exception &ex) {
    add_failure(&result, ex.what());
    return result;
  }
  require_finite(&result, state.expected_log_return, "expected_log_return");
  require_finite(&result, state.expected_arithmetic_return,
                 "expected_arithmetic_return");
  require_covariance_matrix(&result, state.covariance);
  require_correlation_matrix(&result, state.correlation);
  require_finite(&result, state.scenarios, "scenarios");
  if (result.valid && state.scenarios.to(torch::kFloat64)
                          .le(-1.0)
                          .any()
                          .template item<bool>()) {
    add_failure(&result,
                "arithmetic-return scenarios contain returns <= -100%");
  }
  require_range_if_defined(&result, state.confidence, "confidence", 0.0, 1.0);
  require_nonnegative_if_defined(&result, state.marginal_variance,
                                 "marginal_variance");
  require_nonnegative_if_defined(&result, state.marginal_volatility,
                                 "marginal_volatility");
  require_nonnegative_if_defined(&result, state.volatility, "volatility");
  require_nonnegative_if_defined(&result, state.mixture_entropy,
                                 "mixture_entropy");
  require_nonnegative_if_defined(&result, state.component_disagreement,
                                 "component_disagreement");
  require_nonnegative_if_defined(&result, state.channel_disagreement,
                                 "channel_disagreement");
  require_nonnegative_if_defined(&result, state.surprise, "surprise");
  require_range_if_defined(&result, state.calibration_score,
                           "calibration_score", 0.0, 1.0);
  require_range_if_defined(&result, state.residual_quality_score,
                           "residual_quality_score", 0.0, 1.0);
  require_range_if_defined(&result, state.projection_validation_score,
                           "projection_validation_score", 0.0, 1.0);
  require_range_if_defined(&result, state.liquidity_score, "liquidity_score",
                           0.0, 1.0);
  require_nonnegative_if_defined(&result, state.linear_cost, "linear_cost");
  require_nonnegative_if_defined(&result, state.quadratic_impact,
                                 "quadratic_impact");
  require_range_if_defined(&result, state.capacity_weight_limit,
                           "capacity_weight_limit", 0.0, 1.0);
  if (result.valid && state.scenarios.size(0) <= 0) {
    add_failure(&result, "scenario matrix is empty");
  }
  if (result.valid && state.tradable_mask.defined() &&
      state.tradable_mask.to(torch::kBool).sum().item<std::int64_t>() <= 0) {
    add_failure(&result, "tradable universe is empty");
  }
  return result;
}

} // namespace cuwacunu::wikimyei::observer
