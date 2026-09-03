#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"

#include <ATen/Context.h>
#include <ATen/ops/cholesky_solve.h>
#include <ATen/ops/linalg_cholesky_ex.h>
#include <torch/torch.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;

namespace {

constexpr int64_t kChannels = 3;
constexpr int64_t kHistory = 24;
constexpr int64_t kFeatures = 3;
constexpr int64_t kInstruments = 8;
constexpr int64_t kTrainGroups = 256;
constexpr int64_t kHoldoutGroups = 128;
constexpr int64_t kTrainSamples = kTrainGroups * kInstruments;
constexpr int64_t kHoldoutSamples = kHoldoutGroups * kInstruments;
constexpr int64_t kBatchSize = 64;
constexpr int64_t kTrainingSteps = 256;
constexpr int64_t kFixedObjectiveBatches = 4;
constexpr uint64_t kGeneratorSeed = 0x6d74665f7175616cULL;
constexpr std::array<int64_t, kChannels> kChannelWidths{1, 3, 7};
constexpr std::array<int64_t, 3> kModelSeeds{17, 31, 47};
constexpr std::array<double, 3> kRidges{1.0e-5, 1.0e-4, 1.0e-3};
constexpr std::array<const char *, 3> kRidgeNames{"1e_5", "1e_4", "1e_3"};
constexpr int64_t kPrimaryRidge = 1;
constexpr std::array<mtf::mtf_serving_pool_policy_t, 4> kServingPolicies{
    mtf::mtf_serving_pool_policy_t::all_tokens,
    mtf::mtf_serving_pool_policy_t::time_only,
    mtf::mtf_serving_pool_policy_t::frequency_only,
    mtf::mtf_serving_pool_policy_t::domain_balanced};
constexpr double kPi = 3.141592653589793238462643383279502884;

struct Dataset {
  torch::Tensor data{};   // [S,C,H,F], float32
  torch::Tensor mask{};   // [S,C,H,F], bool
  torch::Tensor target{}; // [S,C], float64
  int64_t group_begin{0};
  int64_t groups{0};
};

struct GeneratorGuards {
  bool finite{false};
  bool sign_balance{false};
  bool pairwise_ties{false};
  bool split_disjoint{false};
  double minimum_positive_fraction{0.0};
  double maximum_positive_fraction{0.0};
  double pairwise_tie_fraction{1.0};

  [[nodiscard]] bool passed() const {
    return finite && sign_balance && pairwise_ties && split_disjoint;
  }
};

struct MetricSummary {
  int64_t count{0};
  int64_t direction_count{0};
  int64_t pair_count{0};
  double direction{0.0};
  double rank{0.0};
  double correlation{0.0};
  double rmse{0.0};
  double target_rms{0.0};
  double rmse_target_rms_ratio{std::numeric_limits<double>::infinity()};
};

struct RidgeModel {
  torch::Tensor mean{};    // [C,D]
  torch::Tensor inv_std{}; // [C,D]
  torch::Tensor weights{}; // [C,D]
  torch::Tensor bias{};    // [C]
  double alpha{0.0};
  double coefficient_l2_norm{0.0};
  double maximum_normalized_residual{0.0};
};

struct RidgeEvaluation {
  double alpha{0.0};
  RidgeModel model{};
  MetricSummary train{};
  MetricSummary holdout{};
  std::array<MetricSummary, kChannels> train_channels{};
  std::array<MetricSummary, kChannels> holdout_channels{};
};

struct SurfaceResult {
  std::string name{};
  std::array<RidgeEvaluation, kRidges.size()> ridges{};
  bool ridge_stable_strong{false};
  bool partial{false};
};

struct ExtractedSurfaces {
  std::array<torch::Tensor, kServingPolicies.size()> served{}; // [S,C,De]
  torch::Tensor prepool{}; // [S,C,Nc*De], optional
};

struct MetricComparison {
  double trained_to_untrained_mse_ratio{
      std::numeric_limits<double>::infinity()};
  double direction_delta{0.0};
  double rank_delta{0.0};
  double correlation_delta{0.0};
  bool benefit{false};
  bool regression{false};
};

struct LearningDiagnosis {
  std::array<MetricComparison, kRidges.size()> ridge_aggregate{};
  std::array<MetricComparison, kChannels> primary_channels{};
  int ridge_benefit_count{0};
  int ridge_regression_count{0};
  int primary_channel_benefit_count{0};
  int primary_channel_regression_count{0};
  bool learning_benefit{false};
  bool training_regression{false};
};

struct MechanicsResult {
  int64_t completed_steps{0};
  double initial_fixed_objective{std::numeric_limits<double>::quiet_NaN()};
  double final_fixed_objective{std::numeric_limits<double>::quiet_NaN()};
  double objective_ratio{std::numeric_limits<double>::infinity()};
  double parameter_delta_l2{0.0};
  bool losses_finite{false};
  bool gradients_finite{false};
  bool parameters_finite{false};
  bool parameter_changed{false};
  bool objective_improved{false};

