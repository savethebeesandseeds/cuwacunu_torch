// Development-only frozen-affine-base plus zero-output residual evaluator.
//
// The byte-frozen warm-start evaluator is included in a named implementation
// namespace so this evaluator reuses its canonical parser, Phase 2A affine
// construction, direct-float32 conversion, schedule, and metric vocabulary.
#include <algorithm>
#include <array>
#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

#include <fcntl.h>
#include <unistd.h>

#include <ATen/Context.h>
#include <ATen/Parallel.h>
#include <ATen/ops/cholesky_solve.h>
#include <ATen/ops/linalg_cholesky_ex.h>
#include <torch/torch.h>

namespace residual_frozen_warm_start {
#include "frozen_representation_affine_warm_start_stability_probe.cpp"
} // namespace residual_frozen_warm_start

namespace {

using namespace residual_frozen_warm_start;

constexpr std::string_view kResidualSchema =
    "synthetic_v2_frozen_direct_float32_affine_base_zero_output_residual_"
    "development_v1";
constexpr std::string_view kExpectedBatchScheduleFingerprint =
    "f2fa41d284a42d60";
constexpr double kResidualScale = 1.0;
constexpr double kTrainPreservationAggregateMseRatioLimit = 1.05;
constexpr double kTrainMaterialBenefitAggregateMseRatioLimit = 0.90;
constexpr double kValidationPreservationAggregateMseRatioLimit = 1.05;
constexpr double kHeadMseRatioLimit = 1.10;
constexpr double kMetricDeficitLimit = 0.01;
constexpr double kRmseRatioIncreaseLimit = 0.05;
constexpr double kClearStopAggregateMseRatio = 1.25;
constexpr double kClearStopMaximumHeadMseRatio = 1.50;
constexpr double kStrongDirectionThreshold = 0.95;
constexpr double kStrongRankThreshold = 0.95;
constexpr double kStrongCorrelationThreshold = 0.95;
constexpr double kStrongRmseRatioLimit = 0.25;
constexpr int64_t kBaseTensorCount = 2;
constexpr int64_t kBaseTensorElementCount = 873;
constexpr int64_t kResidualParameterTensorCount = 6;
constexpr int64_t kResidualParameterElementCount = 30089;
constexpr int64_t kZeroOutputParameterTensorCount = 2;
constexpr int64_t kZeroOutputParameterElementCount = 1161;
constexpr int64_t kDefaultHiddenParameterTensorCount = 4;
constexpr int64_t kDefaultHiddenParameterElementCount = 28928;

struct ResidualTrainingSummary {
  double initial_full_train_standardized_mse{0.0};
  double final_full_train_standardized_mse{0.0};
  double last_minibatch_loss{0.0};
  double maximum_preclip_gradient_norm{0.0};
  double first_backward_upstream_gradient_norm{0.0};
  double first_backward_output_weight_gradient_norm{0.0};
  double first_step_output_weight_update_max_abs{0.0};
  double second_backward_upstream_gradient_norm{0.0};
  double final_upstream_parameter_delta_l2{0.0};
  int64_t clipped_step_count{0};
  int64_t optimizer_steps_completed{0};
  int64_t base_per_step_byte_invariance_check_count{0};
};

class ZeroOutputResidualMlpImpl : public torch::nn::Module {
public:
  ZeroOutputResidualMlpImpl() {
    input_ = register_module("input",
                             torch::nn::Linear(kFeatureWidth, kHiddenWidth));
    hidden_ = register_module("hidden",
                              torch::nn::Linear(kHiddenWidth, kHiddenWidth));
    output_ =
        register_module("output", torch::nn::Linear(kHiddenWidth, kHeadCount));
  }

  void zero_output_layer() {
    torch::NoGradGuard no_grad;
    output_->weight.zero_();
    output_->bias.zero_();
  }

  torch::Tensor forward(const torch::Tensor &features,
                        const torch::Tensor &heads) {
    const auto all_heads = output_->forward(
        torch::gelu(hidden_->forward(torch::gelu(input_->forward(features)))));
    return all_heads.gather(1, heads.unsqueeze(1)).squeeze(1);
  }

  std::array<torch::Tensor, kDefaultHiddenParameterTensorCount>
  upstream_parameters() const {
    return {input_->weight, input_->bias, hidden_->weight, hidden_->bias};
  }

  std::array<torch::Tensor, kZeroOutputParameterTensorCount>
  output_parameters() const {
    return {output_->weight, output_->bias};
  }

