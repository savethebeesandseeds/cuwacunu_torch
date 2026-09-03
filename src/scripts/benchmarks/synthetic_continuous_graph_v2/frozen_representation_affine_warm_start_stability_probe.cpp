// Development-only affine warm-start stability evaluator.
//
// Reuse the frozen localization evaluator in the same translation unit so the
// canonical probe parser, fixed-ridge affine oracle, float32 conversion,
// paired-GELU injection, metrics, and batch schedule remain identical. All of
// its dependencies are included first because the frozen source is then placed
// in a named implementation namespace; this keeps its ordinary `main` symbol
// distinct without changing a byte of the frozen source.
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

namespace warm_start_frozen_localization {
#include "frozen_representation_affine_injection_optimizer_localization_probe.cpp"
} // namespace warm_start_frozen_localization

namespace {

using namespace warm_start_frozen_localization;

constexpr std::string_view kWarmStartSchema =
    "synthetic_v2_frozen_representation_affine_warm_start_stability_"
    "development_v1";
constexpr std::string_view kExpectedBatchScheduleFingerprint =
    "f2fa41d284a42d60";
constexpr double kWarmStartStep0ParityTolerance = 1.0e-5;
constexpr double kTrainAggregateMseRatioLimit = 1.05;
constexpr double kTrainHeadMseRatioLimit = 1.10;
constexpr double kMetricDeficitLimit = 0.01;
constexpr double kRmseRatioIncreaseLimit = 0.05;
constexpr double kClearStopAggregateMseRatio = 1.25;
constexpr double kClearStopMaximumHeadMseRatio = 1.50;

struct WarmStartTrainingSummary {
  double initial_full_train_standardized_mse{0.0};
  double final_full_train_standardized_mse{0.0};
  double last_minibatch_loss{0.0};
  double maximum_preclip_gradient_norm{0.0};
  int64_t clipped_step_count{0};
  int64_t optimizer_steps_completed{0};
};

struct ScalarHeadMetrics {
  int64_t count{0};
  double mae{0.0};
  double rmse{0.0};
  double target_rms{0.0};
  double prediction_rms{0.0};
  double rmse_target_rms_ratio{0.0};
  double direction{0.0};
  double correlation{0.0};
};

struct CompleteRouteMetrics {
  RouteMetrics aggregate_and_channels{};
  std::array<ScalarHeadMetrics, kHeadCount> heads{};
  double standardized_mse{0.0};
  std::array<double, kHeadCount> head_standardized_mse{};
};

void require_finite_warm_start(const PairedGeluMlp &model,
                               bool require_gradients) {
  for (const auto &parameter : model->parameters()) {
    if (!torch::isfinite(parameter).all().item<bool>()) {
      local_fail("affine warm-start model contains a nonfinite parameter");
    }
    if (require_gradients &&
        (!parameter.grad().defined() ||
         !torch::isfinite(parameter.grad()).all().item<bool>())) {
      local_fail(
          "affine warm-start model contains an absent/nonfinite gradient");
    }
  }
}

double full_warm_start_standardized_mse(PairedGeluMlp &model,
                                        const torch::Tensor &features,
                                        const torch::Tensor &target,
                                        const torch::Tensor &heads) {
  torch::NoGradGuard no_grad;
  model->eval();
  const double value =
      torch::mse_loss(model->forward(features, heads), target).item<double>();
  if (!(value > 0.0) || !std::isfinite(value)) {
    local_fail("affine warm-start full-train standardized MSE must be positive "
               "and finite");
  }
  return value;
}

WarmStartTrainingSummary
train_warm_start(PairedGeluMlp &model, const torch::Tensor &features,
                 const torch::Tensor &target, const torch::Tensor &heads,
                 const BatchSchedule &schedule) {
  WarmStartTrainingSummary summary;
  summary.initial_full_train_standardized_mse =
      full_warm_start_standardized_mse(model, features, target, heads);
  require_finite_warm_start(model, false);

  // This optimizer is deliberately constructed only after injection and the
  // caller's train/validation step-zero parity checks have succeeded.
  torch::optim::AdamOptions adam_options(kLearningRate);
  adam_options.betas(std::make_tuple(kAdamBeta1, kAdamBeta2));
  adam_options.eps(kAdamEpsilon);
  adam_options.weight_decay(kWeightDecay);
  torch::optim::Adam optimizer(model->parameters(), adam_options);

  model->train();
  for (int64_t step = 0; step < kSteps; ++step) {
    const auto batch = schedule.indices.select(0, step);
    const auto prediction = model->forward(features.index_select(0, batch),
                                           heads.index_select(0, batch));
    const auto loss =
        torch::mse_loss(prediction, target.index_select(0, batch));
    if (!torch::isfinite(loss).item<bool>()) {
      local_fail("affine warm-start training produced a nonfinite loss");
    }
    optimizer.zero_grad();
    loss.backward();
    require_finite_warm_start(model, true);
    const double gradient_norm = torch::nn::utils::clip_grad_norm_(
        model->parameters(), kGradientClipNorm);
    if (!std::isfinite(gradient_norm)) {
      local_fail(
          "affine warm-start training produced a nonfinite gradient norm");
    }
    summary.maximum_preclip_gradient_norm =
        std::max(summary.maximum_preclip_gradient_norm, gradient_norm);
    if (gradient_norm > kGradientClipNorm) {
      ++summary.clipped_step_count;
    }
    optimizer.step();
    require_finite_warm_start(model, false);
    summary.last_minibatch_loss = loss.item<double>();
    ++summary.optimizer_steps_completed;
  }
  if (summary.optimizer_steps_completed != kSteps) {
    local_fail("affine warm-start training did not complete its fixed "
               "schedule");
  }
  summary.final_full_train_standardized_mse =
      full_warm_start_standardized_mse(model, features, target, heads);
  return summary;
}

std::array<ScalarHeadMetrics, kHeadCount>
evaluate_scalar_heads(const torch::Tensor &prediction_input,
                      const torch::Tensor &target_input) {
  const auto prediction =
      prediction_input.to(torch::kCPU, torch::kFloat64).contiguous();
  const auto target =
      target_input.to(torch::kCPU, torch::kFloat64).contiguous();
  if (prediction.sizes() != target.sizes() || prediction.dim() != 3 ||
      prediction.size(1) != kEdgeCount ||
      prediction.size(2) != kChannelCount || prediction.size(0) <= 0) {
    local_fail("scalar-head metric tensor shape mismatch");
  }
  const auto p = prediction.accessor<double, 3>();
  const auto t = target.accessor<double, 3>();
  const auto sign = [](double value) {
    return value > 0.0 ? 1 : (value < 0.0 ? -1 : 0);
  };
  std::array<ScalarHeadMetrics, kHeadCount> result{};
  for (int64_t head = 0; head < kHeadCount; ++head) {
    const int64_t edge = head % kEdgeCount;
    const int64_t channel = head / kEdgeCount;
    double absolute_error_sum = 0.0;
    double square_error_sum = 0.0;
    double prediction_sum = 0.0;
    double prediction_square_sum = 0.0;
    double target_sum = 0.0;
    double target_square_sum = 0.0;
    double cross_sum = 0.0;
    int64_t direction_correct = 0;
    for (int64_t anchor = 0; anchor < prediction.size(0); ++anchor) {
      const double predicted = p[anchor][edge][channel];
      const double realized = t[anchor][edge][channel];
      if (!std::isfinite(predicted) || !std::isfinite(realized)) {
        local_fail("scalar-head metric contains a nonfinite value");
      }
      const double error = predicted - realized;
      absolute_error_sum += std::fabs(error);
      square_error_sum += error * error;
      prediction_sum += predicted;
      prediction_square_sum += predicted * predicted;
      target_sum += realized;
      target_square_sum += realized * realized;
      cross_sum += predicted * realized;
      if (sign(predicted) == sign(realized)) {
        ++direction_correct;
      }
    }
    const double count = static_cast<double>(prediction.size(0));
    const double target_rms = std::sqrt(target_square_sum / count);
    const double prediction_rms =
        std::sqrt(prediction_square_sum / count);
    const double rmse = std::sqrt(square_error_sum / count);
    const double covariance = cross_sum - prediction_sum * target_sum / count;
    const double prediction_variance =
        prediction_square_sum - prediction_sum * prediction_sum / count;
    const double target_variance =
        target_square_sum - target_sum * target_sum / count;
    const double correlation =
        prediction_variance > 0.0 && target_variance > 0.0
            ? covariance /
                  std::sqrt(prediction_variance * target_variance)
            : 0.0;
    ScalarHeadMetrics metrics{
        .count = prediction.size(0),
        .mae = absolute_error_sum / count,
        .rmse = rmse,
        .target_rms = target_rms,
        .prediction_rms = prediction_rms,
        .rmse_target_rms_ratio =
            target_rms > 0.0 ? rmse / target_rms
                             : std::numeric_limits<double>::infinity(),
        .direction = static_cast<double>(direction_correct) / count,
        .correlation = correlation};
    if (!std::isfinite(metrics.mae) || !std::isfinite(metrics.rmse) ||
        !std::isfinite(metrics.target_rms) ||
        !std::isfinite(metrics.prediction_rms) ||
        !std::isfinite(metrics.rmse_target_rms_ratio) ||
        !std::isfinite(metrics.direction) ||
        !std::isfinite(metrics.correlation)) {
      local_fail("scalar-head metric summary contains a nonfinite value");
    }
    result[static_cast<std::size_t>(head)] = metrics;
  }
  return result;
}

CompleteRouteMetrics
evaluate_complete_route(const torch::Tensor &original_prediction,
                        const torch::Tensor &original_target,
                        const torch::Tensor &standardized_prediction,
                        const torch::Tensor &standardized_target,
                        int64_t anchor_count) {
  return {.aggregate_and_channels =
              evaluate_route(original_prediction, original_target),
          .heads = evaluate_scalar_heads(original_prediction, original_target),
          .standardized_mse =
              standardized_mse(standardized_prediction, standardized_target),
          .head_standardized_mse = per_head_standardized_mse(
              standardized_prediction, standardized_target, anchor_count)};
}

void emit_scalar_head(std::ostream &output, const std::string &prefix,
                      const ScalarHeadMetrics &metrics,
                      double standardized_mse_value) {
  output << prefix << ".count=" << metrics.count << '\n';
  output << prefix << ".mae=" << metrics.mae << '\n';
  output << prefix << ".rmse=" << metrics.rmse << '\n';
  output << prefix << ".target_rms=" << metrics.target_rms << '\n';
  output << prefix << ".prediction_rms=" << metrics.prediction_rms << '\n';
  output << prefix << ".rmse_target_rms_ratio="
         << metrics.rmse_target_rms_ratio << '\n';
  output << prefix << ".directional_accuracy=" << metrics.direction << '\n';
  output << prefix << ".correlation=" << metrics.correlation << '\n';
  output << prefix << ".standardized_mse=" << standardized_mse_value << '\n';
}

void emit_complete_route(std::ostream &output, const std::string &prefix,
                         const CompleteRouteMetrics &metrics) {
  emit_metric(output, prefix + ".aggregate",
              metrics.aggregate_and_channels.aggregate);
  output << prefix << ".aggregate.standardized_mse="
         << metrics.standardized_mse << '\n';
  for (int64_t channel = 0; channel < kChannelCount; ++channel) {
    emit_metric(output, prefix + ".channel_" + std::to_string(channel),
                metrics.aggregate_and_channels
                    .channels[static_cast<std::size_t>(channel)]);
  }
  for (int64_t head = 0; head < kHeadCount; ++head) {
    const int64_t edge = head % kEdgeCount;
    const int64_t channel = head / kEdgeCount;
    emit_scalar_head(
        output,
        prefix + ".head_" + std::to_string(head) + ".edge_" +
            std::to_string(edge) + ".channel_" + std::to_string(channel),
        metrics.heads[static_cast<std::size_t>(head)],
        metrics.head_standardized_mse[static_cast<std::size_t>(head)]);
  }
}

void run_warm_start(const LocalOptions &options) {
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
  const auto head_map =
      convert_to_standardized_float32_heads(affine, features, target);

  const auto direct_train_standardized =
      gather_direct_heads(features.train, train_heads, head_map);
  const auto direct_validation_standardized =
      gather_direct_heads(features.validation, validation_heads, head_map);
  const auto direct_train = original_units(
      direct_train_standardized, kTrainEnd - kTrainBegin, target);
  const auto direct_validation =
      original_units(direct_validation_standardized,
                     kValidationEnd - kValidationBegin, target);

  torch::manual_seed(kSeed);
  PairedGeluMlp warm_start;
  warm_start->inject(head_map);
  require_finite_warm_start(warm_start, false);
  warm_start->eval();
  torch::Tensor step0_train_standardized;
  torch::Tensor step0_validation_standardized;
  {
    torch::NoGradGuard no_grad;
    step0_train_standardized =
        warm_start->forward(features.train, train_heads).contiguous();
    step0_validation_standardized =
        warm_start->forward(features.validation, validation_heads).contiguous();
  }
  const double step0_train_delta =
      max_abs_delta(direct_train_standardized, step0_train_standardized);
  const double step0_validation_delta = max_abs_delta(
      direct_validation_standardized, step0_validation_standardized);
  const bool step0_train_parity_pass =
      step0_train_delta <= kWarmStartStep0ParityTolerance;
  const bool step0_validation_parity_pass =
      step0_validation_delta <= kWarmStartStep0ParityTolerance;
  const bool step0_parity_gate_pass =
      step0_train_parity_pass && step0_validation_parity_pass;
  if (!step0_parity_gate_pass) {
    local_fail("step-zero paired-GELU parity precondition failed before "
               "schedule and optimizer construction");
  }

  const auto schedule = make_batch_schedule(features.train.size(0));
  if (schedule.fingerprint != kExpectedBatchScheduleFingerprint) {
    local_fail("fixed batch-schedule fingerprint mismatch before optimizer "
               "construction");
  }
  const auto training = train_warm_start(
      warm_start, features.train, target.train, train_heads, schedule);

  torch::Tensor final_train_standardized;
  torch::Tensor final_validation_standardized;
  {
    torch::NoGradGuard no_grad;
    warm_start->eval();
    final_train_standardized =
        warm_start->forward(features.train, train_heads).contiguous();
    final_validation_standardized =
        warm_start->forward(features.validation, validation_heads).contiguous();
  }
  const auto step0_train = original_units(
      step0_train_standardized, kTrainEnd - kTrainBegin, target);
  const auto step0_validation =
      original_units(step0_validation_standardized,
                     kValidationEnd - kValidationBegin, target);
  const auto final_train = original_units(
      final_train_standardized, kTrainEnd - kTrainBegin, target);
  const auto final_validation =
      original_units(final_validation_standardized,
                     kValidationEnd - kValidationBegin, target);
  const auto float64_train_standardized =
      standardized_units(float64_train, target);
  const auto float64_validation_standardized =
      standardized_units(float64_validation, target);

  const auto oracle_train_metrics = evaluate_complete_route(
      float64_train, train.target, float64_train_standardized, target.train,
      kTrainEnd - kTrainBegin);
  const auto oracle_validation_metrics = evaluate_complete_route(
      float64_validation, validation.target, float64_validation_standardized,
      target.validation, kValidationEnd - kValidationBegin);
  const auto direct_train_metrics = evaluate_complete_route(
      direct_train, train.target, direct_train_standardized, target.train,
      kTrainEnd - kTrainBegin);
  const auto direct_validation_metrics = evaluate_complete_route(
      direct_validation, validation.target, direct_validation_standardized,
      target.validation, kValidationEnd - kValidationBegin);
  const auto step0_train_metrics = evaluate_complete_route(
      step0_train, train.target, step0_train_standardized, target.train,
      kTrainEnd - kTrainBegin);
  const auto step0_validation_metrics = evaluate_complete_route(
      step0_validation, validation.target, step0_validation_standardized,
      target.validation, kValidationEnd - kValidationBegin);
  const auto final_train_metrics = evaluate_complete_route(
      final_train, train.target, final_train_standardized, target.train,
      kTrainEnd - kTrainBegin);
  const auto final_validation_metrics = evaluate_complete_route(
      final_validation, validation.target, final_validation_standardized,
      target.validation, kValidationEnd - kValidationBegin);

  const double train_aggregate_mse_ratio =
      final_train_metrics.standardized_mse /
      oracle_train_metrics.standardized_mse;
  bool every_train_head_mse_ratio_pass = true;
  double maximum_train_head_mse_ratio = 0.0;
  std::array<double, kHeadCount> train_head_mse_ratios{};
  for (int64_t head = 0; head < kHeadCount; ++head) {
    const auto index = static_cast<std::size_t>(head);
    const double ratio = final_train_metrics.head_standardized_mse[index] /
                         oracle_train_metrics.head_standardized_mse[index];
    if (!(ratio > 0.0) || !std::isfinite(ratio)) {
      local_fail("affine warm-start head MSE ratio is invalid or nonfinite");
    }
    train_head_mse_ratios[index] = ratio;
    maximum_train_head_mse_ratio =
        std::max(maximum_train_head_mse_ratio, ratio);
    every_train_head_mse_ratio_pass =
        every_train_head_mse_ratio_pass && ratio <= kTrainHeadMseRatioLimit;
  }
  if (!(train_aggregate_mse_ratio > 0.0) ||
      !std::isfinite(train_aggregate_mse_ratio)) {
    local_fail(
        "affine warm-start aggregate MSE ratio is invalid or nonfinite");
  }

  const auto &oracle_train =
      oracle_train_metrics.aggregate_and_channels.aggregate;
  const auto &direct_validation_reference =
      direct_validation_metrics.aggregate_and_channels.aggregate;
  const auto &trained_train =
      final_train_metrics.aggregate_and_channels.aggregate;
  const auto &trained_validation =
      final_validation_metrics.aggregate_and_channels.aggregate;

  const bool train_aggregate_mse_ratio_pass =
      train_aggregate_mse_ratio <= kTrainAggregateMseRatioLimit;
  const double train_direction_deficit =
      oracle_train.direction - trained_train.direction;
  const double train_rank_deficit = oracle_train.rank - trained_train.rank;
  const double train_correlation_deficit =
      oracle_train.correlation - trained_train.correlation;
  const double train_rmse_ratio_increase =
      trained_train.rmse_target_rms_ratio -
      oracle_train.rmse_target_rms_ratio;
  const bool train_direction_pass =
      train_direction_deficit <= kMetricDeficitLimit;
  const bool train_rank_pass = train_rank_deficit <= kMetricDeficitLimit;
  const bool train_correlation_pass =
      train_correlation_deficit <= kMetricDeficitLimit;
  const bool train_rmse_ratio_pass =
      train_rmse_ratio_increase <= kRmseRatioIncreaseLimit;
  const bool train_preservation_gate_pass =
      train_aggregate_mse_ratio_pass && every_train_head_mse_ratio_pass &&
      train_direction_pass && train_rank_pass && train_correlation_pass &&
      train_rmse_ratio_pass;

  const double validation_direction_deficit =
      direct_validation_reference.direction - trained_validation.direction;
  const double validation_rank_deficit =
      direct_validation_reference.rank - trained_validation.rank;
  const double validation_correlation_deficit =
      direct_validation_reference.correlation -
      trained_validation.correlation;
  const double validation_rmse_ratio_increase =
      trained_validation.rmse_target_rms_ratio -
      direct_validation_reference.rmse_target_rms_ratio;
  const bool validation_direction_pass =
      validation_direction_deficit <= kMetricDeficitLimit;
  const bool validation_rank_pass =
      validation_rank_deficit <= kMetricDeficitLimit;
  const bool validation_correlation_pass =
      validation_correlation_deficit <= kMetricDeficitLimit;
  const bool validation_rmse_ratio_pass =
      validation_rmse_ratio_increase <= kRmseRatioIncreaseLimit;
  const bool validation_guard_gate_pass =
      validation_direction_pass && validation_rank_pass &&
      validation_correlation_pass && validation_rmse_ratio_pass;

  const bool warm_start_stability_gate_pass =
      step0_parity_gate_pass && train_preservation_gate_pass &&
      validation_guard_gate_pass;
  const bool clear_stop_aggregate_threshold_pass =
      train_aggregate_mse_ratio >= kClearStopAggregateMseRatio;
  const bool clear_stop_maximum_head_threshold_pass =
      maximum_train_head_mse_ratio >= kClearStopMaximumHeadMseRatio;
  const bool clear_stop_gate_pass =
      step0_parity_gate_pass && !warm_start_stability_gate_pass &&
      (clear_stop_aggregate_threshold_pass ||
       clear_stop_maximum_head_threshold_pass);
  std::string_view classification = "warm_start_stability_inconclusive";
  if (warm_start_stability_gate_pass) {
    classification = "warm_start_stability_established";
  } else if (clear_stop_gate_pass) {
    classification = "optimizer_destabilization_clear_stop";
  }

  if (!std::isfinite(training.initial_full_train_standardized_mse) ||
      !std::isfinite(training.final_full_train_standardized_mse) ||
      !std::isfinite(training.last_minibatch_loss) ||
      !std::isfinite(training.maximum_preclip_gradient_norm) ||
      training.initial_full_train_standardized_mse <= 0.0 ||
      training.final_full_train_standardized_mse <= 0.0 ||
      training.last_minibatch_loss < 0.0 ||
      training.maximum_preclip_gradient_norm < 0.0 ||
      training.clipped_step_count < 0 ||
      training.clipped_step_count > training.optimizer_steps_completed ||
      training.optimizer_steps_completed != kSteps) {
    local_fail("affine warm-start training diagnostics are invalid");
  }

  int64_t parameter_tensor_count = 0;
  int64_t trainable_parameter_count = 0;
  for (const auto &parameter : warm_start->parameters()) {
    ++parameter_tensor_count;
    if (parameter.requires_grad()) {
      trainable_parameter_count += parameter.numel();
    }
  }

  std::ostringstream output;
  output.imbue(std::locale::classic());
  output << std::setprecision(17);
  output << "schema_id=" << kWarmStartSchema << '\n';
  output << "status=complete\n";
  output << "benchmark_id=synthetic_continuous_graph_v2\n";
  output << "diagnostic_phase=affine_warm_start_stability\n";
  output << "diagnostic_authority=development_only\n";
  output << "benchmark_acceptance_authority=false\n";
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
  output << "validation_read_by_trainer=false\n";
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
  output << "head_index_formula=channel*3+edge\n";
  output << "flat_row_order=anchor,edge,channel\n";
  output << "architecture=Linear(96,128)+GELU+Linear(128,128)+GELU+"
            "Linear(128,9)+gather(channel*3+edge)\n";
  output << "initialization=paired_gelu_exact_affine_injection\n";
  output << "gelu_identity=GELU(z)-GELU(-z)=z\n";
  output << "gelu_injected_hidden_units_per_layer=18\n";
  output << "gelu_unused_hidden_units_per_layer=110\n";
  output << "optimizer_constructed_after_injection=true\n";
  output << "optimizer_constructed_after_step0_parity=true\n";
  output << "all_parameters_trainable=true\n";
  output << "frozen_parameter_tensor_count=0\n";
  output << "parameter_tensor_count=" << parameter_tensor_count << '\n';
  output << "trainable_parameter_count=" << trainable_parameter_count << '\n';
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

  emit_complete_route(output, "route.float64_oracle.train",
                      oracle_train_metrics);
  emit_complete_route(output, "route.float64_oracle.validation",
                      oracle_validation_metrics);
  emit_complete_route(output, "route.direct_float32.train",
                      direct_train_metrics);
  emit_complete_route(output, "route.direct_float32.validation",
                      direct_validation_metrics);
  emit_complete_route(output, "route.affine_warm_start_step0.train",
                      step0_train_metrics);
  emit_complete_route(output, "route.affine_warm_start_step0.validation",
                      step0_validation_metrics);
  emit_complete_route(output, "route.affine_warm_start_final.train",
                      final_train_metrics);
  emit_complete_route(output, "route.affine_warm_start_final.validation",
                      final_validation_metrics);

  output << "step0_parity_tolerance_standardized_target_units="
         << kWarmStartStep0ParityTolerance << '\n';
  output << "step0_train_max_abs_delta_standardized_target_units="
         << step0_train_delta << '\n';
  output << "step0_validation_max_abs_delta_standardized_target_units="
         << step0_validation_delta << '\n';
  output << "step0_train_parity_pass="
         << (step0_train_parity_pass ? "true" : "false") << '\n';
  output << "step0_validation_parity_pass="
         << (step0_validation_parity_pass ? "true" : "false") << '\n';
  output << "step0_parity_gate_pass="
         << (step0_parity_gate_pass ? "true" : "false") << '\n';

  output << "train_aggregate_standardized_mse_ratio_to_oracle="
         << train_aggregate_mse_ratio << '\n';
  output << "train_aggregate_standardized_mse_ratio_limit="
         << kTrainAggregateMseRatioLimit << '\n';
  output << "train_aggregate_standardized_mse_ratio_pass="
         << (train_aggregate_mse_ratio_pass ? "true" : "false") << '\n';
  output << "train_head_standardized_mse_ratio_limit="
         << kTrainHeadMseRatioLimit << '\n';
  for (int64_t head = 0; head < kHeadCount; ++head) {
    const int64_t edge = head % kEdgeCount;
    const int64_t channel = head / kEdgeCount;
    const double ratio =
        train_head_mse_ratios[static_cast<std::size_t>(head)];
    output << "train_head_" << head << ".edge_" << edge << ".channel_"
           << channel << ".standardized_mse_ratio_to_oracle=" << ratio
           << '\n';
    output << "train_head_" << head << ".edge_" << edge << ".channel_"
           << channel << ".standardized_mse_ratio_pass="
           << (ratio <= kTrainHeadMseRatioLimit ? "true" : "false") << '\n';
  }
  output << "train_maximum_head_standardized_mse_ratio_to_oracle="
         << maximum_train_head_mse_ratio << '\n';
  output << "train_every_head_standardized_mse_ratio_pass="
         << (every_train_head_mse_ratio_pass ? "true" : "false") << '\n';
  output << "metric_deficit_limit=" << kMetricDeficitLimit << '\n';
  output << "rmse_target_rms_ratio_increase_limit="
         << kRmseRatioIncreaseLimit << '\n';
  output << "train_direction_deficit_to_oracle=" << train_direction_deficit
         << '\n';
  output << "train_direction_pass="
         << (train_direction_pass ? "true" : "false") << '\n';
  output << "train_rank_deficit_to_oracle=" << train_rank_deficit << '\n';
  output << "train_rank_pass=" << (train_rank_pass ? "true" : "false")
         << '\n';
  output << "train_correlation_deficit_to_oracle="
         << train_correlation_deficit << '\n';
  output << "train_correlation_pass="
         << (train_correlation_pass ? "true" : "false") << '\n';
  output << "train_rmse_target_rms_ratio_increase_to_oracle="
         << train_rmse_ratio_increase << '\n';
  output << "train_rmse_target_rms_ratio_pass="
         << (train_rmse_ratio_pass ? "true" : "false") << '\n';
  output << "train_preservation_gate_pass="
         << (train_preservation_gate_pass ? "true" : "false") << '\n';

  output << "validation_direction_deficit_to_direct_float32="
         << validation_direction_deficit << '\n';
  output << "validation_direction_pass="
         << (validation_direction_pass ? "true" : "false") << '\n';
  output << "validation_rank_deficit_to_direct_float32="
         << validation_rank_deficit << '\n';
  output << "validation_rank_pass="
         << (validation_rank_pass ? "true" : "false") << '\n';
  output << "validation_correlation_deficit_to_direct_float32="
         << validation_correlation_deficit << '\n';
  output << "validation_correlation_pass="
         << (validation_correlation_pass ? "true" : "false") << '\n';
  output << "validation_rmse_target_rms_ratio_increase_to_direct_float32="
         << validation_rmse_ratio_increase << '\n';
  output << "validation_rmse_target_rms_ratio_pass="
         << (validation_rmse_ratio_pass ? "true" : "false") << '\n';
  output << "validation_guard_gate_pass="
         << (validation_guard_gate_pass ? "true" : "false") << '\n';
  output << "warm_start_stability_gate_pass="
         << (warm_start_stability_gate_pass ? "true" : "false") << '\n';
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
  output << "train_preservation_gate=train_aggregate_standardized_mse_ratio_"
            "to_oracle<=1.05,every_train_head_standardized_mse_ratio_to_"
            "oracle<=1.10,train_direction>=oracle-0.01,train_rank>=oracle-"
            "0.01,train_correlation>=oracle-0.01,train_rmse_target_rms_ratio"
            "<=oracle+0.05\n";
  output << "validation_guard_gate=validation_direction>=direct_float32-0."
            "01,validation_rank>=direct_float32-0.01,validation_correlation>"
            "=direct_float32-0.01,validation_rmse_target_rms_ratio<=direct_"
            "float32+0.05\n";
  output << "clear_stop_gate=step0_parity_and_not_stable_and_(train_aggregate_"
            "standardized_mse_ratio_to_oracle>=1.25_or_train_maximum_head_"
            "standardized_mse_ratio_to_oracle>=1.50)\n";

  output << "affine_oracle_grouped_fit_count=1\n";
  output << "affine_oracle_head_solve_count=9\n";
  output << "optimizer_fits_completed=1\n";
  output << "total_train_fit_procedures=2\n";
  output << "optimizer_steps_completed="
         << training.optimizer_steps_completed << '\n';
  output << "seed=" << kSeed << '\n';
  output << "steps_per_fit=" << kSteps << '\n';
  output << "batch_size=" << kBatchSize << '\n';
  output << "learning_rate=" << kLearningRate << '\n';
  output << "adam_beta1=" << kAdamBeta1 << '\n';
  output << "adam_beta2=" << kAdamBeta2 << '\n';
  output << "adam_epsilon=" << kAdamEpsilon << '\n';
  output << "weight_decay=" << kWeightDecay << '\n';
  output << "gradient_clip_norm=" << kGradientClipNorm << '\n';
  output << "optimizer=Adam\n";
  output << "batch_sampling=mt19937_64_uniform_with_replacement\n";
  output << "batch_schedule_fingerprint=" << schedule.fingerprint << '\n';
  output << "early_stopping=false\n";
  output << "seed_selection=false\n";
  output << "hyperparameter_search=false\n";
  output << "retry=false\n";
  output << "refit=false\n";
  output << "warm_start.initial_full_train_standardized_mse="
         << training.initial_full_train_standardized_mse << '\n';
  output << "warm_start.final_full_train_standardized_mse="
         << training.final_full_train_standardized_mse << '\n';
  output << "warm_start.last_minibatch_loss="
         << training.last_minibatch_loss << '\n';
  output << "warm_start.maximum_preclip_gradient_norm="
         << training.maximum_preclip_gradient_norm << '\n';
  output << "warm_start.clipped_step_count=" << training.clipped_step_count
         << '\n';
  if (!output) {
    local_fail("failed while rendering affine warm-start report");
  }
  write_exclusive(options.output, output.str());
}

} // namespace

int main(int argc, char **argv) {
  try {
    std::locale::global(std::locale::classic());
    run_warm_start(parse_local_options(argc, argv));
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "frozen representation affine warm-start stability probe: "
              << error.what() << '\n';
    return 1;
  }
}