  [[nodiscard]] bool passed() const {
    return completed_steps == kTrainingSteps && losses_finite &&
           gradients_finite && parameters_finite && parameter_changed &&
           objective_improved;
  }
};

struct SeedResult {
  int64_t seed{0};
  MechanicsResult mechanics{};
  SurfaceResult untrained_prepool{};
  SurfaceResult trained_prepool{};
  std::array<SurfaceResult, kServingPolicies.size()> untrained_served{};
  std::array<SurfaceResult, kServingPolicies.size()> trained_served{};
  LearningDiagnosis prepool_diagnosis{};
  std::array<LearningDiagnosis, kServingPolicies.size()> policy_diagnoses{};
  bool any_policy_benefit{false};
  bool any_policy_regression{false};
  bool all_tokens_policy_specific_harm{false};
  bool prepool_only_capacity_without_learning{false};
};

[[nodiscard]] uint64_t splitmix64(uint64_t value) {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

[[nodiscard]] double unit_interval(uint64_t value) {
  return static_cast<double>(splitmix64(value) >> 11U) *
         (1.0 / 9007199254740992.0);
}

[[nodiscard]] uint64_t generator_key(int64_t group, int64_t instrument,
                                     int64_t component, uint64_t salt) {
  uint64_t key = kGeneratorSeed ^ salt;
  key ^= splitmix64(static_cast<uint64_t>(group) + 0x100000001b3ULL);
  key ^= splitmix64(static_cast<uint64_t>(instrument) + 0x9e3779b9ULL);
  key ^= splitmix64(static_cast<uint64_t>(component) + 0x85ebca6bULL);
  return splitmix64(key);
}

[[nodiscard]] double latent_return(int64_t group, int64_t instrument,
                                   int64_t time) {
  constexpr std::array<double, 3> periods{37.0, 19.0, 11.0};
  constexpr std::array<double, 3> gains{1.0, 0.65, 0.35};
  double value = 0.0;
  for (int64_t component = 0; component < 3; ++component) {
    const double amplitude =
        0.75 + 0.50 * unit_interval(generator_key(group, instrument, component,
                                                  0xa11ce5eedULL));
    const double phase = 2.0 * kPi *
                         unit_interval(generator_key(
                             group, instrument, component, 0xbadc0ffeeULL));
    value += gains[static_cast<std::size_t>(component)] * amplitude *
             std::sin(2.0 * kPi * static_cast<double>(time) /
                          periods[static_cast<std::size_t>(component)] +
                      phase);
  }
  return value;
}

[[nodiscard]] Dataset generate_dataset(int64_t group_begin, int64_t groups) {
  const int64_t samples = groups * kInstruments;
  auto data = torch::empty(
      {samples, kChannels, kHistory, kFeatures},
      torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU));
  auto target = torch::empty(
      {samples, kChannels},
      torch::TensorOptions().dtype(torch::kFloat64).device(torch::kCPU));
  auto data_a = data.accessor<float, 4>();
  auto target_a = target.accessor<double, 2>();

  for (int64_t local_group = 0; local_group < groups; ++local_group) {
    const int64_t group = group_begin + local_group;
    for (int64_t instrument = 0; instrument < kInstruments; ++instrument) {
      const int64_t sample = local_group * kInstruments + instrument;
      for (int64_t channel = 0; channel < kChannels; ++channel) {
        const int64_t width = kChannelWidths[static_cast<std::size_t>(channel)];
        const double inv_sqrt_width =
            1.0 / std::sqrt(static_cast<double>(width));
        for (int64_t history = 0; history < kHistory; ++history) {
          const int64_t begin = (history - kHistory) * width;
          double sum = 0.0;
          double absolute_sum = 0.0;
          double square_sum = 0.0;
          for (int64_t offset = 0; offset < width; ++offset) {
            const double value =
                latent_return(group, instrument, begin + offset);
            sum += value;
            absolute_sum += std::fabs(value);
            square_sum += value * value;
          }
          data_a[sample][channel][history][0] =
              static_cast<float>(sum * inv_sqrt_width);
          data_a[sample][channel][history][1] =
              static_cast<float>(absolute_sum / static_cast<double>(width));
          data_a[sample][channel][history][2] = static_cast<float>(
              std::sqrt(square_sum / static_cast<double>(width)));
        }
        double future = 0.0;
        for (int64_t offset = 0; offset < width; ++offset) {
          future += latent_return(group, instrument, offset);
        }
        target_a[sample][channel] = future * inv_sqrt_width;
      }
    }
  }

  return {.data = std::move(data),
          .mask = torch::ones(
              {samples, kChannels, kHistory, kFeatures},
              torch::TensorOptions().dtype(torch::kBool).device(torch::kCPU)),
          .target = std::move(target),
          .group_begin = group_begin,
          .groups = groups};
}

void normalize_inputs(Dataset &train, Dataset &holdout) {
  const auto arranged = train.data.permute({1, 3, 0, 2})
                            .contiguous()
                            .view({kChannels, kFeatures, -1});
  const auto mean = arranged.mean(/*dim=*/2);
  const auto variance = (arranged - mean.unsqueeze(-1)).pow(2).mean(/*dim=*/2);
  const auto std = variance.sqrt();
  const auto inv_std =
      torch::where(std > 1.0e-8, std.reciprocal(), torch::ones_like(std));
  const auto mean_view = mean.view({1, kChannels, 1, kFeatures});
  const auto inv_view = inv_std.view({1, kChannels, 1, kFeatures});
  train.data = ((train.data - mean_view) * inv_view).contiguous();
  holdout.data = ((holdout.data - mean_view) * inv_view).contiguous();
}

[[nodiscard]] double positive_fraction(const torch::Tensor &target,
                                       int64_t channel) {
  return target.select(1, channel)
      .gt(0.0)
      .to(torch::kFloat64)
      .mean()
      .item<double>();
}

[[nodiscard]] GeneratorGuards validate_generator(const Dataset &train,
                                                 const Dataset &holdout) {
  GeneratorGuards guards{};
  guards.finite = torch::isfinite(train.data).all().item<bool>() &&
                  torch::isfinite(train.target).all().item<bool>() &&
                  torch::isfinite(holdout.data).all().item<bool>() &&
                  torch::isfinite(holdout.target).all().item<bool>();
  guards.minimum_positive_fraction = 1.0;
  guards.maximum_positive_fraction = 0.0;
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    for (const Dataset *dataset : {&train, &holdout}) {
      const double fraction = positive_fraction(dataset->target, channel);
      guards.minimum_positive_fraction =
          std::min(guards.minimum_positive_fraction, fraction);
      guards.maximum_positive_fraction =
          std::max(guards.maximum_positive_fraction, fraction);
    }
  }
  guards.sign_balance = guards.minimum_positive_fraction >= 0.45 &&
                        guards.maximum_positive_fraction <= 0.55;

  int64_t comparisons = 0;
  int64_t ties = 0;
  for (const Dataset *dataset : {&train, &holdout}) {
    const auto target = dataset->target.accessor<double, 2>();
    for (int64_t group = 0; group < dataset->groups; ++group) {
      for (int64_t channel = 0; channel < kChannels; ++channel) {
        for (int64_t lhs = 0; lhs < kInstruments; ++lhs) {
          for (int64_t rhs = lhs + 1; rhs < kInstruments; ++rhs) {
            ++comparisons;
            const int64_t lhs_row = group * kInstruments + lhs;
            const int64_t rhs_row = group * kInstruments + rhs;
            if (std::fabs(target[lhs_row][channel] -
                          target[rhs_row][channel]) <= 1.0e-10) {
              ++ties;
            }
          }
        }
      }
    }
  }
  guards.pairwise_tie_fraction =
      comparisons > 0
          ? static_cast<double>(ties) / static_cast<double>(comparisons)
          : 1.0;
  guards.pairwise_ties = guards.pairwise_tie_fraction < 0.01;
  guards.split_disjoint = true;
  for (int64_t train_group = 0; train_group < train.groups; ++train_group) {
    for (int64_t holdout_group = 0; holdout_group < holdout.groups;
         ++holdout_group) {
      if (train.group_begin + train_group ==
          holdout.group_begin + holdout_group) {
        guards.split_disjoint = false;
      }
    }
  }
  return guards;
}

[[nodiscard]] int sign(double value) { return (value > 0.0) - (value < 0.0); }

[[nodiscard]] MetricSummary metrics(const torch::Tensor &prediction_input,
                                    const torch::Tensor &target_input,
                                    int64_t groups, int64_t only_channel = -1) {
  const auto prediction =
      prediction_input.to(torch::kCPU, torch::kFloat64).contiguous();
  const auto target =
      target_input.to(torch::kCPU, torch::kFloat64).contiguous();
  if (prediction.sizes() != target.sizes() || prediction.dim() != 2 ||
      prediction.size(0) != groups * kInstruments ||
      prediction.size(1) != kChannels || only_channel >= kChannels) {
    throw std::runtime_error("metric tensor contract mismatch");
  }
  const auto p = prediction.accessor<double, 2>();
  const auto y = target.accessor<double, 2>();
  const int64_t channel_begin = only_channel >= 0 ? only_channel : 0;
  const int64_t channel_end = only_channel >= 0 ? only_channel + 1 : kChannels;
  double prediction_sum = 0.0;
  double target_sum = 0.0;
  double prediction_square_sum = 0.0;
  double target_square_sum = 0.0;
  double cross_sum = 0.0;
  double square_error_sum = 0.0;
  int64_t count = 0;
  int64_t direction_count = 0;
  int64_t direction_correct = 0;
  int64_t pair_count = 0;
  int64_t pair_correct = 0;

  for (int64_t group = 0; group < groups; ++group) {
    for (int64_t channel = channel_begin; channel < channel_end; ++channel) {
      for (int64_t instrument = 0; instrument < kInstruments; ++instrument) {
        const int64_t row = group * kInstruments + instrument;
        const double predicted = p[row][channel];
        const double realized = y[row][channel];
        if (!std::isfinite(predicted) || !std::isfinite(realized)) {
          throw std::runtime_error("non-finite metric value");
        }
        ++count;
        const double error = predicted - realized;
        square_error_sum += error * error;
        prediction_sum += predicted;
        target_sum += realized;
        prediction_square_sum += predicted * predicted;
        target_square_sum += realized * realized;
        cross_sum += predicted * realized;
        if (std::fabs(realized) > 1.0e-12) {
          ++direction_count;
          if (sign(predicted) == sign(realized)) {
            ++direction_correct;
          }
        }
      }
      for (int64_t lhs = 0; lhs < kInstruments; ++lhs) {
        for (int64_t rhs = lhs + 1; rhs < kInstruments; ++rhs) {
          const int64_t lhs_row = group * kInstruments + lhs;
          const int64_t rhs_row = group * kInstruments + rhs;
          const double realized_difference =
              y[lhs_row][channel] - y[rhs_row][channel];
          if (std::fabs(realized_difference) <= 1.0e-12) {
            continue;
          }
          ++pair_count;
          if (sign(p[lhs_row][channel] - p[rhs_row][channel]) ==
              sign(realized_difference)) {
            ++pair_correct;
          }
        }
      }
    }
  }

  if (count <= 1 || direction_count <= 0 || pair_count <= 0) {
    throw std::runtime_error("empty metric domain");
  }
  const double count_d = static_cast<double>(count);
  const double prediction_variance =
      prediction_square_sum - prediction_sum * prediction_sum / count_d;
  const double target_variance =
      target_square_sum - target_sum * target_sum / count_d;
  const double covariance = cross_sum - prediction_sum * target_sum / count_d;
  const double correlation =
      prediction_variance > 0.0 && target_variance > 0.0
          ? covariance / std::sqrt(prediction_variance * target_variance)
          : 0.0;
  const double rmse = std::sqrt(square_error_sum / count_d);
  const double target_rms = std::sqrt(target_square_sum / count_d);
  return {.count = count,
          .direction_count = direction_count,
          .pair_count = pair_count,
          .direction = static_cast<double>(direction_correct) /
                       static_cast<double>(direction_count),
          .rank = static_cast<double>(pair_correct) /
                  static_cast<double>(pair_count),
          .correlation = correlation,
          .rmse = rmse,
          .target_rms = target_rms,
          .rmse_target_rms_ratio =
              target_rms > 0.0 ? rmse / target_rms
                               : std::numeric_limits<double>::infinity()};
}