  torch::Tensor output_weight() const { return output_->weight; }

private:
  torch::nn::Linear input_{nullptr};
  torch::nn::Linear hidden_{nullptr};
  torch::nn::Linear output_{nullptr};
};
TORCH_MODULE(ZeroOutputResidualMlp);

std::size_t tensor_byte_count(const torch::Tensor &tensor) {
  if (!tensor.defined() || !tensor.device().is_cpu() ||
      !tensor.is_contiguous() || tensor.numel() < 0) {
    local_fail("byte comparison requires a defined contiguous CPU tensor");
  }
  const auto elements = static_cast<std::uint64_t>(tensor.numel());
  const auto element_size = static_cast<std::uint64_t>(tensor.element_size());
  if (element_size == 0 ||
      elements > std::numeric_limits<std::size_t>::max() / element_size) {
    local_fail("tensor byte count overflow");
  }
  return static_cast<std::size_t>(elements * element_size);
}

bool tensor_bytes_equal(const torch::Tensor &lhs, const torch::Tensor &rhs) {
  if (!lhs.defined() || !rhs.defined() || lhs.sizes() != rhs.sizes() ||
      lhs.scalar_type() != rhs.scalar_type() ||
      lhs.device() != rhs.device() || !lhs.device().is_cpu() ||
      !lhs.is_contiguous() || !rhs.is_contiguous()) {
    return false;
  }
  const auto bytes = tensor_byte_count(lhs);
  return bytes == tensor_byte_count(rhs) &&
         std::memcmp(lhs.data_ptr(), rhs.data_ptr(), bytes) == 0;
}

bool tensor_bytes_all_zero(const torch::Tensor &tensor) {
  const auto bytes = tensor_byte_count(tensor);
  const auto *data = static_cast<const unsigned char *>(tensor.data_ptr());
  for (std::size_t index = 0; index < bytes; ++index) {
    if (data[index] != 0U) {
      return false;
    }
  }
  return true;
}

bool storage_ranges_overlap(const torch::Tensor &lhs,
                            const torch::Tensor &rhs) {
  const auto lhs_begin =
      reinterpret_cast<std::uintptr_t>(lhs.data_ptr());
  const auto rhs_begin =
      reinterpret_cast<std::uintptr_t>(rhs.data_ptr());
  const auto lhs_bytes = tensor_byte_count(lhs);
  const auto rhs_bytes = tensor_byte_count(rhs);
  if (lhs_begin > std::numeric_limits<std::uintptr_t>::max() - lhs_bytes ||
      rhs_begin > std::numeric_limits<std::uintptr_t>::max() - rhs_bytes) {
    local_fail("tensor storage range overflow");
  }
  const auto lhs_end = lhs_begin + lhs_bytes;
  const auto rhs_end = rhs_begin + rhs_bytes;
  return lhs_begin < rhs_end && rhs_begin < lhs_end;
}

double tensor_max_abs(const torch::Tensor &tensor) {
  if (!tensor.defined() || tensor.numel() <= 0 ||
      !torch::isfinite(tensor).all().item<bool>()) {
    local_fail("max-absolute-value tensor is empty or nonfinite");
  }
  return tensor.abs().max().item<double>();
}

double gradient_l2(const std::vector<torch::Tensor> &parameters) {
  double square_sum = 0.0;
  for (const auto &parameter : parameters) {
    if (!parameter.grad().defined() ||
        !torch::isfinite(parameter.grad()).all().item<bool>()) {
      local_fail("residual parameter has an absent or nonfinite gradient");
    }
    square_sum += parameter.grad()
                      .detach()
                      .to(torch::kFloat64)
                      .pow(2)
                      .sum()
                      .item<double>();
  }
  const double result = std::sqrt(square_sum);
  if (!std::isfinite(result) || result < 0.0) {
    local_fail("residual gradient norm is invalid");
  }
  return result;
}

template <std::size_t N>
std::vector<torch::Tensor>
as_vector(const std::array<torch::Tensor, N> &parameters) {
  return std::vector<torch::Tensor>(parameters.begin(), parameters.end());
}

double parameter_delta_l2(
    const std::array<torch::Tensor, kDefaultHiddenParameterTensorCount>
        &parameters,
    const std::array<torch::Tensor, kDefaultHiddenParameterTensorCount>
        &snapshots) {
  double square_sum = 0.0;
  torch::NoGradGuard no_grad;
  for (std::size_t index = 0; index < parameters.size(); ++index) {
    square_sum += (parameters[index].detach().to(torch::kFloat64) -
                   snapshots[index].to(torch::kFloat64))
                      .pow(2)
                      .sum()
                      .item<double>();
  }
  const double result = std::sqrt(square_sum);
  if (!std::isfinite(result) || result < 0.0) {
    local_fail("residual parameter delta is invalid");
  }
  return result;
}

void require_finite_residual(const ZeroOutputResidualMlp &model,
                             bool require_gradients) {
  for (const auto &parameter : model->parameters()) {
    if (!torch::isfinite(parameter).all().item<bool>()) {
      local_fail("residual model contains a nonfinite parameter");
    }
    if (require_gradients &&
        (!parameter.grad().defined() ||
         !torch::isfinite(parameter.grad()).all().item<bool>())) {
      local_fail("residual model contains an absent/nonfinite gradient");
    }
  }
}

void require_frozen_base(const Float32HeadMap &base,
                         const Float32HeadMap &snapshot) {
  if (base.weights.dim() != 2 || base.weights.size(0) != kHeadCount ||
      base.weights.size(1) != kFeatureWidth || base.bias.dim() != 1 ||
      base.bias.size(0) != kHeadCount ||
      base.weights.scalar_type() != torch::kFloat32 ||
      base.bias.scalar_type() != torch::kFloat32 ||
      !base.weights.device().is_cpu() || !base.bias.device().is_cpu() ||
      !base.weights.is_contiguous() || !base.bias.is_contiguous() ||
      base.weights.requires_grad() || base.bias.requires_grad() ||
      base.weights.grad().defined() || base.bias.grad().defined() ||
      !torch::isfinite(base.weights).all().item<bool>() ||
      !torch::isfinite(base.bias).all().item<bool>() ||
      !tensor_bytes_equal(base.weights, snapshot.weights) ||
      !tensor_bytes_equal(base.bias, snapshot.bias)) {
    local_fail("frozen direct-float32 base integrity failed");
  }
}

void require_base_disjoint_from_parameters(
    const Float32HeadMap &base,
    const std::vector<torch::Tensor> &parameters) {
  for (const auto &parameter : parameters) {
    if (storage_ranges_overlap(base.weights, parameter) ||
        storage_ranges_overlap(base.bias, parameter)) {
      local_fail("frozen base aliases a residual parameter storage range");
    }
  }
}

void require_residual_parameter_contract(const ZeroOutputResidualMlp &model) {
  const auto parameters = model->parameters();
  if (static_cast<int64_t>(parameters.size()) !=
      kResidualParameterTensorCount) {
    local_fail("residual parameter tensor count mismatch");
  }
  int64_t element_count = 0;
  std::set<const void *> identities;
  for (const auto &parameter : parameters) {
    if (!parameter.requires_grad() || parameter.grad().defined() ||
        !parameter.device().is_cpu() || !parameter.is_contiguous() ||
        parameter.scalar_type() != torch::kFloat32 ||
        !torch::isfinite(parameter).all().item<bool>()) {
      local_fail("residual parameter contract mismatch");
    }
    element_count += parameter.numel();
    if (!identities.insert(parameter.data_ptr()).second) {
      local_fail("residual parameter storage identity is duplicated");
    }
  }
  if (element_count != kResidualParameterElementCount) {
    local_fail("residual parameter element count mismatch");
  }
  const auto upstream = model->upstream_parameters();
  const auto output = model->output_parameters();
  int64_t upstream_elements = 0;
  for (const auto &parameter : upstream) {
    upstream_elements += parameter.numel();
    if (tensor_bytes_all_zero(parameter)) {
      local_fail("an input/hidden residual parameter was zero-initialized");
    }
  }
  int64_t output_elements = 0;
  for (const auto &parameter : output) {
    output_elements += parameter.numel();
    if (!tensor_bytes_all_zero(parameter)) {
      local_fail("residual output parameter is not byte-zero initialized");
    }
  }
  if (upstream_elements != kDefaultHiddenParameterElementCount ||
      output_elements != kZeroOutputParameterElementCount) {
    local_fail("residual initialization element count mismatch");
  }
}

void require_optimizer_contract(torch::optim::Adam &optimizer,
                                const ZeroOutputResidualMlp &model,
                                const Float32HeadMap &base) {
  const auto model_parameters = model->parameters();
  const auto &groups = optimizer.param_groups();
  if (groups.size() != 1 ||
      static_cast<int64_t>(groups.front().params().size()) !=
          kResidualParameterTensorCount ||
      !optimizer.state().empty()) {
    local_fail("residual optimizer group/state contract mismatch");
  }
  std::set<const void *> model_identities;
  for (const auto &parameter : model_parameters) {
    model_identities.insert(parameter.data_ptr());
  }
  std::set<const void *> optimizer_identities;
  int64_t optimizer_elements = 0;
  for (const auto &parameter : groups.front().params()) {
    optimizer_identities.insert(parameter.data_ptr());
    optimizer_elements += parameter.numel();
    if (storage_ranges_overlap(base.weights, parameter) ||
        storage_ranges_overlap(base.bias, parameter)) {
      local_fail("frozen base is present in residual optimizer storage");
    }
  }
  if (model_identities != optimizer_identities ||
      optimizer_elements != kResidualParameterElementCount) {
    local_fail("residual optimizer parameter identity/count mismatch");
  }
}

double full_combined_standardized_mse(
    ZeroOutputResidualMlp &model, const torch::Tensor &features,
    const torch::Tensor &base_prediction, const torch::Tensor &target,
    const torch::Tensor &heads) {
  torch::NoGradGuard no_grad;
  model->eval();
  const auto combined =
      base_prediction + kResidualScale * model->forward(features, heads);
  const double value = torch::mse_loss(combined, target).item<double>();
  if (value < 0.0 || !std::isfinite(value)) {
    local_fail("combined full-train standardized MSE must be nonnegative and "
               "finite");
  }
  return value;
}

double nonnegative_standardized_mse(const torch::Tensor &prediction,
                                    const torch::Tensor &target) {
  if (prediction.numel() != target.numel()) {
    local_fail("nonnegative standardized MSE shape mismatch");
  }
  const double value =
      torch::mse_loss(prediction.reshape({-1}).to(torch::kFloat64),
                      target.reshape({-1}).to(torch::kFloat64))
          .item<double>();
  if (value < 0.0 || !std::isfinite(value)) {
    local_fail("standardized MSE must be nonnegative and finite");
  }
  return value;
}

std::array<double, kHeadCount> nonnegative_per_head_standardized_mse(
    const torch::Tensor &prediction, const torch::Tensor &target,
    int64_t anchor_count) {
  const auto prediction_cube =
      prediction.reshape({anchor_count, kEdgeCount, kChannelCount});
  const auto target_cube =
      target.reshape({anchor_count, kEdgeCount, kChannelCount});
  std::array<double, kHeadCount> result{};
  for (int64_t head = 0; head < kHeadCount; ++head) {
    const int64_t edge = head % kEdgeCount;
    const int64_t channel = head / kEdgeCount;
    result[static_cast<std::size_t>(head)] = nonnegative_standardized_mse(
        prediction_cube.select(1, edge).select(1, channel),
        target_cube.select(1, edge).select(1, channel));
  }
  return result;
}

CompleteRouteMetrics evaluate_complete_route_nonnegative(
    const torch::Tensor &original_prediction,
    const torch::Tensor &original_target,
    const torch::Tensor &standardized_prediction,
    const torch::Tensor &standardized_target, int64_t anchor_count) {
  return {.aggregate_and_channels =
              evaluate_route(original_prediction, original_target),
          .heads = evaluate_scalar_heads(original_prediction, original_target),
          .standardized_mse = nonnegative_standardized_mse(
              standardized_prediction, standardized_target),
          .head_standardized_mse = nonnegative_per_head_standardized_mse(
              standardized_prediction, standardized_target, anchor_count)};
}

ResidualTrainingSummary train_residual(
    ZeroOutputResidualMlp &model, torch::optim::Adam &optimizer,
    const torch::Tensor &features, const torch::Tensor &base_prediction,
    const torch::Tensor &target, const torch::Tensor &heads,
    const BatchSchedule &schedule, const Float32HeadMap &base,
    const Float32HeadMap &base_snapshot) {
  ResidualTrainingSummary summary;
  summary.initial_full_train_standardized_mse =
      full_combined_standardized_mse(model, features, base_prediction, target,
                                     heads);
  require_finite_residual(model, false);
  const auto upstream_parameters = model->upstream_parameters();
  std::array<torch::Tensor, kDefaultHiddenParameterTensorCount>
      upstream_snapshots{};
  {
    torch::NoGradGuard no_grad;
    for (std::size_t index = 0; index < upstream_parameters.size(); ++index) {
      upstream_snapshots[index] =
          upstream_parameters[index].detach().clone().contiguous();
    }
  }

  model->train();
  for (int64_t step = 0; step < kSteps; ++step) {
    const auto batch = schedule.indices.select(0, step);
    const auto residual = model->forward(features.index_select(0, batch),
                                         heads.index_select(0, batch));
    const auto combined =
        base_prediction.index_select(0, batch) + kResidualScale * residual;
    const auto loss =
        torch::mse_loss(combined, target.index_select(0, batch));
    if (!torch::isfinite(loss).item<bool>()) {
      local_fail("residual training produced a nonfinite loss");
    }
    optimizer.zero_grad();
    loss.backward();
    require_finite_residual(model, true);

    const auto upstream = as_vector(model->upstream_parameters());
    if (step == 0) {
      summary.first_backward_upstream_gradient_norm = gradient_l2(upstream);
      summary.first_backward_output_weight_gradient_norm =
          gradient_l2({model->output_weight()});
      if (summary.first_backward_upstream_gradient_norm != 0.0 ||
          !(summary.first_backward_output_weight_gradient_norm > 0.0)) {
        local_fail("zero-output residual first-backward activation proof "
                   "failed");
      }
    } else if (step == 1) {
      summary.second_backward_upstream_gradient_norm = gradient_l2(upstream);
      if (!(summary.second_backward_upstream_gradient_norm > 0.0)) {
        local_fail("zero-output residual second-backward upstream activation "
                   "proof failed");
      }
    }

    const double gradient_norm = torch::nn::utils::clip_grad_norm_(
        model->parameters(), kGradientClipNorm);
    if (!std::isfinite(gradient_norm)) {
      local_fail("residual training produced a nonfinite gradient norm");
    }
    summary.maximum_preclip_gradient_norm =
        std::max(summary.maximum_preclip_gradient_norm, gradient_norm);
    if (gradient_norm > kGradientClipNorm) {
      ++summary.clipped_step_count;
    }
    optimizer.step();
    require_finite_residual(model, false);
    require_frozen_base(base, base_snapshot);
    ++summary.base_per_step_byte_invariance_check_count;
    if (step == 0) {
      summary.first_step_output_weight_update_max_abs =
          tensor_max_abs(model->output_weight());
      if (!(summary.first_step_output_weight_update_max_abs > 0.0)) {
        local_fail("zero-output residual first optimizer step did not update "
                   "the output weight");
      }
    }
    summary.last_minibatch_loss = loss.item<double>();
    ++summary.optimizer_steps_completed;
  }
  if (summary.optimizer_steps_completed != kSteps ||
      summary.base_per_step_byte_invariance_check_count != kSteps) {
    local_fail("residual training did not complete its fixed schedule and base "
               "checks");
  }
  summary.final_full_train_standardized_mse =
      full_combined_standardized_mse(model, features, base_prediction, target,
                                     heads);
  summary.final_upstream_parameter_delta_l2 =
      parameter_delta_l2(upstream_parameters, upstream_snapshots);
  if (!(summary.final_upstream_parameter_delta_l2 > 0.0)) {
    local_fail("residual upstream parameters did not change");
  }
  return summary;
}

struct PreservationComparison {
  double aggregate_standardized_mse_ratio{0.0};
  std::array<double, kHeadCount> head_standardized_mse_ratios{};
  double maximum_head_standardized_mse_ratio{0.0};
  double direction_deficit{0.0};
  double rank_deficit{0.0};
  double correlation_deficit{0.0};
  double rmse_ratio_increase{0.0};
  bool aggregate_preservation_pass{false};
  bool every_head_preservation_pass{false};
  bool direction_pass{false};
  bool rank_pass{false};
  bool correlation_pass{false};
  bool rmse_ratio_pass{false};
  bool head_and_metric_safety_pass{false};
  bool preservation_gate_pass{false};
};

PreservationComparison compare_to_direct_float32_base(
    const CompleteRouteMetrics &final_metrics,
    const CompleteRouteMetrics &base_metrics, double aggregate_ratio_limit) {
  PreservationComparison result;
  const double aggregate_denominator = base_metrics.standardized_mse;
  if (!(aggregate_denominator > 0.0) ||
      !std::isfinite(aggregate_denominator)) {
    local_fail("direct-float32 base aggregate standardized MSE denominator is "
               "invalid");
  }
  result.aggregate_standardized_mse_ratio =
      final_metrics.standardized_mse / aggregate_denominator;
  if (result.aggregate_standardized_mse_ratio < 0.0 ||
      !std::isfinite(result.aggregate_standardized_mse_ratio)) {
    local_fail("combined/base aggregate standardized MSE ratio is invalid");
  }
  result.aggregate_preservation_pass =
      result.aggregate_standardized_mse_ratio <= aggregate_ratio_limit;
  result.every_head_preservation_pass = true;
  for (int64_t head = 0; head < kHeadCount; ++head) {
    const auto index = static_cast<std::size_t>(head);
    const double denominator = base_metrics.head_standardized_mse[index];
    if (!(denominator > 0.0) || !std::isfinite(denominator)) {
      local_fail("direct-float32 base head standardized MSE denominator is "
                 "invalid");
    }
    const double ratio =
        final_metrics.head_standardized_mse[index] / denominator;
    if (ratio < 0.0 || !std::isfinite(ratio)) {
      local_fail("combined/base head standardized MSE ratio is invalid");
    }
    result.head_standardized_mse_ratios[index] = ratio;
    result.maximum_head_standardized_mse_ratio =
        std::max(result.maximum_head_standardized_mse_ratio, ratio);
    result.every_head_preservation_pass =
        result.every_head_preservation_pass && ratio <= kHeadMseRatioLimit;
  }
  const auto &base = base_metrics.aggregate_and_channels.aggregate;
  const auto &combined = final_metrics.aggregate_and_channels.aggregate;
  result.direction_deficit = base.direction - combined.direction;
  result.rank_deficit = base.rank - combined.rank;
  result.correlation_deficit = base.correlation - combined.correlation;
  result.rmse_ratio_increase = combined.rmse_target_rms_ratio -
                               base.rmse_target_rms_ratio;
  for (const double value :
       {result.direction_deficit, result.rank_deficit,
        result.correlation_deficit, result.rmse_ratio_increase}) {
    if (!std::isfinite(value)) {
      local_fail("combined/base metric difference is nonfinite");
    }
  }
  result.direction_pass = result.direction_deficit <= kMetricDeficitLimit;
  result.rank_pass = result.rank_deficit <= kMetricDeficitLimit;
  result.correlation_pass =
      result.correlation_deficit <= kMetricDeficitLimit;
  result.rmse_ratio_pass =
      result.rmse_ratio_increase <= kRmseRatioIncreaseLimit;
  result.head_and_metric_safety_pass =
      result.every_head_preservation_pass && result.direction_pass &&
      result.rank_pass && result.correlation_pass && result.rmse_ratio_pass;
  result.preservation_gate_pass =
      result.aggregate_preservation_pass &&
      result.head_and_metric_safety_pass;
  return result;
}

void emit_preservation_comparison(
    std::ostream &output, const std::string &split,
    const PreservationComparison &comparison, double aggregate_ratio_limit) {
  output << split
         << "_aggregate_standardized_mse_ratio_to_direct_float32_base="
         << comparison.aggregate_standardized_mse_ratio << '\n';
  output << split << "_aggregate_standardized_mse_ratio_limit="
         << aggregate_ratio_limit << '\n';
  output << split << "_aggregate_preservation_pass="
         << (comparison.aggregate_preservation_pass ? "true" : "false")
         << '\n';
  output << split << "_head_standardized_mse_ratio_limit="
         << kHeadMseRatioLimit << '\n';
  for (int64_t head = 0; head < kHeadCount; ++head) {
    const int64_t edge = head % kEdgeCount;
    const int64_t channel = head / kEdgeCount;
    const double ratio = comparison.head_standardized_mse_ratios
        [static_cast<std::size_t>(head)];
    output << split << "_head_" << head << ".edge_" << edge << ".channel_"
           << channel << ".standardized_mse_ratio_to_direct_float32_base="
           << ratio << '\n';
    output << split << "_head_" << head << ".edge_" << edge << ".channel_"
           << channel << ".standardized_mse_ratio_pass="
           << (ratio <= kHeadMseRatioLimit ? "true" : "false") << '\n';
  }
  output << split
         << "_maximum_head_standardized_mse_ratio_to_direct_float32_base="
         << comparison.maximum_head_standardized_mse_ratio << '\n';
  output << split << "_every_head_standardized_mse_ratio_pass="
         << (comparison.every_head_preservation_pass ? "true" : "false")
         << '\n';
  output << split << "_direction_deficit_to_direct_float32_base="
         << comparison.direction_deficit << '\n';
  output << split << "_direction_pass="
         << (comparison.direction_pass ? "true" : "false") << '\n';
  output << split << "_rank_deficit_to_direct_float32_base="
         << comparison.rank_deficit << '\n';
  output << split << "_rank_pass="
         << (comparison.rank_pass ? "true" : "false") << '\n';
  output << split << "_correlation_deficit_to_direct_float32_base="
         << comparison.correlation_deficit << '\n';
  output << split << "_correlation_pass="
         << (comparison.correlation_pass ? "true" : "false") << '\n';
  output << split << "_rmse_target_rms_ratio_increase_to_direct_float32_base="
         << comparison.rmse_ratio_increase << '\n';
  output << split << "_rmse_target_rms_ratio_pass="
         << (comparison.rmse_ratio_pass ? "true" : "false") << '\n';
  output << split << "_head_and_metric_safety_gate_pass="
         << (comparison.head_and_metric_safety_pass ? "true" : "false")
         << '\n';
  output << split << "_preservation_gate_pass="
         << (comparison.preservation_gate_pass ? "true" : "false") << '\n';
}

void run_residual(const LocalOptions &options) {
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);
  at::globalContext().setDeterministicFillUninitializedMemory(true);

