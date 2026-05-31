// SPDX-License-Identifier: MIT
#pragma once

#include <algorithm>
#include <cstdint>
#include <string>
#include <utility>

#include <torch/torch.h>

#include "wikimyei/observer/belief/types.h"
#include "wikimyei/observer/utility/calibration.h"
#include "wikimyei/observer/utility/surprise.h"

namespace cuwacunu::wikimyei::observer::belief {

struct allocation_belief_health_update_options_t {
  surprise_options_t surprise_options{};
  double confidence_divisor_floor{1.0e-12};
};

namespace detail {

inline void require_health_vector(const torch::Tensor &tensor, std::int64_t A,
                                  const char *name) {
  TORCH_CHECK(tensor.defined() && tensor.dim() == 1 && tensor.size(0) == A,
              "[AllocationBeliefHealthUpdate] ", name, " must be [A]");
}

[[nodiscard]] inline torch::Tensor
bool_vector_or_true_for_health(const torch::Tensor &tensor, std::int64_t A,
                               const torch::Device &device, const char *name) {
  if (!tensor.defined()) {
    return torch::ones(
        {A}, torch::TensorOptions().dtype(torch::kBool).device(device));
  }
  TORCH_CHECK(tensor.dim() == 1 && tensor.size(0) == A,
              "[AllocationBeliefHealthUpdate] ", name, " must be [A]");
  return tensor.to(torch::TensorOptions().dtype(torch::kBool).device(device));
}

[[nodiscard]] inline torch::Tensor
score_from_surprise_nll(const torch::Tensor &nll,
                        const surprise_options_t &options) {
  TORCH_CHECK(
      options.score_scale > 0.0,
      "[AllocationBeliefHealthUpdate] surprise score_scale must be > 0");
  auto x = nll.to(torch::kFloat64);
  auto excess = (x - options.score_soft_nll).clamp_min(0.0);
  return torch::exp(-excess / options.score_scale).clamp(0.0, 1.0);
}

[[nodiscard]] inline torch::Tensor
optional_score_or_ones(const torch::Tensor &score, std::int64_t A,
                       torch::TensorOptions options, const char *name) {
  if (!score.defined()) {
    return torch::ones({A}, options);
  }
  require_health_vector(score, A, name);
  return score.to(options).clamp(0.0, 1.0);
}

[[nodiscard]] inline torch::Tensor
active_mask_for_health(const AllocationBelief &state, std::int64_t A,
                       const torch::Device &device) {
  auto active =
      bool_vector_or_true_for_health(state.valid_mask, A, device, "valid_mask");
  if (state.tradable_mask.defined()) {
    active = active.logical_and(bool_vector_or_true_for_health(
        state.tradable_mask, A, device, "tradable_mask"));
  }
  return active;
}

} // namespace detail

[[nodiscard]] inline AllocationBelief
apply_surprise(AllocationBelief state, const surprise_t &surprise_result,
               const allocation_belief_health_update_options_t &options = {}) {
  validate_allocation_belief_contract(state);
  const auto A = asset_count(state);
  TORCH_CHECK(
      options.confidence_divisor_floor > 0.0,
      "[AllocationBeliefHealthUpdate] confidence_divisor_floor must be > 0");
  detail::require_health_vector(surprise_result.surprise, A,
                                "surprise.surprise");
  detail::require_health_vector(surprise_result.score, A, "surprise.score");
  detail::require_health_vector(surprise_result.valid_mask, A,
                                "surprise.valid_mask");

  auto tensor_options = state.confidence.options().dtype(torch::kFloat64);
  auto device = state.confidence.device();
  auto active = detail::active_mask_for_health(state, A, device);
  auto old_surprise_score =
      state.surprise.defined()
          ? detail::score_from_surprise_nll(state.surprise.to(tensor_options),
                                            options.surprise_options)
          : torch::ones({A}, tensor_options);
  auto new_surprise_score = surprise_result.score.to(tensor_options);
  auto observed = surprise_result.valid_mask.to(
      torch::TensorOptions().dtype(torch::kBool).device(device));
  new_surprise_score = torch::where(observed, new_surprise_score,
                                    torch::ones_like(new_surprise_score));

  auto confidence = state.confidence.to(tensor_options).clamp(0.0, 1.0);
  confidence = confidence /
               old_surprise_score.clamp_min(options.confidence_divisor_floor);
  confidence = (confidence * new_surprise_score).clamp(0.0, 1.0);
  confidence = confidence.masked_fill(active.logical_not(), 0.0);

  state.surprise = surprise_result.surprise.to(tensor_options);
  state.confidence = confidence.to(state.confidence.options());
  state.diagnostics.notes.push_back("wikimyei.observer.belief.apply_surprise");
  validate_allocation_belief_contract(state);
  return state;
}

[[nodiscard]] inline AllocationBelief apply_calibration(
    AllocationBelief state, const calibration_summary_t &summary,
    const allocation_belief_health_update_options_t &options = {}) {
  validate_allocation_belief_contract(state);
  const auto A = asset_count(state);
  TORCH_CHECK(
      options.confidence_divisor_floor > 0.0,
      "[AllocationBeliefHealthUpdate] confidence_divisor_floor must be > 0");
  detail::require_health_vector(summary.score, A, "calibration.score");

  auto tensor_options = state.confidence.options().dtype(torch::kFloat64);
  auto device = state.confidence.device();
  auto active = detail::active_mask_for_health(state, A, device);
  auto old_calibration_score = detail::optional_score_or_ones(
      state.calibration_score, A, tensor_options, "calibration_score");
  auto new_calibration_score = summary.score.to(tensor_options).clamp(0.0, 1.0);

  auto confidence = state.confidence.to(tensor_options).clamp(0.0, 1.0);
  confidence = confidence / old_calibration_score.clamp_min(
                                options.confidence_divisor_floor);
  confidence = (confidence * new_calibration_score).clamp(0.0, 1.0);
  confidence = confidence.masked_fill(active.logical_not(), 0.0);

  state.calibration_score = new_calibration_score.to(
      state.confidence.options().dtype(torch::kFloat64));
  state.confidence = confidence.to(state.confidence.options());
  state.diagnostics.notes.push_back(
      "wikimyei.observer.belief.apply_calibration");
  validate_allocation_belief_contract(state);
  return state;
}

[[nodiscard]] inline AllocationBelief apply_surprise_and_calibration(
    AllocationBelief state, const surprise_t &surprise_result,
    const calibration_summary_t &summary,
    const allocation_belief_health_update_options_t &options = {}) {
  state = apply_surprise(std::move(state), surprise_result, options);
  state = apply_calibration(std::move(state), summary, options);
  return state;
}

} // namespace cuwacunu::wikimyei::observer::belief