[[nodiscard]] bool finite_metric(const MetricSummary &metric) {
  return metric.count > 0 && metric.direction_count > 0 &&
         metric.pair_count > 0 && std::isfinite(metric.direction) &&
         std::isfinite(metric.rank) && std::isfinite(metric.correlation) &&
         std::isfinite(metric.rmse) && std::isfinite(metric.target_rms) &&
         std::isfinite(metric.rmse_target_rms_ratio);
}

[[nodiscard]] bool strong(const MetricSummary &metric) {
  return finite_metric(metric) && metric.direction >= 0.95 &&
         metric.rank >= 0.95 && metric.correlation >= 0.95 &&
         metric.rmse_target_rms_ratio <= 0.25;
}

[[nodiscard]] bool channel_floor(const MetricSummary &metric) {
  return finite_metric(metric) && metric.direction >= 0.90 &&
         metric.rank >= 0.90 && metric.correlation >= 0.90 &&
         metric.rmse_target_rms_ratio <= 0.35;
}

[[nodiscard]] bool partial(const MetricSummary &metric) {
  return finite_metric(metric) && metric.direction >= 0.80 &&
         metric.rank >= 0.78;
}

[[nodiscard]] bool raw_tight(const MetricSummary &metric) {
  return finite_metric(metric) && metric.direction >= 0.98 &&
         metric.rank >= 0.98 && metric.correlation >= 0.995 &&
         metric.rmse_target_rms_ratio <= 0.10;
}

[[nodiscard]] MetricComparison compare_metrics(const MetricSummary &trained,
                                               const MetricSummary &untrained) {
  MetricComparison result{};
  if (!finite_metric(trained) || !finite_metric(untrained) ||
      !(untrained.rmse > 0.0)) {
    return result;
  }
  result.trained_to_untrained_mse_ratio =
      (trained.rmse * trained.rmse) / (untrained.rmse * untrained.rmse);
  result.direction_delta = trained.direction - untrained.direction;
  result.rank_delta = trained.rank - untrained.rank;
  result.correlation_delta = trained.correlation - untrained.correlation;
  result.benefit = result.trained_to_untrained_mse_ratio <= 0.80 &&
                   result.direction_delta >= -0.02 &&
                   result.rank_delta >= -0.02 &&
                   result.correlation_delta >= -0.02;
  result.regression = result.trained_to_untrained_mse_ratio >= 1.25 &&
                      result.correlation_delta <= -0.05;
  return result;
}

[[nodiscard]] LearningDiagnosis
diagnose_learning(const SurfaceResult &trained,
                  const SurfaceResult &untrained) {
  LearningDiagnosis result{};
  for (std::size_t ridge = 0; ridge < kRidges.size(); ++ridge) {
    result.ridge_aggregate[ridge] = compare_metrics(
        trained.ridges[ridge].holdout, untrained.ridges[ridge].holdout);
    result.ridge_benefit_count += result.ridge_aggregate[ridge].benefit ? 1 : 0;
    result.ridge_regression_count +=
        result.ridge_aggregate[ridge].regression ? 1 : 0;
  }
  for (std::size_t channel = 0; channel < kChannels; ++channel) {
    result.primary_channels[channel] = compare_metrics(
        trained.ridges[kPrimaryRidge].holdout_channels[channel],
        untrained.ridges[kPrimaryRidge].holdout_channels[channel]);
    result.primary_channel_benefit_count +=
        result.primary_channels[channel].benefit ? 1 : 0;
    result.primary_channel_regression_count +=
        result.primary_channels[channel].regression ? 1 : 0;
  }
  result.learning_benefit = result.ridge_benefit_count >= 2 &&
                            result.primary_channel_benefit_count >= 2;
  result.training_regression = result.ridge_regression_count >= 2 &&
                               result.primary_channel_regression_count >= 2;
  return result;
}

[[nodiscard]] RidgeModel fit_ridge(const torch::Tensor &features_input,
                                   const torch::Tensor &target_input,
                                   double alpha) {
  torch::NoGradGuard no_grad;
  const auto features =
      features_input.to(torch::kCPU, torch::kFloat64).contiguous();
  const auto target =
      target_input.to(torch::kCPU, torch::kFloat64).contiguous();
  if (features.dim() != 3 || features.size(0) != target.size(0) ||
      features.size(1) != kChannels || target.dim() != 2 ||
      target.size(1) != kChannels || !(alpha > 0.0)) {
    throw std::runtime_error("ridge fit tensor contract mismatch");
  }
  const int64_t rows = features.size(0);
  const int64_t width = features.size(2);
  const auto mean = features.mean(/*dim=*/0);
  const auto variance = (features - mean.unsqueeze(0)).pow(2).mean(/*dim=*/0);
  const auto std = variance.sqrt();
  const auto inv_std =
      torch::where(std > 1.0e-12, std.reciprocal(), torch::ones_like(std));
  const auto standardized =
      (features - mean.unsqueeze(0)) * inv_std.unsqueeze(0);
  auto weights = torch::zeros({kChannels, width}, torch::kFloat64);
  auto bias = torch::zeros({kChannels}, torch::kFloat64);
  double maximum_residual = 0.0;

  for (int64_t channel = 0; channel < kChannels; ++channel) {
    const auto x = standardized.select(1, channel).contiguous();
    const auto y = target.select(1, channel).contiguous();
    const auto x_mean = x.mean(/*dim=*/0);
    const auto y_mean = y.mean();
    const auto centered_x = x - x_mean;
    const auto centered_y = y - y_mean;
    auto gram = centered_x.transpose(0, 1).matmul(centered_x);
    gram.diagonal(0, 0, 1).add_(static_cast<double>(rows) * alpha);
    const auto rhs = centered_x.transpose(0, 1).matmul(centered_y.unsqueeze(1));
    auto [cholesky, info] = at::linalg_cholesky_ex(gram, false, false);
    if (info.item<int64_t>() != 0) {
      throw std::runtime_error("ridge Cholesky factorization failed");
    }
    const auto row = at::cholesky_solve(rhs, cholesky, false).squeeze(1);
    const auto residual = gram.matmul(row.unsqueeze(1)) - rhs;
    const double normalized_residual =
        residual.norm().item<double>() /
        std::max(rhs.norm().item<double>(), 1.0e-30);
    if (!torch::isfinite(row).all().item<bool>() ||
        !std::isfinite(normalized_residual) || normalized_residual > 1.0e-6) {
      throw std::runtime_error("ridge solve residual or finiteness failed");
    }
    maximum_residual = std::max(maximum_residual, normalized_residual);
    weights.select(0, channel).copy_(row);
    bias.select(0, channel).copy_(y_mean - x_mean.dot(row));
  }
  return {.mean = mean,
          .inv_std = inv_std,
          .weights = weights,
          .bias = bias,
          .alpha = alpha,
          .coefficient_l2_norm = weights.norm().item<double>(),
          .maximum_normalized_residual = maximum_residual};
}