  require_regular_nonsymlink(options.train_input);
  require_regular_nonsymlink(options.validation_input);
  const auto train = read_probe(options.train_input, kTrainBegin, kTrainEnd,
                                kRepresentationSpec);
  const auto validation = read_probe(options.validation_input, kValidationBegin,
                                     kValidationEnd, kRepresentationSpec);

  const auto affine = fit_conditioned_affine(train);
  const auto float64_train = predict_float64(affine, train.features);
  const auto float64_validation = predict_float64(affine, validation.features);
  const auto features = standardize_features(train, validation);
  const auto target = standardize_target(train, validation);
  const auto train_heads = head_indices(kTrainEnd - kTrainBegin);
  const auto validation_heads = head_indices(kValidationEnd - kValidationBegin);
  const auto converted_base =
      convert_to_standardized_float32_heads(affine, features, target);
  const Float32HeadMap base{
      .weights = converted_base.weights.detach().clone().contiguous(),
      .bias = converted_base.bias.detach().clone().contiguous()};
  const Float32HeadMap base_snapshot{
      .weights = base.weights.detach().clone().contiguous(),
      .bias = base.bias.detach().clone().contiguous()};
  require_frozen_base(base, base_snapshot);

  const auto base_train_standardized =
      gather_direct_heads(features.train, train_heads, base).detach().contiguous();
  const auto base_validation_standardized =
      gather_direct_heads(features.validation, validation_heads, base)
          .detach()
          .contiguous();
  const auto base_train_standardized_snapshot =
      base_train_standardized.clone().contiguous();
  const auto base_validation_standardized_snapshot =
      base_validation_standardized.clone().contiguous();