[[nodiscard]] torch::Tensor predict_ridge(const RidgeModel &model,
                                          const torch::Tensor &features_input) {
  torch::NoGradGuard no_grad;
  const auto features = features_input.to(torch::kCPU, torch::kFloat64);
  const auto standardized =
      (features - model.mean.unsqueeze(0)) * model.inv_std.unsqueeze(0);
  return (standardized * model.weights.unsqueeze(0)).sum(/*dim=*/2) +
         model.bias.unsqueeze(0);
}

[[nodiscard]] SurfaceResult
qualify_surface(std::string name, const torch::Tensor &train_features,
                const torch::Tensor &holdout_features,
                const torch::Tensor &train_target,
                const torch::Tensor &holdout_target) {
  if (train_features.dim() != 3 || holdout_features.dim() != 3 ||
      train_features.size(0) != kTrainSamples ||
      holdout_features.size(0) != kHoldoutSamples ||
      train_features.size(1) != kChannels ||
      holdout_features.size(1) != kChannels ||
      train_features.size(2) != holdout_features.size(2)) {
    throw std::runtime_error("surface feature contract mismatch");
  }
  SurfaceResult result{};
  result.name = std::move(name);
  for (std::size_t index = 0; index < kRidges.size(); ++index) {
    auto model = fit_ridge(train_features, train_target, kRidges[index]);
    const auto train_prediction = predict_ridge(model, train_features);
    const auto holdout_prediction = predict_ridge(model, holdout_features);
    RidgeEvaluation evaluation{};
    evaluation.alpha = kRidges[index];
    evaluation.model = std::move(model);
    evaluation.train = metrics(train_prediction, train_target, kTrainGroups);
    evaluation.holdout =
        metrics(holdout_prediction, holdout_target, kHoldoutGroups);
    for (int64_t channel = 0; channel < kChannels; ++channel) {
      evaluation.train_channels[static_cast<std::size_t>(channel)] =
          metrics(train_prediction, train_target, kTrainGroups, channel);
      evaluation.holdout_channels[static_cast<std::size_t>(channel)] =
          metrics(holdout_prediction, holdout_target, kHoldoutGroups, channel);
    }
    result.ridges[index] = std::move(evaluation);
  }
  const auto &primary = result.ridges[kPrimaryRidge];
  bool channels_pass = true;
  for (const auto &channel : primary.holdout_channels) {
    channels_pass = channels_pass && channel_floor(channel);
  }
  result.ridge_stable_strong =
      strong(primary.holdout) && channels_pass &&
      (strong(result.ridges[0].holdout) || strong(result.ridges[2].holdout));
  result.partial = partial(primary.holdout);
  return result;
}