  torch::manual_seed(kSeed);
  ZeroOutputResidualMlp residual_model;
  residual_model->zero_output_layer();
  require_residual_parameter_contract(residual_model);
  require_base_disjoint_from_parameters(base, residual_model->parameters());

  torch::Tensor step0_residual_train;
  torch::Tensor step0_residual_validation;
  torch::Tensor step0_combined_train_standardized;
  torch::Tensor step0_combined_validation_standardized;
  {
    torch::NoGradGuard no_grad;
    residual_model->eval();
    step0_residual_train =
        residual_model->forward(features.train, train_heads).contiguous();
    step0_residual_validation =
        residual_model->forward(features.validation, validation_heads)
            .contiguous();
    step0_combined_train_standardized =
        (base_train_standardized + kResidualScale * step0_residual_train)
            .contiguous();
    step0_combined_validation_standardized =
        (base_validation_standardized +
         kResidualScale * step0_residual_validation)
            .contiguous();
  }
  const double step0_residual_train_max_abs =
      tensor_max_abs(step0_residual_train);
  const double step0_residual_validation_max_abs =
      tensor_max_abs(step0_residual_validation);
  const double step0_combined_base_train_max_abs_delta = max_abs_delta(
      step0_combined_train_standardized, base_train_standardized);
  const double step0_combined_base_validation_max_abs_delta = max_abs_delta(
      step0_combined_validation_standardized, base_validation_standardized);
  if (step0_residual_train_max_abs != 0.0 ||
      step0_residual_validation_max_abs != 0.0 ||
      step0_combined_base_train_max_abs_delta != 0.0 ||
      step0_combined_base_validation_max_abs_delta != 0.0) {
    local_fail("zero-output residual step-zero identity failed before Adam");
  }
  require_frozen_base(base, base_snapshot);

  const auto schedule = make_batch_schedule(features.train.size(0));
  if (schedule.fingerprint != kExpectedBatchScheduleFingerprint) {
    local_fail("fixed residual batch-schedule fingerprint mismatch before "
               "Adam");
  }
  torch::optim::AdamOptions adam_options(kLearningRate);
  adam_options.betas(std::make_tuple(kAdamBeta1, kAdamBeta2));
  adam_options.eps(kAdamEpsilon);
  adam_options.weight_decay(kWeightDecay);
  torch::optim::Adam optimizer(residual_model->parameters(), adam_options);
  require_optimizer_contract(optimizer, residual_model, base);

  const auto training = train_residual(
      residual_model, optimizer, features.train, base_train_standardized,
      target.train, train_heads, schedule, base, base_snapshot);
  if (static_cast<int64_t>(optimizer.state().size()) !=
      kResidualParameterTensorCount) {
    local_fail("residual Adam final state count mismatch");
  }

  torch::Tensor final_residual_train;
  torch::Tensor final_residual_validation;
  torch::Tensor final_combined_train_standardized;
  torch::Tensor final_combined_validation_standardized;
  {
    torch::NoGradGuard no_grad;
    residual_model->eval();
    final_residual_train =
        residual_model->forward(features.train, train_heads).contiguous();
    final_residual_validation =
        residual_model->forward(features.validation, validation_heads)
            .contiguous();
    final_combined_train_standardized =
        (base_train_standardized + kResidualScale * final_residual_train)
            .contiguous();
    final_combined_validation_standardized =
        (base_validation_standardized +
         kResidualScale * final_residual_validation)
            .contiguous();
  }
  const double final_residual_train_rms =
      final_residual_train.to(torch::kFloat64).pow(2).mean().sqrt().item<double>();
  const double final_residual_validation_rms = final_residual_validation
                                                   .to(torch::kFloat64)
                                                   .pow(2)
                                                   .mean()
                                                   .sqrt()
                                                   .item<double>();
  if (!(final_residual_train_rms > 0.0) ||
      !(final_residual_validation_rms > 0.0) ||
      !std::isfinite(final_residual_train_rms) ||
      !std::isfinite(final_residual_validation_rms)) {
    local_fail("final residual activation proof failed");
  }
  require_frozen_base(base, base_snapshot);
  const auto recomputed_base_train =
      gather_direct_heads(features.train, train_heads, base).detach().contiguous();
  const auto recomputed_base_validation =
      gather_direct_heads(features.validation, validation_heads, base)
          .detach()
          .contiguous();
  if (!tensor_bytes_equal(recomputed_base_train,
                          base_train_standardized_snapshot) ||
      !tensor_bytes_equal(recomputed_base_validation,
                          base_validation_standardized_snapshot)) {
    local_fail("frozen base final prediction byte parity failed");
  }

  const auto base_train = original_units(
      base_train_standardized, kTrainEnd - kTrainBegin, target);
  const auto base_validation =
      original_units(base_validation_standardized,
                     kValidationEnd - kValidationBegin, target);
  const auto step0_combined_train = original_units(
      step0_combined_train_standardized, kTrainEnd - kTrainBegin, target);
  const auto step0_combined_validation =
      original_units(step0_combined_validation_standardized,
                     kValidationEnd - kValidationBegin, target);
  const auto final_combined_train = original_units(
      final_combined_train_standardized, kTrainEnd - kTrainBegin, target);
  const auto final_combined_validation =
      original_units(final_combined_validation_standardized,
                     kValidationEnd - kValidationBegin, target);
  const auto float64_train_standardized =
      standardized_units(float64_train, target);
  const auto float64_validation_standardized =
      standardized_units(float64_validation, target);