[[nodiscard]] bool raw_harness_pass(const SurfaceResult &raw) {
  const auto &primary = raw.ridges[kPrimaryRidge];
  bool channels_pass = true;
  for (const auto &channel : primary.holdout_channels) {
    channels_pass = channels_pass && channel.correlation >= 0.99 &&
                    channel.rmse_target_rms_ratio <= 0.15 &&
                    channel.direction >= 0.95 && channel.rank >= 0.95;
  }
  return raw_tight(primary.holdout) && channels_pass &&
         (raw_tight(raw.ridges[0].holdout) || raw_tight(raw.ridges[2].holdout));
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t qualification_config() {
  mtf::mtf_jepa_mae_vicreg_config_t config{};
  config.channel_count = kChannels;
  config.history_length = kHistory;
  config.input_width = kFeatures;
  config.d_model = 12;
  config.latent_dim = 8;
  config.projector_dim = 16;
  config.predictor_hidden_dim = 16;
  config.num_encoder_layers = 1;
  config.num_predictor_layers = 1;
  config.num_decoder_layers = 1;
  config.num_heads = 2;
  config.dropout = 0.0;
  config.time_scales = {6, 12, 24};
  config.scale_strides = {6, 12, 24};
  config.frequency_num_bins = 6;
  // Keep the architecture compact, but exercise the active train-core policy.
  config.mask_ratio_time = 0.10;
  config.mask_ratio_frequency = 0.05;
  config.mask_ratio_channel = 0.0;
  config.min_context_ratio = 0.75;
  config.lambda_jepa = 1.0;
  config.lambda_mae = 0.25;
  config.lambda_tf_align = 0.10;
  config.lambda_vicreg = 0.05;
  config.use_global_vicreg = true;
  config.use_channel_vicreg = false;
  config.lambda_global_vicreg = 0.25;
  config.lambda_channel_vicreg = 1.0;
  config.target_ema_tau = 0.990;
  config.couple_time_frequency_masks = false;
  config.mask_same_window_across_domains = false;
  config.mask_same_channel_block = false;
  config.max_context_target_time_overlap = 0.50;
  config.augmentation_profile = "light_phase_safe_v2";
  config.gaussian_jitter_std = 0.001;
  config.time_dilation_min = 0.98;
  config.time_dilation_max = 1.02;
  config.time_warp_max = 0.01;
  config.amplitude_scale_min = 0.98;
  config.amplitude_scale_max = 1.02;
  config.frequency_mask_ratio = 0.02;
  config.frequency_jitter_std = 0.01;
  config.dtype = torch::kFloat32;
  config.device = torch::Device(torch::kCPU);
  return config;
}

[[nodiscard]] std::array<std::vector<int64_t>, kChannels>
ordered_channel_token_indices(const mtf::mtf_token_metadata_t &metadata,
                              int64_t token_count) {
  const auto channels =
      metadata.channel_id.to(torch::kCPU, torch::kInt64).contiguous();
  const auto domains =
      metadata.domain_id.to(torch::kCPU, torch::kInt64).contiguous();
  const auto scales =
      metadata.scale_id.to(torch::kCPU, torch::kInt64).contiguous();
  const auto starts =
      metadata.start_index.to(torch::kCPU, torch::kInt64).contiguous();
  const auto widths =
      metadata.width.to(torch::kCPU, torch::kInt64).contiguous();
  if (channels.numel() != token_count || domains.numel() != token_count ||
      scales.numel() != token_count || starts.numel() != token_count ||
      widths.numel() != token_count) {
    throw std::runtime_error("token metadata width mismatch");
  }
  const auto channel_a = channels.accessor<int64_t, 1>();
  const auto domain_a = domains.accessor<int64_t, 1>();
  const auto scale_a = scales.accessor<int64_t, 1>();
  const auto start_a = starts.accessor<int64_t, 1>();
  const auto width_a = widths.accessor<int64_t, 1>();
  std::array<std::vector<int64_t>, kChannels> indices{};
  for (int64_t token = 0; token < token_count; ++token) {
    const int64_t channel = channel_a[token];
    if (channel < 0 || channel >= kChannels) {
      throw std::runtime_error("token channel id out of range");
    }
    indices[static_cast<std::size_t>(channel)].push_back(token);
  }
  for (auto &channel_indices : indices) {
    std::sort(channel_indices.begin(), channel_indices.end(),
              [&](int64_t lhs, int64_t rhs) {
                return std::tuple{domain_a[lhs], scale_a[lhs], start_a[lhs],
                                  width_a[lhs], lhs} <
                       std::tuple{domain_a[rhs], scale_a[rhs], start_a[rhs],
                                  width_a[rhs], rhs};
              });
    if (channel_indices.empty()) {
      throw std::runtime_error("channel has no encoded tokens");
    }
  }
  const std::size_t expected = indices[0].size();
  for (const auto &channel_indices : indices) {
    if (channel_indices.size() != expected) {
      throw std::runtime_error("per-channel token counts differ");
    }
  }
  return indices;
}

[[nodiscard]] ExtractedSurfaces extract_surfaces(mtf::MtfJepaMaeVicreg &model,
                                                 const Dataset &dataset,
                                                 bool include_prepool) {
  const bool was_training = model->is_training();
  model->eval();
  torch::NoGradGuard no_grad;
  std::array<std::vector<torch::Tensor>, kServingPolicies.size()>
      served_chunks{};
  std::vector<torch::Tensor> prepool_chunks;
  std::array<std::vector<int64_t>, kChannels> token_indices{};
  bool have_indices = false;

  for (int64_t begin = 0; begin < dataset.data.size(0); begin += kBatchSize) {
    const int64_t size =
        std::min<int64_t>(kBatchSize, dataset.data.size(0) - begin);
    const auto encoded = model->encode(dataset.data.narrow(0, begin, size),
                                       dataset.mask.narrow(0, begin, size));
    for (std::size_t policy_index = 0; policy_index < kServingPolicies.size();
         ++policy_index) {
      const auto served = mtf::select_mtf_serving_pool(
          encoded, kServingPolicies[policy_index], model->config());
      if (!served.valid_mask.all().item<bool>() ||
          served.values.sizes() !=
              torch::IntArrayRef(
                  {size, kChannels, model->config().latent_dim})) {
        throw std::runtime_error("served policy surface is invalid");
      }
      served_chunks[policy_index].push_back(
          served.values.detach().to(torch::kCPU));
    }
    if (!include_prepool) {
      continue;
    }
    if (!have_indices) {
      token_indices = ordered_channel_token_indices(encoded.metadata,
                                                    encoded.embeddings.size(1));
      have_indices = true;
    }
    std::vector<torch::Tensor> channel_features;
    channel_features.reserve(kChannels);
    for (int64_t channel = 0; channel < kChannels; ++channel) {
      const auto &indices = token_indices[static_cast<std::size_t>(channel)];
      const auto index =
          torch::tensor(indices, torch::TensorOptions().dtype(torch::kInt64));
      const auto selected_mask = encoded.token_mask.index_select(1, index);
      if (!selected_mask.all().item<bool>()) {
        throw std::runtime_error("fully valid input produced an invalid token");
      }
      channel_features.push_back(
          encoded.embeddings.index_select(1, index).reshape({size, -1}));
    }
    prepool_chunks.push_back(
        torch::stack(channel_features, /*dim=*/1).detach().to(torch::kCPU));
  }
  model->train(was_training);
  ExtractedSurfaces result{};
  for (std::size_t policy_index = 0; policy_index < kServingPolicies.size();
       ++policy_index) {
    result.served[policy_index] =
        torch::cat(served_chunks[policy_index], /*dim=*/0).contiguous();
  }
  result.prepool = include_prepool
                       ? torch::cat(prepool_chunks, /*dim=*/0).contiguous()
                       : torch::Tensor();
  return result;
}

[[nodiscard]] double fixed_view_objective(mtf::MtfJepaMaeVicreg &model,
                                          const Dataset &holdout) {
  const bool was_training = model->is_training();
  model->eval();
  torch::NoGradGuard no_grad;
  double total = 0.0;
  for (int64_t batch = 0; batch < kFixedObjectiveBatches; ++batch) {
    const int64_t begin = batch * kBatchSize;
    torch::manual_seed(900001 + batch);
    const auto output =
        model->forward(holdout.data.narrow(0, begin, kBatchSize),
                       holdout.mask.narrow(0, begin, kBatchSize));
    const double loss = output.loss.item<double>();
    if (!std::isfinite(loss)) {
      model->train(was_training);
      return std::numeric_limits<double>::quiet_NaN();
    }
    total += loss;
  }
  model->train(was_training);
  return total / static_cast<double>(kFixedObjectiveBatches);
}

[[nodiscard]] std::vector<int64_t> epoch_permutation(int64_t seed,
                                                     int64_t epoch) {
  std::vector<int64_t> order(static_cast<std::size_t>(kTrainSamples));
  std::iota(order.begin(), order.end(), 0);
  uint64_t state =
      splitmix64(static_cast<uint64_t>(seed) ^
                 (static_cast<uint64_t>(epoch) << 32U) ^ 0x545241494eULL);
  for (int64_t index = kTrainSamples - 1; index > 0; --index) {
    state = splitmix64(state);
    const int64_t other =
        static_cast<int64_t>(state % static_cast<uint64_t>(index + 1));
    std::swap(order[static_cast<std::size_t>(index)],
              order[static_cast<std::size_t>(other)]);
  }
  return order;
}

[[nodiscard]] bool
parameters_finite(const std::vector<torch::Tensor> &parameters) {
  for (const auto &parameter : parameters) {
    if (!torch::isfinite(parameter.detach()).all().item<bool>()) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] bool
finite_gradient_norm(const std::vector<torch::Tensor> &parameters,
                     double *norm_out) {
  double square_sum = 0.0;
  bool saw_gradient = false;
  for (const auto &parameter : parameters) {
    if (!parameter.grad().defined()) {
      continue;
    }
    saw_gradient = true;
    if (!torch::isfinite(parameter.grad()).all().item<bool>()) {
      return false;
    }
    square_sum += parameter.grad().detach().pow(2).sum().item<double>();
  }
  *norm_out = std::sqrt(square_sum);
  return saw_gradient && std::isfinite(*norm_out);
}

void clip_gradients(const std::vector<torch::Tensor> &parameters, double norm,
                    double maximum) {
  if (!(norm > maximum)) {
    return;
  }
  torch::NoGradGuard no_grad;
  const double scale = maximum / std::max(norm, 1.0e-30);
  for (const auto &parameter : parameters) {
    if (parameter.grad().defined()) {
      parameter.grad().mul_(scale);
    }
  }
}

[[nodiscard]] MechanicsResult train_model(mtf::MtfJepaMaeVicreg &model,
                                          const Dataset &train,
                                          const Dataset &holdout,
                                          int64_t seed) {
  MechanicsResult mechanics{};
  mechanics.initial_fixed_objective = fixed_view_objective(model, holdout);
  auto parameters = model->parameters();
  std::vector<torch::Tensor> initial_parameters;
  initial_parameters.reserve(parameters.size());
  for (const auto &parameter : parameters) {
    initial_parameters.push_back(parameter.detach().clone());
  }
  torch::optim::AdamW optimizer(
      parameters, torch::optim::AdamWOptions(1.0e-3).weight_decay(1.0e-4));
  torch::manual_seed(seed * 100003 + 701);
  model->train();
  mechanics.losses_finite = true;
  mechanics.gradients_finite = true;
  std::vector<int64_t> order;
  int64_t current_epoch = -1;
  constexpr int64_t batches_per_epoch = kTrainSamples / kBatchSize;
  for (int64_t step = 0; step < kTrainingSteps; ++step) {
    const int64_t epoch = step / batches_per_epoch;
    if (epoch != current_epoch) {
      order = epoch_permutation(seed, epoch);
      current_epoch = epoch;
    }
    const int64_t batch_in_epoch = step % batches_per_epoch;
    std::vector<int64_t> batch_indices;
    batch_indices.reserve(kBatchSize);
    for (int64_t row = 0; row < kBatchSize; ++row) {
      batch_indices.push_back(
          order[static_cast<std::size_t>(batch_in_epoch * kBatchSize + row)]);
    }
    const auto index = torch::tensor(
        batch_indices, torch::TensorOptions().dtype(torch::kInt64));
    optimizer.zero_grad();
    const auto output = model->forward(train.data.index_select(0, index),
                                       train.mask.index_select(0, index));
    if (!torch::isfinite(output.loss).all().item<bool>()) {
      mechanics.losses_finite = false;
      break;
    }
    output.loss.backward();
    double gradient_norm = 0.0;
    if (!finite_gradient_norm(parameters, &gradient_norm)) {
      mechanics.gradients_finite = false;
      break;
    }
    clip_gradients(parameters, gradient_norm, 5.0);
    optimizer.step();
    model->update_target_network();
    ++mechanics.completed_steps;
  }

  mechanics.final_fixed_objective = fixed_view_objective(model, holdout);
  mechanics.objective_ratio =
      std::isfinite(mechanics.initial_fixed_objective) &&
              mechanics.initial_fixed_objective > 0.0
          ? mechanics.final_fixed_objective / mechanics.initial_fixed_objective
          : std::numeric_limits<double>::infinity();
  mechanics.parameters_finite = parameters_finite(parameters);
  double delta_square_sum = 0.0;
  for (std::size_t index = 0; index < parameters.size(); ++index) {
    delta_square_sum += (parameters[index].detach() - initial_parameters[index])
                            .pow(2)
                            .sum()
                            .item<double>();
  }
  mechanics.parameter_delta_l2 = std::sqrt(delta_square_sum);
  mechanics.parameter_changed = std::isfinite(mechanics.parameter_delta_l2) &&
                                mechanics.parameter_delta_l2 > 1.0e-8;
  mechanics.objective_improved = std::isfinite(mechanics.objective_ratio) &&
                                 mechanics.objective_ratio <= 0.90;
  return mechanics;
}

[[nodiscard]] SeedResult run_seed(int64_t seed, const Dataset &train,
                                  const Dataset &holdout,
                                  const torch::Tensor &train_target,
                                  const torch::Tensor &holdout_target) {
  torch::manual_seed(seed);
  auto model = mtf::MtfJepaMaeVicreg(qualification_config());
  const auto untrained_train = extract_surfaces(model, train, true);
  const auto untrained_holdout = extract_surfaces(model, holdout, true);
  SeedResult result{};
  result.seed = seed;
  result.untrained_prepool =
      qualify_surface("untrained_prepool", untrained_train.prepool,
                      untrained_holdout.prepool, train_target, holdout_target);
  for (std::size_t policy_index = 0; policy_index < kServingPolicies.size();
       ++policy_index) {
    const std::string policy_name =
        mtf::mtf_serving_pool_policy_name(kServingPolicies[policy_index]);
    result.untrained_served[policy_index] = qualify_surface(
        "untrained_" + policy_name, untrained_train.served[policy_index],
        untrained_holdout.served[policy_index], train_target, holdout_target);
  }
  result.mechanics = train_model(model, train, holdout, seed);
  const auto trained_train = extract_surfaces(model, train, true);
  const auto trained_holdout = extract_surfaces(model, holdout, true);
  result.trained_prepool =
      qualify_surface("trained_prepool", trained_train.prepool,
                      trained_holdout.prepool, train_target, holdout_target);
  result.prepool_diagnosis =
      diagnose_learning(result.trained_prepool, result.untrained_prepool);
  bool any_trained_policy_strong = false;
  bool alternative_policy_benefit = false;
  for (std::size_t policy_index = 0; policy_index < kServingPolicies.size();
       ++policy_index) {
    const std::string policy_name =
        mtf::mtf_serving_pool_policy_name(kServingPolicies[policy_index]);
    result.trained_served[policy_index] = qualify_surface(
        "trained_" + policy_name, trained_train.served[policy_index],
        trained_holdout.served[policy_index], train_target, holdout_target);
    result.policy_diagnoses[policy_index] =
        diagnose_learning(result.trained_served[policy_index],
                          result.untrained_served[policy_index]);
    result.any_policy_benefit =
        result.any_policy_benefit ||
        result.policy_diagnoses[policy_index].learning_benefit;
    result.any_policy_regression =
        result.any_policy_regression ||
        result.policy_diagnoses[policy_index].training_regression;
    any_trained_policy_strong =
        any_trained_policy_strong ||
        result.trained_served[policy_index].ridge_stable_strong;
    if (policy_index != 0) {
      alternative_policy_benefit =
          alternative_policy_benefit ||
          result.policy_diagnoses[policy_index].learning_benefit;
    }
  }
  result.all_tokens_policy_specific_harm =
      result.policy_diagnoses[0].training_regression &&
      alternative_policy_benefit;
  result.prepool_only_capacity_without_learning =
      result.trained_prepool.ridge_stable_strong &&
      result.untrained_prepool.ridge_stable_strong &&
      !result.prepool_diagnosis.learning_benefit &&
      !result.any_policy_benefit && !any_trained_policy_strong;
  return result;
}

void emit_metric(const std::string &prefix, const MetricSummary &metric) {
  std::cout << prefix << ".count=" << metric.count << '\n';
  std::cout << prefix << ".direction_count=" << metric.direction_count << '\n';
  std::cout << prefix << ".pairwise_rank_count=" << metric.pair_count << '\n';
  std::cout << prefix << ".directional_accuracy=" << metric.direction << '\n';
  std::cout << prefix << ".pairwise_rank_accuracy=" << metric.rank << '\n';
  std::cout << prefix << ".correlation=" << metric.correlation << '\n';
  std::cout << prefix << ".rmse=" << metric.rmse << '\n';
  std::cout << prefix << ".target_rms=" << metric.target_rms << '\n';
  std::cout << prefix
            << ".rmse_target_rms_ratio=" << metric.rmse_target_rms_ratio
            << '\n';
}

void emit_surface(const std::string &prefix, const SurfaceResult &surface) {
  std::cout << prefix << ".name=" << surface.name << '\n';
  std::cout << prefix << ".ridge_stable_strong=" << surface.ridge_stable_strong
            << '\n';
  std::cout << prefix << ".partial=" << surface.partial << '\n';
  for (std::size_t index = 0; index < surface.ridges.size(); ++index) {
    const auto &ridge = surface.ridges[index];
    const std::string ridge_prefix = prefix + ".ridge_" + kRidgeNames[index];
    std::cout << ridge_prefix << ".alpha=" << ridge.alpha << '\n';
    std::cout << ridge_prefix
              << ".coefficient_l2_norm=" << ridge.model.coefficient_l2_norm
              << '\n';
    std::cout << ridge_prefix << ".maximum_normalized_residual="
              << ridge.model.maximum_normalized_residual << '\n';
    emit_metric(ridge_prefix + ".train.aggregate", ridge.train);
    emit_metric(ridge_prefix + ".holdout.aggregate", ridge.holdout);
    for (int64_t channel = 0; channel < kChannels; ++channel) {
      const std::string suffix = ".channel_" + std::to_string(channel);
      emit_metric(ridge_prefix + ".train" + suffix,
                  ridge.train_channels[static_cast<std::size_t>(channel)]);
      emit_metric(ridge_prefix + ".holdout" + suffix,
                  ridge.holdout_channels[static_cast<std::size_t>(channel)]);
    }
  }
}

void emit_surface_compact(const std::string &prefix,
                          const SurfaceResult &surface) {
  const auto &holdout = surface.ridges[kPrimaryRidge].holdout;
  std::cout << prefix << ".ridge_stable_strong=" << surface.ridge_stable_strong
            << '\n';
  std::cout << prefix << ".partial=" << surface.partial << '\n';
  std::cout << prefix << ".holdout_directional_accuracy=" << holdout.direction
            << '\n';
  std::cout << prefix << ".holdout_pairwise_rank_accuracy=" << holdout.rank
            << '\n';
  std::cout << prefix << ".holdout_correlation=" << holdout.correlation << '\n';
  std::cout << prefix << ".holdout_rmse_target_rms_ratio="
            << holdout.rmse_target_rms_ratio << '\n';
}

void emit_mechanics(const std::string &prefix,
                    const MechanicsResult &mechanics) {
  std::cout << prefix << ".completed_steps=" << mechanics.completed_steps
            << '\n';
  std::cout << prefix
            << ".initial_fixed_objective=" << mechanics.initial_fixed_objective
            << '\n';
  std::cout << prefix
            << ".final_fixed_objective=" << mechanics.final_fixed_objective
            << '\n';
  std::cout << prefix << ".objective_ratio=" << mechanics.objective_ratio
            << '\n';
  std::cout << prefix << ".parameter_delta_l2=" << mechanics.parameter_delta_l2
            << '\n';
  std::cout << prefix << ".losses_finite=" << mechanics.losses_finite << '\n';
  std::cout << prefix << ".gradients_finite=" << mechanics.gradients_finite
            << '\n';
  std::cout << prefix << ".parameters_finite=" << mechanics.parameters_finite
            << '\n';
  std::cout << prefix << ".parameter_changed=" << mechanics.parameter_changed
            << '\n';
  std::cout << prefix << ".objective_improved=" << mechanics.objective_improved
            << '\n';
  std::cout << prefix << ".passed=" << mechanics.passed() << '\n';
}

void emit_metric_comparison(const std::string &prefix,
                            const MetricComparison &comparison) {
  std::cout << prefix << ".trained_to_untrained_mse_ratio="
            << comparison.trained_to_untrained_mse_ratio << '\n';
  std::cout << prefix << ".direction_delta=" << comparison.direction_delta
            << '\n';
  std::cout << prefix << ".rank_delta=" << comparison.rank_delta << '\n';
  std::cout << prefix << ".correlation_delta=" << comparison.correlation_delta
            << '\n';
  std::cout << prefix << ".benefit=" << comparison.benefit << '\n';
  std::cout << prefix << ".regression=" << comparison.regression << '\n';
}

void emit_diagnosis(const std::string &prefix,
                    const LearningDiagnosis &diagnosis, bool verbose) {
  std::cout << prefix
            << ".ridge_benefit_count=" << diagnosis.ridge_benefit_count << '\n';
  std::cout << prefix
            << ".ridge_regression_count=" << diagnosis.ridge_regression_count
            << '\n';
  std::cout << prefix << ".primary_channel_benefit_count="
            << diagnosis.primary_channel_benefit_count << '\n';
  std::cout << prefix << ".primary_channel_regression_count="
            << diagnosis.primary_channel_regression_count << '\n';
  std::cout << prefix << ".learning_benefit=" << diagnosis.learning_benefit
            << '\n';
  std::cout << prefix
            << ".training_regression=" << diagnosis.training_regression << '\n';
  emit_metric_comparison(prefix + ".primary_aggregate",
                         diagnosis.ridge_aggregate[kPrimaryRidge]);
  if (!verbose) {
    return;
  }
  for (std::size_t ridge = 0; ridge < kRidges.size(); ++ridge) {
    emit_metric_comparison(prefix + ".ridge_" + kRidgeNames[ridge] +
                               ".aggregate",
                           diagnosis.ridge_aggregate[ridge]);
  }
  for (std::size_t channel = 0; channel < kChannels; ++channel) {
    emit_metric_comparison(prefix + ".primary.channel_" +
                               std::to_string(channel),
                           diagnosis.primary_channels[channel]);
  }
}

void emit_seed(const SeedResult &seed, bool verbose) {
  const std::string prefix = "seed_" + std::to_string(seed.seed);
  std::cout << prefix << ".seed=" << seed.seed << '\n';
  if (verbose) {
    emit_mechanics(prefix + ".mechanics", seed.mechanics);
    emit_surface(prefix + ".untrained_prepool", seed.untrained_prepool);
    emit_surface(prefix + ".trained_prepool", seed.trained_prepool);
  } else {
    std::cout << prefix
              << ".mechanics.objective_ratio=" << seed.mechanics.objective_ratio
              << '\n';
    std::cout << prefix << ".mechanics.passed=" << seed.mechanics.passed()
              << '\n';
    emit_surface_compact(prefix + ".untrained_prepool", seed.untrained_prepool);
    emit_surface_compact(prefix + ".trained_prepool", seed.trained_prepool);
  }
  emit_diagnosis(prefix + ".prepool_diagnosis", seed.prepool_diagnosis,
                 verbose);
  for (std::size_t policy_index = 0; policy_index < kServingPolicies.size();
       ++policy_index) {
    const std::string policy_name =
        mtf::mtf_serving_pool_policy_name(kServingPolicies[policy_index]);
    const std::string policy_prefix = prefix + ".policy_" + policy_name;
    if (verbose) {
      emit_surface(policy_prefix + ".untrained",
                   seed.untrained_served[policy_index]);
      emit_surface(policy_prefix + ".trained",
                   seed.trained_served[policy_index]);
    } else {
      emit_surface_compact(policy_prefix + ".untrained",
                           seed.untrained_served[policy_index]);
      emit_surface_compact(policy_prefix + ".trained",
                           seed.trained_served[policy_index]);
    }
    emit_diagnosis(policy_prefix + ".diagnosis",
                   seed.policy_diagnoses[policy_index], verbose);
  }
  std::cout << prefix << ".any_policy_benefit=" << seed.any_policy_benefit
            << '\n';
  std::cout << prefix << ".any_policy_regression=" << seed.any_policy_regression
            << '\n';
  std::cout << prefix << ".all_tokens_policy_specific_harm="
            << seed.all_tokens_policy_specific_harm << '\n';
  std::cout << prefix << ".prepool_only_capacity_without_learning="
            << seed.prepool_only_capacity_without_learning << '\n';
}

[[nodiscard]] std::string sanitized_error(std::string message) {
  for (char &character : message) {
    if (character == '\n' || character == '\r' || character == '=') {
      character = ' ';
    }
  }
  return message;
}

int run_qualification(bool verbose) {
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);
  at::globalContext().setDeterministicFillUninitializedMemory(true);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.module_qualification.v2\n";
  std::cout << "output_mode=" << (verbose ? "verbose" : "compact") << '\n';
  std::cout << "in_memory_only=true\n";
  std::cout << "architecture_scope=compact_qualification_config\n";
  std::cout << "training_policy_scope=active_core_policy\n";
  std::cout << "outer_launcher_augmentation_included=false\n";
  std::cout << "device=cpu\n";
  std::cout << "dtype=float32\n";
  std::cout << "probe_dtype=float64\n";
  std::cout << "train_groups=" << kTrainGroups << '\n';
  std::cout << "holdout_groups=" << kHoldoutGroups << '\n';
  std::cout << "instruments_per_group=" << kInstruments << '\n';
  std::cout << "channel_widths=1,3,7\n";
  std::cout << "history_length=" << kHistory << '\n';
  std::cout << "training_steps=" << kTrainingSteps << '\n';
  std::cout << "model_seeds=17,31,47\n";
  std::cout << "ridge_grid=1e-5,1e-4,1e-3\n";
  std::cout << "primary_ridge=1e-4\n";
  std::cout << "strong_gate=direction>=0.95,rank>=0.95,correlation>=0.95,"
               "rmse_target_rms_ratio<=0.25\n";
  std::cout << "partial_gate=direction>=0.80,rank>=0.78\n";
  std::cout << "learning_comparison_gate=aggregate_agreement>=2/3_ridges,"
               "channel_agreement>=2/3_at_primary_ridge\n";
  std::cout << "learning_benefit_point=mse_ratio<=0.80,direction_delta>=-0.02,"
               "rank_delta>=-0.02,correlation_delta>=-0.02\n";
  std::cout << "training_regression_point=mse_ratio>=1.25,"
               "correlation_delta<=-0.05\n";
  std::cout << "uncertainty_scope=three_fixed_seeds_no_bootstrap\n";

  auto train = generate_dataset(/*group_begin=*/0, kTrainGroups);
  auto holdout = generate_dataset(/*group_begin=*/1000000, kHoldoutGroups);
  normalize_inputs(train, holdout);
  const auto guards = validate_generator(train, holdout);
  std::cout << "generator.finite=" << guards.finite << '\n';
  std::cout << "generator.sign_balance=" << guards.sign_balance << '\n';
  std::cout << "generator.pairwise_ties=" << guards.pairwise_ties << '\n';
  std::cout << "generator.split_disjoint=" << guards.split_disjoint << '\n';
  std::cout << "generator.minimum_positive_fraction="
            << guards.minimum_positive_fraction << '\n';
  std::cout << "generator.maximum_positive_fraction="
            << guards.maximum_positive_fraction << '\n';
  std::cout << "generator.pairwise_tie_fraction="
            << guards.pairwise_tie_fraction << '\n';
  if (!guards.passed()) {
    std::cout << "classification=invalid_harness\n";
    std::cout << "full_qualification=false\n";
    std::cout << "status=invalid_harness\n";
    return 2;
  }

  const auto target_scale = train.target.pow(2).mean(/*dim=*/0).sqrt();
  if (!torch::isfinite(target_scale).all().item<bool>() ||
      !target_scale.gt(1.0e-12).all().item<bool>()) {
    throw std::runtime_error("target scale is invalid");
  }
  const auto train_target =
      (train.target / target_scale.unsqueeze(0)).contiguous();
  const auto holdout_target =
      (holdout.target / target_scale.unsqueeze(0)).contiguous();
  const auto raw_train =
      train.data.reshape({kTrainSamples, kChannels, kHistory * kFeatures});
  const auto raw_holdout =
      holdout.data.reshape({kHoldoutSamples, kChannels, kHistory * kFeatures});
  const auto raw = qualify_surface("raw", raw_train, raw_holdout, train_target,
                                   holdout_target);
  if (verbose) {
    emit_surface("raw", raw);
  } else {
    emit_surface_compact("raw", raw);
  }
  const bool raw_pass = raw_harness_pass(raw);
  std::cout << "raw_harness_pass=" << raw_pass << '\n';
  if (!raw_pass) {
    std::cout << "classification=invalid_generator_or_probe\n";
    std::cout << "full_qualification=false\n";
    std::cout << "status=invalid_harness\n";
    return 2;
  }

  std::vector<SeedResult> seeds;
  seeds.reserve(kModelSeeds.size());
  for (const int64_t seed : kModelSeeds) {
    seeds.push_back(
        run_seed(seed, train, holdout, train_target, holdout_target));
    emit_seed(seeds.back(), verbose);
  }

  int mechanics_pass_count = 0;
  int untrained_prepool_strong_count = 0;
  int trained_prepool_strong_count = 0;
  int prepool_partial_count = 0;
  int prepool_learning_benefit_count = 0;
  int prepool_regression_count = 0;
  std::array<int, kServingPolicies.size()> untrained_policy_strong_count{};
  std::array<int, kServingPolicies.size()> trained_policy_strong_count{};
  std::array<int, kServingPolicies.size()> policy_benefit_count{};
  std::array<int, kServingPolicies.size()> policy_regression_count{};
  int any_served_strong_count = 0;
  int any_policy_benefit_count = 0;
  int any_policy_regression_count = 0;
  int all_tokens_specific_harm_count = 0;
  int prepool_only_capacity_count = 0;
  int joint_qualified_count = 0;
  for (const auto &seed : seeds) {
    const bool mechanics_pass = seed.mechanics.passed();
    mechanics_pass_count += mechanics_pass ? 1 : 0;
    if (!mechanics_pass) {
      continue;
    }
    untrained_prepool_strong_count +=
        seed.untrained_prepool.ridge_stable_strong ? 1 : 0;
    trained_prepool_strong_count +=
        seed.trained_prepool.ridge_stable_strong ? 1 : 0;
    prepool_partial_count += seed.trained_prepool.partial ? 1 : 0;
    prepool_learning_benefit_count +=
        seed.prepool_diagnosis.learning_benefit ? 1 : 0;
    prepool_regression_count +=
        seed.prepool_diagnosis.training_regression ? 1 : 0;
    bool any_served_strong = false;
    for (std::size_t policy_index = 0; policy_index < kServingPolicies.size();
         ++policy_index) {
      untrained_policy_strong_count[policy_index] +=
          seed.untrained_served[policy_index].ridge_stable_strong ? 1 : 0;
      trained_policy_strong_count[policy_index] +=
          seed.trained_served[policy_index].ridge_stable_strong ? 1 : 0;
      policy_benefit_count[policy_index] +=
          seed.policy_diagnoses[policy_index].learning_benefit ? 1 : 0;
      policy_regression_count[policy_index] +=
          seed.policy_diagnoses[policy_index].training_regression ? 1 : 0;
      any_served_strong = any_served_strong ||
                          seed.trained_served[policy_index].ridge_stable_strong;
    }
    any_served_strong_count += any_served_strong ? 1 : 0;
    any_policy_benefit_count += seed.any_policy_benefit ? 1 : 0;
    any_policy_regression_count += seed.any_policy_regression ? 1 : 0;
    all_tokens_specific_harm_count +=
        seed.all_tokens_policy_specific_harm ? 1 : 0;
    prepool_only_capacity_count +=
        seed.prepool_only_capacity_without_learning ? 1 : 0;
    const bool active_policy_joint_qualified =
        seed.trained_served[0].ridge_stable_strong &&
        seed.policy_diagnoses[0].learning_benefit;
    joint_qualified_count += active_policy_joint_qualified ? 1 : 0;
  }
  std::cout << "summary.mechanics_pass_count=" << mechanics_pass_count << '\n';
  std::cout << "summary.untrained_prepool_strong_count="
            << untrained_prepool_strong_count << '\n';
  std::cout << "summary.trained_prepool_strong_count="
            << trained_prepool_strong_count << '\n';
  std::cout << "summary.trained_prepool_partial_count=" << prepool_partial_count
            << '\n';
  std::cout << "summary.prepool_learning_benefit_count="
            << prepool_learning_benefit_count << '\n';
  std::cout << "summary.prepool_training_regression_count="
            << prepool_regression_count << '\n';
  for (std::size_t policy_index = 0; policy_index < kServingPolicies.size();
       ++policy_index) {
    const std::string prefix =
        std::string("summary.policy_") +
        mtf::mtf_serving_pool_policy_name(kServingPolicies[policy_index]);
    std::cout << prefix << ".untrained_strong_count="
              << untrained_policy_strong_count[policy_index] << '\n';
    std::cout << prefix << ".trained_strong_count="
              << trained_policy_strong_count[policy_index] << '\n';
    std::cout << prefix << ".learning_benefit_count="
              << policy_benefit_count[policy_index] << '\n';
    std::cout << prefix << ".training_regression_count="
              << policy_regression_count[policy_index] << '\n';
  }
  std::cout << "summary.any_served_strong_count=" << any_served_strong_count
            << '\n';
  std::cout << "summary.any_policy_learning_benefit_count="
            << any_policy_benefit_count << '\n';
  std::cout << "summary.any_policy_training_regression_count="
            << any_policy_regression_count << '\n';
  std::cout << "summary.all_tokens_policy_specific_harm_count="
            << all_tokens_specific_harm_count << '\n';
  std::cout << "summary.prepool_only_capacity_without_learning_count="
            << prepool_only_capacity_count << '\n';
  std::cout << "summary.active_all_tokens_joint_qualified_count="
            << joint_qualified_count << '\n';

  if (mechanics_pass_count < 2) {
    std::cout << "classification=training_mechanics_failed\n";
    std::cout << "full_qualification=false\n";
    std::cout << "status=failed_training_mechanics\n";
    return 3;
  }

  std::string classification;
  bool full_qualification = false;
  if (joint_qualified_count >= 2) {
    classification = "learned_active_served_representation_qualified";
    full_qualification = true;
  } else if (all_tokens_specific_harm_count >= 2) {
    classification = "all_tokens_policy_specific_harm_detected";
  } else if (any_policy_benefit_count >= 2) {
    classification = "same_width_policy_training_benefit_detected";
  } else if (prepool_only_capacity_count >= 2) {
    classification = "prepool_only_capacity_without_learned_improvement";
  } else if (any_policy_regression_count >= 2) {
    classification = "same_width_policy_training_regression_detected";
  } else if (any_served_strong_count >= 2) {
    classification = "served_capacity_sufficient_learning_unproven";
  } else if (trained_prepool_strong_count >= 2) {
    classification = "prepool_capacity_detected_learning_unproven";
  } else if (prepool_partial_count >= 2) {
    classification = "prepool_signal_partial_insufficient";
  } else {
    classification = "encoder_or_objective_information_failure";
  }
  std::cout << "classification=" << classification << '\n';
  std::cout << "full_qualification=" << full_qualification << '\n';
  std::cout << "status=complete\n";
  return 0;
}

} // namespace

int main(int argc, char **argv) {
  try {
    bool verbose = false;
    if (argc == 2 && std::string(argv[1]) == "--verbose") {
      verbose = true;
    } else if (argc != 1) {
      throw std::invalid_argument(
          "usage: qualify_wikimyei_mtf_jepa_mae_vicreg [--verbose]");
    }
    return run_qualification(verbose);
  } catch (const std::exception &error) {
    std::cout << std::boolalpha;
    std::cout << "classification=invalid_harness\n";
    std::cout << "full_qualification=false\n";
    std::cout << "status=error\n";
    std::cout << "error=" << sanitized_error(error.what()) << '\n';
    return 2;
  }
}