  const auto oracle_train_metrics = evaluate_complete_route_nonnegative(
      float64_train, train.target, float64_train_standardized, target.train,
      kTrainEnd - kTrainBegin);
  const auto oracle_validation_metrics = evaluate_complete_route_nonnegative(
      float64_validation, validation.target, float64_validation_standardized,
      target.validation, kValidationEnd - kValidationBegin);
  const auto base_train_metrics = evaluate_complete_route_nonnegative(
      base_train, train.target, base_train_standardized, target.train,
      kTrainEnd - kTrainBegin);
  const auto base_validation_metrics = evaluate_complete_route_nonnegative(
      base_validation, validation.target, base_validation_standardized,
      target.validation, kValidationEnd - kValidationBegin);
  const auto step0_train_metrics = evaluate_complete_route_nonnegative(
      step0_combined_train, train.target, step0_combined_train_standardized,
      target.train, kTrainEnd - kTrainBegin);
  const auto step0_validation_metrics = evaluate_complete_route_nonnegative(
      step0_combined_validation, validation.target,
      step0_combined_validation_standardized, target.validation,
      kValidationEnd - kValidationBegin);
  const auto final_train_metrics = evaluate_complete_route_nonnegative(
      final_combined_train, train.target, final_combined_train_standardized,
      target.train, kTrainEnd - kTrainBegin);
  const auto final_validation_metrics = evaluate_complete_route_nonnegative(
      final_combined_validation, validation.target,
      final_combined_validation_standardized, target.validation,
      kValidationEnd - kValidationBegin);

  const auto train_comparison = compare_to_direct_float32_base(
      final_train_metrics, base_train_metrics,
      kTrainPreservationAggregateMseRatioLimit);
  const auto validation_comparison = compare_to_direct_float32_base(
      final_validation_metrics, base_validation_metrics,
      kValidationPreservationAggregateMseRatioLimit);
  const bool train_material_benefit_gate_pass =
      train_comparison.aggregate_standardized_mse_ratio <=
      kTrainMaterialBenefitAggregateMseRatioLimit;
  const bool train_repair_candidate_gate_pass =
      train_material_benefit_gate_pass &&
      train_comparison.head_and_metric_safety_pass;
  const bool residual_repair_gate_pass =
      train_repair_candidate_gate_pass &&
      train_comparison.preservation_gate_pass &&
      validation_comparison.preservation_gate_pass;
  const bool clear_stop_aggregate_threshold_pass =
      train_comparison.aggregate_standardized_mse_ratio >=
      kClearStopAggregateMseRatio;
  const bool clear_stop_maximum_head_threshold_pass =
      train_comparison.maximum_head_standardized_mse_ratio >=
      kClearStopMaximumHeadMseRatio;
  const bool clear_stop_gate_pass =
      !train_comparison.preservation_gate_pass &&
      (clear_stop_aggregate_threshold_pass ||
       clear_stop_maximum_head_threshold_pass);
  const auto &final_validation_aggregate =
      final_validation_metrics.aggregate_and_channels.aggregate;
  const bool strong_direction_pass =
      final_validation_aggregate.direction >= kStrongDirectionThreshold;
  const bool strong_rank_pass =
      final_validation_aggregate.rank >= kStrongRankThreshold;
  const bool strong_correlation_pass =
      final_validation_aggregate.correlation >= kStrongCorrelationThreshold;
  const bool strong_rmse_ratio_pass =
      final_validation_aggregate.rmse_target_rms_ratio <=
      kStrongRmseRatioLimit;
  const bool original_strong_gate_pass =
      strong_direction_pass && strong_rank_pass && strong_correlation_pass &&
      strong_rmse_ratio_pass;

  std::string_view classification =
      "frozen_affine_base_residual_inconclusive";
  if (residual_repair_gate_pass && original_strong_gate_pass) {
    classification =
        "frozen_affine_base_residual_strong_gate_pass_seed31";
  } else if (residual_repair_gate_pass) {
    classification =
        "frozen_affine_base_residual_repair_established_seed31";
  } else if (clear_stop_gate_pass) {
    classification = "residual_optimizer_destabilization_clear_stop";
  } else if (train_repair_candidate_gate_pass &&
             !validation_comparison.preservation_gate_pass) {
    classification =
        "residual_train_only_gain_no_validation_preservation";
  }

  if (!std::isfinite(training.initial_full_train_standardized_mse) ||
      !std::isfinite(training.final_full_train_standardized_mse) ||
      !std::isfinite(training.last_minibatch_loss) ||
      !std::isfinite(training.maximum_preclip_gradient_norm) ||
      !std::isfinite(training.first_backward_upstream_gradient_norm) ||
      !std::isfinite(training.first_backward_output_weight_gradient_norm) ||
      !std::isfinite(training.first_step_output_weight_update_max_abs) ||
      !std::isfinite(training.second_backward_upstream_gradient_norm) ||
      !std::isfinite(training.final_upstream_parameter_delta_l2) ||
      training.initial_full_train_standardized_mse <= 0.0 ||
      training.final_full_train_standardized_mse < 0.0 ||
      training.last_minibatch_loss < 0.0 ||
      training.maximum_preclip_gradient_norm < 0.0 ||
      training.first_backward_upstream_gradient_norm != 0.0 ||
      training.first_backward_output_weight_gradient_norm <= 0.0 ||
      training.first_step_output_weight_update_max_abs <= 0.0 ||
      training.second_backward_upstream_gradient_norm <= 0.0 ||
      training.final_upstream_parameter_delta_l2 <= 0.0 ||
      training.clipped_step_count < 0 ||
      training.clipped_step_count > training.optimizer_steps_completed ||
      training.optimizer_steps_completed != kSteps ||
      training.base_per_step_byte_invariance_check_count != kSteps) {
    local_fail("residual training diagnostics are invalid");
  }

  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::setprecision(17);
  output << "schema_id=" << kResidualSchema << '\n';
  output << "status=complete\n";
  output << "benchmark_id=synthetic_continuous_graph_v2\n";
  output << "diagnostic_phase=frozen_direct_float32_affine_base_zero_output_"
            "residual\n";
  output << "diagnostic_authority=development_only\n";
  output << "benchmark_acceptance_authority=false\n";
  output << "certified_authorization_eligible=false\n";
  output << "classification=" << classification << '\n';
  output << "train_input=" << options.train_input.string() << '\n';
  output << "validation_input=" << options.validation_input.string() << '\n';
  output << "probe_kind=representation\n";
  output << "probe_record_schema=" << kRepresentationSpec.record_schema << '\n';
  output << "train_probe_rows=" << train.rows << '\n';
  output << "validation_probe_rows=" << validation.rows << '\n';
  output << "certified_probe_rows=0\n";
  output << "fit_anchor_range=[0,2496)\n";
  output << "validation_anchor_range=[2560,2816)\n";
  output << "maximum_anchor_read=2815\n";
  output << "final_holdout_begin=3328\n";
  output << "validation_batches_read_by_trainer=0\n";
  output << "validation_read_by_trainer=false\n";
  output << "validation_combined_state_evaluation_count=2\n";
  output << "validation_postfit_guard_evaluation_count=1\n";
  output << "validation_driven_choice=false\n";
  output << "representation_forward_executed=false\n";
  output << "checkpoint_read=false\n";
  output << "checkpoint_written=false\n";
  output << "model_input_access=false\n";
  output << "certified_input_access=false\n";
  output << "final_holdout_access=false\n";
  output << "policy_access=false\n";
  output << "fixed_ridge=" << kFixedRidge << '\n';
  output << "ridge_selection=false\n";
  output << "residual_scale=" << kResidualScale << '\n';
  output << "head_index_formula=channel*3+edge\n";
  output << "flat_row_order=anchor,edge,channel\n";
  output << "base_architecture=detached_direct_float32_Linear(96,9)+gather("
            "channel*3+edge)\n";
  output << "residual_architecture=Linear(96,128)+GELU+Linear(128,128)+GELU+"
            "Linear(128,9)+gather(channel*3+edge)\n";
  output << "combined_prediction=base.detach()+1.0*residual\n";
  output << "residual_initialization=seed31_default_input_hidden_byte_zero_"
            "output_weight_bias\n";
  output << "base_storage=two_contiguous_immutable_nonparameter_float32_"
            "buffers\n";
  output << "optimizer_constructed_after_step0_integrity=true\n";
  output << "base_absent_from_optimizer=true\n";
  output << "base_requires_grad=false\n";
  output << "base_gradient_defined=false\n";
  output << "base_storage_alias_with_residual=false\n";
  output << "base_final_buffer_byte_invariance_pass=true\n";
  output << "base_final_prediction_byte_parity_pass=true\n";
  output << "step0_integrity_gate_pass=true\n";
  output << "activation_integrity_gate_pass=true\n";
  output << "device=cpu\n";
  output << "feature_dtype=float32\n";
  output << "target_training_dtype=float32\n";
  output << "metric_dtype=float64\n";
  output << "oracle_solver_dtype=float64\n";
  output << "deterministic_algorithms=true\n";
  output << "deterministic_cudnn=true\n";
  output << "deterministic_fill_uninitialized_memory=true\n";
  output << "intraop_threads=1\n";
  output << "interop_threads=1\n";
  output << "float64_solver=float64_centered_cholesky_ridge\n";
  output
      << "float32_feature_standardization=train_core_all_edges_all_channels\n";
  output << "target_standardization=train_core_per_edge_channel\n";
  output << "feature_standardization_clamped_coordinate_count="
         << features.clamped_coordinate_count << '\n';
  output << "target_standardization_clamped_coordinate_count="
         << target.clamped_coordinate_count << '\n';
  output << "affine_maximum_normalized_residual="
         << affine.maximum_normalized_residual << '\n';
  output << "affine_coefficient_l2_norm=" << affine.coefficient_l2_norm << '\n';
  output << "sealed_warm_start_reference_validation=external_runner_required\n";

  emit_complete_route(output, "route.float64_oracle.train",
                      oracle_train_metrics);
  emit_complete_route(output, "route.float64_oracle.validation",
                      oracle_validation_metrics);
  emit_complete_route(output, "route.direct_float32_base.train",
                      base_train_metrics);
  emit_complete_route(output, "route.direct_float32_base.validation",
                      base_validation_metrics);
  emit_complete_route(output, "route.combined_step0.train",
                      step0_train_metrics);
  emit_complete_route(output, "route.combined_step0.validation",
                      step0_validation_metrics);
  emit_complete_route(output, "route.combined_final.train",
                      final_train_metrics);
  emit_complete_route(output, "route.combined_final.validation",
                      final_validation_metrics);

  output << "step0_residual_train_standardized_max_abs="
         << step0_residual_train_max_abs << '\n';
  output << "step0_residual_validation_standardized_max_abs="
         << step0_residual_validation_max_abs << '\n';
  output << "step0_combined_base_train_standardized_max_abs_delta="
         << step0_combined_base_train_max_abs_delta << '\n';
  output << "step0_combined_base_validation_standardized_max_abs_delta="
         << step0_combined_base_validation_max_abs_delta << '\n';
  output << "final_residual_train_standardized_rms="
         << final_residual_train_rms << '\n';
  output << "final_residual_validation_standardized_rms="
         << final_residual_validation_rms << '\n';

  emit_preservation_comparison(
      output, "train", train_comparison,
      kTrainPreservationAggregateMseRatioLimit);
  emit_preservation_comparison(
      output, "validation", validation_comparison,
      kValidationPreservationAggregateMseRatioLimit);
  output << "metric_deficit_limit=" << kMetricDeficitLimit << '\n';
  output << "rmse_target_rms_ratio_increase_limit="
         << kRmseRatioIncreaseLimit << '\n';
  output << "train_material_benefit_aggregate_standardized_mse_ratio_limit="
         << kTrainMaterialBenefitAggregateMseRatioLimit << '\n';
  output << "train_material_benefit_gate_pass="
         << (train_material_benefit_gate_pass ? "true" : "false") << '\n';
  output << "train_repair_candidate_gate_pass="
         << (train_repair_candidate_gate_pass ? "true" : "false") << '\n';
  output << "residual_repair_gate_pass="
         << (residual_repair_gate_pass ? "true" : "false") << '\n';
  output << "clear_stop_aggregate_standardized_mse_ratio_threshold="
         << kClearStopAggregateMseRatio << '\n';
  output << "clear_stop_maximum_head_standardized_mse_ratio_threshold="
         << kClearStopMaximumHeadMseRatio << '\n';
  output << "clear_stop_aggregate_threshold_pass="
         << (clear_stop_aggregate_threshold_pass ? "true" : "false")
         << '\n';
  output << "clear_stop_maximum_head_threshold_pass="
         << (clear_stop_maximum_head_threshold_pass ? "true" : "false")
         << '\n';
  output << "clear_stop_gate_pass="
         << (clear_stop_gate_pass ? "true" : "false") << '\n';
  output << "original_strong_direction_threshold="
         << kStrongDirectionThreshold << '\n';
  output << "original_strong_rank_threshold=" << kStrongRankThreshold << '\n';
  output << "original_strong_correlation_threshold="
         << kStrongCorrelationThreshold << '\n';
  output << "original_strong_rmse_target_rms_ratio_limit="
         << kStrongRmseRatioLimit << '\n';
  output << "original_strong_direction_pass="
         << (strong_direction_pass ? "true" : "false") << '\n';
  output << "original_strong_rank_pass="
         << (strong_rank_pass ? "true" : "false") << '\n';
  output << "original_strong_correlation_pass="
         << (strong_correlation_pass ? "true" : "false") << '\n';
  output << "original_strong_rmse_target_rms_ratio_pass="
         << (strong_rmse_ratio_pass ? "true" : "false") << '\n';
  output << "original_strong_gate_pass="
         << (original_strong_gate_pass ? "true" : "false") << '\n';
  output << "original_strong_gate_evaluated=true\n";
  output << "original_strong_gate_acceptance_authority=false\n";
  output << "train_preservation_gate=train_aggregate_standardized_mse_ratio_"
            "to_direct_float32_base<=1.05,every_train_head_ratio<=1.10,train_"
            "direction>=base-0.01,train_rank>=base-0.01,train_correlation>=base"
            "-0.01,train_rmse_target_rms_ratio<=base+0.05\n";
  output << "train_material_benefit_gate=train_aggregate_standardized_mse_"
            "ratio_to_direct_float32_base<=0.90\n";
  output << "validation_preservation_gate=validation_aggregate_standardized_"
            "mse_ratio_to_direct_float32_base<=1.05,every_validation_head_"
            "ratio<=1.10,validation_direction>=base-0.01,validation_rank>=base"
            "-0.01,validation_correlation>=base-0.01,validation_rmse_target_"
            "rms_ratio<=base+0.05\n";
  output << "clear_stop_gate=!train_preservation_and_(train_aggregate_"
            "standardized_mse_ratio_to_direct_float32_base>=1.25_or_train_"
            "maximum_head_standardized_mse_ratio_to_direct_float32_base>=1."
            "50)\n";
  output << "original_strong_gate=validation_direction>=0.95,validation_rank>"
            "=0.95,validation_correlation>=0.95,validation_rmse_target_rms_"
            "ratio<=0.25\n";

  output << "affine_oracle_grouped_fit_count=1\n";
  output << "affine_oracle_head_solve_count=9\n";
  output << "direct_float32_base_construction_count=1\n";
  output << "residual_optimizer_fits_completed=1\n";
  output << "optimizer_fits_completed=1\n";
  output << "total_train_fit_procedures=2\n";
  output << "seed_count=1\n";
  output << "seed=" << kSeed << '\n';
  output << "batch_schedule_count=1\n";
  output << "optimizer_steps_completed="
         << training.optimizer_steps_completed << '\n';
  output << "steps_per_fit=" << kSteps << '\n';
  output << "batch_size=" << kBatchSize << '\n';
  output << "batch_schedule_fingerprint=" << schedule.fingerprint << '\n';
  output << "learning_rate=" << kLearningRate << '\n';
  output << "adam_beta1=" << kAdamBeta1 << '\n';
  output << "adam_beta2=" << kAdamBeta2 << '\n';
  output << "adam_epsilon=" << kAdamEpsilon << '\n';
  output << "weight_decay=" << kWeightDecay << '\n';
  output << "gradient_clip_norm=" << kGradientClipNorm << '\n';
  output << "optimizer=Adam\n";
  output << "batch_sampling=mt19937_64_uniform_with_replacement\n";
  output << "early_stopping=false\n";
  output << "seed_selection=false\n";
  output << "hyperparameter_search=false\n";
  output << "retry=false\n";
  output << "refit=false\n";
  output << "retry_count=0\n";
  output << "refit_count=0\n";
  output << "early_stop_count=0\n";
  output << "base_tensor_count=" << kBaseTensorCount << '\n';
  output << "base_tensor_element_count=" << kBaseTensorElementCount << '\n';
  output << "base_parameter_tensor_count=0\n";
  output << "base_parameter_element_count=0\n";
  output << "base_optimizer_parameter_tensor_count=0\n";
  output << "base_optimizer_parameter_element_count=0\n";
  output << "residual_parameter_tensor_count="
         << kResidualParameterTensorCount << '\n';
  output << "residual_parameter_element_count="
         << kResidualParameterElementCount << '\n';
  output << "residual_trainable_parameter_tensor_count="
         << kResidualParameterTensorCount << '\n';
  output << "residual_trainable_parameter_element_count="
         << kResidualParameterElementCount << '\n';
  output << "optimizer_parameter_group_count=1\n";
  output << "optimizer_parameter_tensor_count="
         << kResidualParameterTensorCount << '\n';
  output << "optimizer_parameter_element_count="
         << kResidualParameterElementCount << '\n';
  output << "optimizer_state_count_before_first_step=0\n";
  output << "optimizer_state_count_after_fit=" << optimizer.state().size()
         << '\n';
  output << "zero_initialized_output_parameter_tensor_count="
         << kZeroOutputParameterTensorCount << '\n';
  output << "zero_initialized_output_parameter_element_count="
         << kZeroOutputParameterElementCount << '\n';
  output << "default_initialized_hidden_parameter_tensor_count="
         << kDefaultHiddenParameterTensorCount << '\n';
  output << "default_initialized_hidden_parameter_element_count="
         << kDefaultHiddenParameterElementCount << '\n';
  output << "first_backward_activation_check_count=1\n";
  output << "second_backward_activation_check_count=1\n";
  output << "base_per_step_byte_invariance_check_count="
         << training.base_per_step_byte_invariance_check_count << '\n';
  output << "residual.initial_full_train_standardized_mse="
         << training.initial_full_train_standardized_mse << '\n';
  output << "residual.final_full_train_standardized_mse="
         << training.final_full_train_standardized_mse << '\n';
  output << "residual.last_minibatch_loss=" << training.last_minibatch_loss
         << '\n';
  output << "residual.maximum_preclip_gradient_norm="
         << training.maximum_preclip_gradient_norm << '\n';
  output << "residual.clipped_step_count=" << training.clipped_step_count
         << '\n';
  output << "residual.first_backward_upstream_gradient_norm="
         << training.first_backward_upstream_gradient_norm << '\n';
  output << "residual.first_backward_output_weight_gradient_norm="
         << training.first_backward_output_weight_gradient_norm << '\n';
  output << "residual.first_step_output_weight_update_max_abs="
         << training.first_step_output_weight_update_max_abs << '\n';
  output << "residual.second_backward_upstream_gradient_norm="
         << training.second_backward_upstream_gradient_norm << '\n';
  output << "residual.final_upstream_parameter_delta_l2="
         << training.final_upstream_parameter_delta_l2 << '\n';
  output << "single_seed_causal_evidence_only=true\n";
  output << "confirmation_required=true\n";
  output << "confirmation_seed31_source=sealed_current_result\n";
  output << "confirmation_current_seed31_counted=true\n";
  output << "confirmation_fixed_seed_set=[31,47,73]\n";
  output << "confirmation_pass_rule=at_least_2_of_3_seeds\n";
  output << "next_confirmation_fixed_seeds=[47,73]\n";
  output << "successful_result_licenses_confirmation_only=true\n";
  if (!output) {
    local_fail("failed while rendering frozen-base residual report");
  }
  write_exclusive(options.output, output.str());
}

} // namespace

int main(int argc, char **argv) {
  try {
    std::locale::global(std::locale::classic());
    run_residual(parse_local_options(argc, argv));
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "frozen direct-float32 affine-base zero-output residual "
                 "probe: "
              << error.what() << '\n';
    return 1;
  }
}
