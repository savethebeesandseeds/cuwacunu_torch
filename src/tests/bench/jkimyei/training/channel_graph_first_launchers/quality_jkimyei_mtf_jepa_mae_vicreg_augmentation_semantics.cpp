#include "jkimyei/training/representation/mtf_jepa_mae_vicreg_graph_first_launcher.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <torch/torch.h>

namespace launcher = cuwacunu::jkimyei::training::representation::
    mtf_jepa_mae_vicreg_graph_first_launcher_detail;
namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;

namespace {

constexpr int64_t kBatchSize = 256;
constexpr int64_t kChannelCount = 3;
constexpr int64_t kHistory = 30;
constexpr int64_t kFeatureWidth = 9;
constexpr uint64_t kSeed = 17;
constexpr int64_t kAugmentationDraws = 16;
constexpr double kTwoPi = 6.28318530717958647692;
constexpr std::array<int64_t, kChannelCount> kChannelHistories{4, 10, 30};
constexpr std::array<double, kChannelCount> kCouplingGain{1.0, 1.5, -0.75};
constexpr std::array<double, kChannelCount> kLagSteps{0.0, 1.0, 2.0};

struct TestInput {
  torch::Tensor data;
  torch::Tensor feature_mask;
};

struct PhaseAccumulator {
  int64_t count{};
  int64_t sign_correct{};
  double prediction_sum{};
  double target_sum{};
  double prediction_sq_sum{};
  double target_sq_sum{};
  double cross_sum{};
  double phase_sq_error{};
};

struct ChannelMetrics {
  double support_retention{};
  double terminal_support{};
  double terminal_anchor_conditional_mae{};
  double terminal_anchor_total_mae{};
  double terminal_value_nrmse{};
  double temporal_pair_concordance{};
  double phase_future_direction_accuracy{};
  double phase_future_correlation{};
  double phase_rmse_radians{};
  double future_slope_direction_accuracy{};
};

struct SemanticMetrics {
  std::array<ChannelMetrics, kChannelCount> channels{};
  double cross_channel_coupling_nrmse{};
  double cross_channel_lag_rmse_steps{};
  double mask_only_future_direction_accuracy{};
  int64_t mask_pattern_count{};
  bool structural_history_identity_visible{};
};

struct ArmEvaluation {
  std::string name;
  SemanticMetrics aggregate;
  std::vector<SemanticMetrics> draws;
};

struct ArmGateResult {
  bool support{};
  bool terminal{};
  bool temporal_order{};
  bool phase{};
  bool future_direction{};
  bool cross_channel_coupling{};
  bool cross_channel_lag{};
  bool every_draw{};
  bool qualified{};
};

struct SinusoidFit {
  bool valid{};
  double sine_coefficient{};
  double cosine_coefficient{};
};

[[nodiscard]] double circular_difference(double lhs, double rhs) {
  return std::atan2(std::sin(lhs - rhs), std::cos(lhs - rhs));
}

[[nodiscard]] double finite_or_infinity(double numerator, double denominator) {
  if (denominator <= 0.0) {
    return std::numeric_limits<double>::infinity();
  }
  return numerator / denominator;
}

[[nodiscard]] double correlation(const PhaseAccumulator &accumulator) {
  const double count = static_cast<double>(accumulator.count);
  if (count <= 1.0) {
    return 0.0;
  }
  const double numerator = accumulator.cross_sum - accumulator.prediction_sum *
                                                       accumulator.target_sum /
                                                       count;
  const double prediction_variance =
      accumulator.prediction_sq_sum -
      accumulator.prediction_sum * accumulator.prediction_sum / count;
  const double target_variance =
      accumulator.target_sq_sum -
      accumulator.target_sum * accumulator.target_sum / count;
  const double denominator = std::sqrt(std::max(0.0, prediction_variance) *
                                       std::max(0.0, target_variance));
  return denominator > 0.0 ? numerator / denominator : 0.0;
}

void require_contract(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t neutral_config() {
  mtf::mtf_jepa_mae_vicreg_config_t config;
  config.channel_count = kChannelCount;
  config.history_length = kHistory;
  config.input_width = kFeatureWidth;
  config.augmentation_profile = "no_input_augmentation_v1";
  config.gaussian_jitter_std = 0.0;
  config.feature_dropout_prob = 0.0;
  config.history_dropout_prob = 0.0;
  config.time_crop_jitter_max = 0;
  config.time_dilation_min = 1.0;
  config.time_dilation_max = 1.0;
  config.time_warp_max = 0.0;
  config.amplitude_scale_min = 1.0;
  config.amplitude_scale_max = 1.0;
  config.amplitude_shift_std = 0.0;
  config.frequency_mask_ratio = 0.0;
  config.frequency_jitter_std = 0.0;
  config.phase_jitter_max = 0.0;
  config.channel_dropout_prob = 0.0;
  config.cross_channel_dropout_prob = 0.0;
  config.node_dropout_prob = 0.0;
  config.edge_dropout_prob = 0.0;
  config.magnitude_normalization_noise_std = 0.0;
  return config;
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t active_config() {
  auto config = neutral_config();
  config.augmentation_profile = "light_phase_safe_v2";
  config.gaussian_jitter_std = 0.001;
  config.time_dilation_min = 0.98;
  config.time_dilation_max = 1.02;
  config.time_warp_max = 0.01;
  config.amplitude_scale_min = 0.98;
  config.amplitude_scale_max = 1.02;
  config.frequency_mask_ratio = 0.02;
  config.frequency_jitter_std = 0.01;
  return config;
}

[[nodiscard]] std::vector<
    std::pair<std::string, mtf::mtf_jepa_mae_vicreg_config_t>>
leave_one_in_configs() {
  std::vector<std::pair<std::string, mtf::mtf_jepa_mae_vicreg_config_t>> arms;
  auto add = [&arms](std::string name,
                     mtf::mtf_jepa_mae_vicreg_config_t config) {
    config.augmentation_profile = "attribution_" + name;
    arms.emplace_back(std::move(name), std::move(config));
  };

  auto gaussian = neutral_config();
  gaussian.gaussian_jitter_std = 0.001;
  add("gaussian_jitter_only", std::move(gaussian));

  auto amplitude = neutral_config();
  amplitude.amplitude_scale_min = 0.98;
  amplitude.amplitude_scale_max = 1.02;
  add("amplitude_scale_only", std::move(amplitude));

  auto frequency_mask = neutral_config();
  frequency_mask.frequency_mask_ratio = 0.02;
  add("frequency_mask_only", std::move(frequency_mask));

  auto frequency_gain_jitter = neutral_config();
  frequency_gain_jitter.frequency_jitter_std = 0.01;
  add("frequency_gain_jitter_only", std::move(frequency_gain_jitter));

  auto candidate_safe = neutral_config();
  candidate_safe.gaussian_jitter_std = 0.001;
  candidate_safe.amplitude_scale_min = 0.98;
  candidate_safe.amplitude_scale_max = 1.02;
  candidate_safe.frequency_jitter_std = 0.01;
  add("candidate_safe_stack", std::move(candidate_safe));

  auto dilation = neutral_config();
  dilation.time_dilation_min = 0.98;
  dilation.time_dilation_max = 1.02;
  add("dilation_only", std::move(dilation));

  auto warp = neutral_config();
  warp.time_warp_max = 0.01;
  add("warp_only", std::move(warp));

  auto temporal = neutral_config();
  temporal.time_dilation_min = 0.98;
  temporal.time_dilation_max = 1.02;
  temporal.time_warp_max = 0.01;
  add("temporal_dilation_plus_warp", std::move(temporal));
  return arms;
}

[[nodiscard]] TestInput make_semantic_input() {
  auto data = torch::zeros({kBatchSize, kChannelCount, kHistory, kFeatureWidth},
                           torch::kFloat32);
  auto mask = torch::zeros({kBatchSize, kChannelCount, kHistory, kFeatureWidth},
                           torch::kBool);
  auto values = data.accessor<float, 4>();
  auto valid = mask.accessor<bool, 4>();
  constexpr double phase_omega = kTwoPi / 8.0;
  constexpr double lag_omega = kTwoPi / 13.0;

  for (int64_t b = 0; b < kBatchSize; ++b) {
    const double phase = kTwoPi * (static_cast<double>(b) + 0.37) / kBatchSize;
    const double sample_amplitude =
        0.8 + 0.4 * static_cast<double>(b % 17) / 16.0;
    const double sample_bias = -0.2 + 0.4 * static_cast<double>(b % 13) / 12.0;
    const double future_direction = (b % 2 == 0) ? -1.0 : 1.0;
    for (int64_t c = 0; c < kChannelCount; ++c) {
      const int64_t start =
          kHistory - kChannelHistories[static_cast<std::size_t>(c)];
      for (int64_t t = start; t < kHistory; ++t) {
        const double x = static_cast<double>(t) / (kHistory - 1);
        const double coupled =
            0.65 * std::sin(kTwoPi * static_cast<double>(t) / 11.0 + phase) +
            0.25 *
                std::cos(kTwoPi * static_cast<double>(t) / 5.0 + 0.3 * phase) +
            0.1 * x;
        values[b][c][t][0] = static_cast<float>(x);
        values[b][c][t][1] = static_cast<float>(
            sample_bias + sample_amplitude * (0.25 * x + 0.75 * x * x));
        values[b][c][t][2] = static_cast<float>(
            std::sin(phase_omega * static_cast<double>(t) + phase));
        values[b][c][t][3] = static_cast<float>(
            std::cos(phase_omega * static_cast<double>(t) + phase));
        values[b][c][t][4] = static_cast<float>(
            kCouplingGain[static_cast<std::size_t>(c)] * coupled);
        values[b][c][t][5] = static_cast<float>(
            std::sin(lag_omega * (static_cast<double>(t) -
                                  kLagSteps[static_cast<std::size_t>(c)]) +
                     phase));
        values[b][c][t][6] = static_cast<float>(
            future_direction * (0.25 + 1.25 * x) + 0.05 * phase);
        values[b][c][t][7] = t == kHistory - 1 ? 1.0F : 0.0F;
        values[b][c][t][8] = static_cast<float>(
            std::sin(0.11 * static_cast<double>(t * t) + 0.5 * phase));
        for (int64_t f = 0; f < kFeatureWidth; ++f) {
          valid[b][c][t][f] = true;
        }
      }
    }
  }
  return {data, mask};
}

[[nodiscard]] TestInput with_dirty_masked_sentinel(const TestInput &input) {
  const auto sentinel = torch::full_like(input.data, 1234567.0);
  return {torch::where(input.feature_mask, input.data, sentinel),
          input.feature_mask.clone()};
}

[[nodiscard]] SinusoidFit
fit_sinusoid(const torch::TensorAccessor<float, 4> &data,
             const torch::TensorAccessor<bool, 4> &mask, int64_t batch,
             int64_t channel, int64_t feature, double omega) {
  double ss = 0.0;
  double sc = 0.0;
  double cc = 0.0;
  double sy = 0.0;
  double cy = 0.0;
  for (int64_t t = 0; t < kHistory; ++t) {
    if (!mask[batch][channel][t][feature]) {
      continue;
    }
    const double sine = std::sin(omega * static_cast<double>(t));
    const double cosine = std::cos(omega * static_cast<double>(t));
    const double value = data[batch][channel][t][feature];
    ss += sine * sine;
    sc += sine * cosine;
    cc += cosine * cosine;
    sy += sine * value;
    cy += cosine * value;
  }
  const double determinant = ss * cc - sc * sc;
  if (std::fabs(determinant) <= 1.0e-10) {
    return {};
  }
  return {true, (sy * cc - cy * sc) / determinant,
          (cy * ss - sy * sc) / determinant};
}

[[nodiscard]] std::string
mask_pattern(const torch::TensorAccessor<bool, 4> &mask, int64_t batch) {
  std::string pattern;
  pattern.reserve(static_cast<std::size_t>(kChannelCount * kHistory));
  for (int64_t c = 0; c < kChannelCount; ++c) {
    for (int64_t t = 0; t < kHistory; ++t) {
      pattern.push_back(mask[batch][c][t][0] ? '1' : '0');
    }
  }
  return pattern;
}

[[nodiscard]] double mask_only_future_direction_accuracy(
    const torch::TensorAccessor<bool, 4> &mask) {
  std::map<std::string, std::array<int64_t, 2>> pattern_counts;
  for (int64_t b = 0; b < mask.size(0); ++b) {
    const int64_t sample = b % kBatchSize;
    ++pattern_counts[mask_pattern(mask, b)]
                    [static_cast<std::size_t>(sample % 2)];
  }
  int64_t correct = 0;
  for (const auto &[pattern, counts] : pattern_counts) {
    (void)pattern;
    correct += std::max(counts[0], counts[1]);
  }
  return finite_or_infinity(static_cast<double>(correct),
                            static_cast<double>(mask.size(0)));
}

[[nodiscard]] SemanticMetrics evaluate_semantics(const TestInput &clean,
                                                 const mtf::mtf_input_t &out) {
  require_contract(out.data.sizes() == clean.data.sizes(),
                   "augmented data shape differs from input");
  require_contract(out.feature_mask.sizes() == clean.feature_mask.sizes(),
                   "augmented mask shape differs from input");
  const auto clean_data = clean.data.to(torch::kCPU).contiguous();
  const auto clean_mask = clean.feature_mask.to(torch::kCPU).contiguous();
  const auto output_data = out.data.to(torch::kCPU).contiguous();
  const auto output_mask = out.feature_mask.to(torch::kCPU).contiguous();
  const auto clean_values = clean_data.accessor<float, 4>();
  const auto values = output_data.accessor<float, 4>();
  const auto valid = output_mask.accessor<bool, 4>();

  SemanticMetrics metrics;
  std::array<PhaseAccumulator, kChannelCount> phase_metrics{};
  std::array<double, kChannelCount> terminal_anchor_error{};
  std::array<int64_t, kChannelCount> terminal_anchor_count{};
  std::array<double, kChannelCount> terminal_value_error_sq{};
  std::array<double, kChannelCount> terminal_value_reference_sq{};
  std::array<int64_t, kChannelCount> order_correct{};
  std::array<int64_t, kChannelCount> order_count{};
  std::array<int64_t, kChannelCount> slope_correct{};
  std::array<int64_t, kChannelCount> slope_count{};
  double coupling_error_sq = 0.0;
  double coupling_reference_sq = 0.0;
  double lag_error_sq = 0.0;
  int64_t lag_error_count = 0;
  constexpr double phase_omega = kTwoPi / 8.0;
  constexpr double lag_omega = kTwoPi / 13.0;
  const int64_t batch_size = output_data.size(0);

  for (int64_t c = 0; c < kChannelCount; ++c) {
    const double initial_count = clean_mask.select(1, c).sum().item<double>();
    const double retained_count = output_mask.select(1, c).sum().item<double>();
    metrics.channels[static_cast<std::size_t>(c)].support_retention =
        finite_or_infinity(retained_count, initial_count);
  }

  for (int64_t b = 0; b < batch_size; ++b) {
    const int64_t sample = b % kBatchSize;
    const double phase =
        kTwoPi * (static_cast<double>(sample) + 0.37) / kBatchSize;
    const double phase_target =
        std::sin(phase_omega * static_cast<double>(kHistory) + phase);
    const double expected_slope_sign = sample % 2 == 0 ? -1.0 : 1.0;
    std::array<SinusoidFit, kChannelCount> lag_fits{};
    for (int64_t c = 0; c < kChannelCount; ++c) {
      if (valid[b][c][kHistory - 1][7]) {
        ++terminal_anchor_count[static_cast<std::size_t>(c)];
        terminal_anchor_error[static_cast<std::size_t>(c)] +=
            std::fabs(static_cast<double>(values[b][c][kHistory - 1][7]) - 1.0);
        const double terminal_diff =
            static_cast<double>(values[b][c][kHistory - 1][1]) -
            static_cast<double>(clean_values[b][c][kHistory - 1][1]);
        terminal_value_error_sq[static_cast<std::size_t>(c)] +=
            terminal_diff * terminal_diff;
        const double reference = clean_values[b][c][kHistory - 1][1];
        terminal_value_reference_sq[static_cast<std::size_t>(c)] +=
            reference * reference;
      }

      for (int64_t left = 0; left < kHistory; ++left) {
        if (!valid[b][c][left][0]) {
          continue;
        }
        for (int64_t right = left + 1; right < kHistory; ++right) {
          if (!valid[b][c][right][0]) {
            continue;
          }
          ++order_count[static_cast<std::size_t>(c)];
          if (values[b][c][right][0] > values[b][c][left][0]) {
            ++order_correct[static_cast<std::size_t>(c)];
          }
        }
      }

      const auto phase_fit =
          fit_sinusoid(values, valid, b, c, /*feature=*/2, phase_omega);
      if (phase_fit.valid && std::fabs(phase_target) > 0.05) {
        const double prediction =
            phase_fit.sine_coefficient *
                std::sin(phase_omega * static_cast<double>(kHistory)) +
            phase_fit.cosine_coefficient *
                std::cos(phase_omega * static_cast<double>(kHistory));
        auto &accumulator = phase_metrics[static_cast<std::size_t>(c)];
        ++accumulator.count;
        accumulator.sign_correct +=
            (prediction > 0.0) == (phase_target > 0.0) ? 1 : 0;
        accumulator.prediction_sum += prediction;
        accumulator.target_sum += phase_target;
        accumulator.prediction_sq_sum += prediction * prediction;
        accumulator.target_sq_sum += phase_target * phase_target;
        accumulator.cross_sum += prediction * phase_target;
        const double estimated_phase = std::atan2(phase_fit.cosine_coefficient,
                                                  phase_fit.sine_coefficient);
        const double phase_error = circular_difference(estimated_phase, phase);
        accumulator.phase_sq_error += phase_error * phase_error;
      }

      double time_sum = 0.0;
      double value_sum = 0.0;
      double time_sq_sum = 0.0;
      double cross_sum = 0.0;
      int64_t valid_count = 0;
      for (int64_t t = 0; t < kHistory; ++t) {
        if (!valid[b][c][t][6]) {
          continue;
        }
        const double time = static_cast<double>(t);
        const double value = values[b][c][t][6];
        time_sum += time;
        value_sum += value;
        time_sq_sum += time * time;
        cross_sum += time * value;
        ++valid_count;
      }
      const double slope_denominator =
          time_sq_sum -
          time_sum * time_sum /
              static_cast<double>(std::max<int64_t>(1, valid_count));
      if (valid_count >= 2 && slope_denominator > 1.0e-10) {
        const double slope = (cross_sum - time_sum * value_sum / valid_count) /
                             slope_denominator;
        ++slope_count[static_cast<std::size_t>(c)];
        slope_correct[static_cast<std::size_t>(c)] +=
            (slope > 0.0) == (expected_slope_sign > 0.0) ? 1 : 0;
      }

      lag_fits[static_cast<std::size_t>(c)] =
          fit_sinusoid(values, valid, b, c, /*feature=*/5, lag_omega);
    }

    for (int64_t c = 1; c < kChannelCount; ++c) {
      for (int64_t t = 0; t < kHistory; ++t) {
        if (!valid[b][0][t][4] || !valid[b][c][t][4]) {
          continue;
        }
        const double base = clean_values[b][0][t][4];
        const double lhs = values[b][0][t][4] / kCouplingGain[0];
        const double rhs =
            values[b][c][t][4] / kCouplingGain[static_cast<std::size_t>(c)];
        const double difference = lhs - rhs;
        coupling_error_sq += difference * difference;
        coupling_reference_sq += base * base;
      }

      const auto &reference_fit = lag_fits[0];
      const auto &other_fit = lag_fits[static_cast<std::size_t>(c)];
      if (reference_fit.valid && other_fit.valid) {
        const double reference_phase = std::atan2(
            reference_fit.cosine_coefficient, reference_fit.sine_coefficient);
        const double other_phase = std::atan2(other_fit.cosine_coefficient,
                                              other_fit.sine_coefficient);
        const double expected_difference =
            -lag_omega * kLagSteps[static_cast<std::size_t>(c)];
        const double error_radians = circular_difference(
            other_phase - reference_phase, expected_difference);
        const double error_steps = error_radians / lag_omega;
        lag_error_sq += error_steps * error_steps;
        ++lag_error_count;
      }
    }
  }

  for (std::size_t c = 0; c < metrics.channels.size(); ++c) {
    auto &channel = metrics.channels[c];
    const double terminal_count = static_cast<double>(terminal_anchor_count[c]);
    channel.terminal_support = terminal_count / batch_size;
    channel.terminal_anchor_conditional_mae =
        finite_or_infinity(terminal_anchor_error[c], terminal_count);
    channel.terminal_anchor_total_mae =
        (terminal_anchor_error[c] +
         static_cast<double>(batch_size - terminal_anchor_count[c])) /
        batch_size;
    channel.terminal_value_nrmse = std::sqrt(finite_or_infinity(
        terminal_value_error_sq[c], terminal_value_reference_sq[c]));
    channel.temporal_pair_concordance =
        finite_or_infinity(static_cast<double>(order_correct[c]),
                           static_cast<double>(order_count[c]));
    const auto &phase = phase_metrics[c];
    channel.phase_future_direction_accuracy =
        finite_or_infinity(static_cast<double>(phase.sign_correct),
                           static_cast<double>(phase.count));
    channel.phase_future_correlation = correlation(phase);
    channel.phase_rmse_radians = std::sqrt(finite_or_infinity(
        phase.phase_sq_error, static_cast<double>(phase.count)));
    channel.future_slope_direction_accuracy =
        finite_or_infinity(static_cast<double>(slope_correct[c]),
                           static_cast<double>(slope_count[c]));
  }

  metrics.cross_channel_coupling_nrmse =
      std::sqrt(finite_or_infinity(coupling_error_sq, coupling_reference_sq));
  metrics.cross_channel_lag_rmse_steps = std::sqrt(
      finite_or_infinity(lag_error_sq, static_cast<double>(lag_error_count)));
  metrics.mask_only_future_direction_accuracy =
      mask_only_future_direction_accuracy(valid);
  std::set<std::string> patterns;
  for (int64_t b = 0; b < batch_size; ++b) {
    patterns.insert(mask_pattern(valid, b));
  }
  metrics.mask_pattern_count = static_cast<int64_t>(patterns.size());
  std::set<int64_t> channel_support_counts;
  for (int64_t c = 0; c < kChannelCount; ++c) {
    channel_support_counts.insert(output_mask.select(0, 0)
                                      .select(0, c)
                                      .select(1, 0)
                                      .sum()
                                      .item<int64_t>());
  }
  metrics.structural_history_identity_visible =
      channel_support_counts.size() == kChannelCount;
  return metrics;
}

[[nodiscard]] ArmEvaluation
evaluate_arm(const TestInput &input, const std::string &name,
             const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  std::vector<torch::Tensor> data;
  std::vector<torch::Tensor> masks;
  std::vector<SemanticMetrics> draws;
  data.reserve(kAugmentationDraws);
  masks.reserve(kAugmentationDraws);
  draws.reserve(kAugmentationDraws);
  for (int64_t draw = 0; draw < kAugmentationDraws; ++draw) {
    torch::manual_seed(kSeed + static_cast<uint64_t>(draw));
    const auto output =
        launcher::apply_mtf_training_augmentations(input, config);
    draws.push_back(evaluate_semantics(input, output));
    data.push_back(output.data);
    masks.push_back(output.feature_mask);
  }
  const TestInput repeated_input{
      input.data.repeat({kAugmentationDraws, 1, 1, 1}),
      input.feature_mask.repeat({kAugmentationDraws, 1, 1, 1})};
  const mtf::mtf_input_t combined{torch::cat(data, /*dim=*/0),
                                  torch::cat(masks, /*dim=*/0)};
  return {name, evaluate_semantics(repeated_input, combined), std::move(draws)};
}

[[nodiscard]] bool all_channels(const SemanticMetrics &metrics,
                                bool (*predicate)(const ChannelMetrics &)) {
  return std::all_of(metrics.channels.begin(), metrics.channels.end(),
                     predicate);
}

[[nodiscard]] bool support_gate(const ChannelMetrics &metric) {
  return metric.support_retention >= 0.95;
}

[[nodiscard]] bool terminal_gate(const ChannelMetrics &metric) {
  return metric.terminal_support >= 0.99 &&
         metric.terminal_anchor_total_mae <= 0.05 &&
         metric.terminal_value_nrmse <= 0.05;
}

[[nodiscard]] bool order_gate(const ChannelMetrics &metric) {
  return metric.temporal_pair_concordance >= 0.995;
}

[[nodiscard]] bool phase_gate(const ChannelMetrics &metric) {
  return metric.phase_future_direction_accuracy >= 0.95 &&
         metric.phase_future_correlation >= 0.95 &&
         metric.phase_rmse_radians <= 0.25;
}

[[nodiscard]] bool future_direction_gate(const ChannelMetrics &metric) {
  return metric.future_slope_direction_accuracy >= 0.99;
}

[[nodiscard]] bool semantic_gates_pass(const SemanticMetrics &metrics) {
  return all_channels(metrics, support_gate) &&
         all_channels(metrics, terminal_gate) &&
         all_channels(metrics, order_gate) &&
         all_channels(metrics, phase_gate) &&
         all_channels(metrics, future_direction_gate) &&
         metrics.cross_channel_coupling_nrmse <= 0.05 &&
         metrics.cross_channel_lag_rmse_steps <= 0.25;
}

[[nodiscard]] ArmGateResult classify_arm(const ArmEvaluation &arm) {
  ArmGateResult result;
  result.support = all_channels(arm.aggregate, support_gate);
  result.terminal = all_channels(arm.aggregate, terminal_gate);
  result.temporal_order = all_channels(arm.aggregate, order_gate);
  result.phase = all_channels(arm.aggregate, phase_gate);
  result.future_direction = all_channels(arm.aggregate, future_direction_gate);
  result.cross_channel_coupling =
      arm.aggregate.cross_channel_coupling_nrmse <= 0.05;
  result.cross_channel_lag = arm.aggregate.cross_channel_lag_rmse_steps <= 0.25;
  result.every_draw =
      std::all_of(arm.draws.begin(), arm.draws.end(), semantic_gates_pass);
  result.qualified = result.support && result.terminal &&
                     result.temporal_order && result.phase &&
                     result.future_direction && result.cross_channel_coupling &&
                     result.cross_channel_lag && result.every_draw;
  return result;
}

void print_channel_metrics(const SemanticMetrics &metrics) {
  for (std::size_t c = 0; c < metrics.channels.size(); ++c) {
    const auto history = kChannelHistories[c];
    const auto &metric = metrics.channels[c];
    const std::string prefix = "metric.h" + std::to_string(history) + ".";
    std::cout << prefix << "support_retention=" << metric.support_retention
              << '\n';
    std::cout << prefix << "terminal_support=" << metric.terminal_support
              << '\n';
    std::cout << prefix << "terminal_anchor_conditional_mae="
              << metric.terminal_anchor_conditional_mae << '\n';
    std::cout << prefix << "terminal_anchor_total_mae="
              << metric.terminal_anchor_total_mae << '\n';
    std::cout << prefix
              << "terminal_value_nrmse=" << metric.terminal_value_nrmse << '\n';
    std::cout << prefix << "temporal_pair_concordance="
              << metric.temporal_pair_concordance << '\n';
    std::cout << prefix << "phase_future_direction_accuracy="
              << metric.phase_future_direction_accuracy << '\n';
    std::cout << prefix
              << "phase_future_correlation=" << metric.phase_future_correlation
              << '\n';
    std::cout << prefix << "phase_rmse_radians=" << metric.phase_rmse_radians
              << '\n';
    std::cout << prefix << "future_slope_direction_accuracy="
              << metric.future_slope_direction_accuracy << '\n';
  }
}

void print_seed_suite_ranges(const std::vector<SemanticMetrics> &draws) {
  for (std::size_t c = 0; c < kChannelHistories.size(); ++c) {
    double support_min = std::numeric_limits<double>::infinity();
    double support_max = -std::numeric_limits<double>::infinity();
    double terminal_support_min = std::numeric_limits<double>::infinity();
    double terminal_support_max = -std::numeric_limits<double>::infinity();
    double order_min = std::numeric_limits<double>::infinity();
    double phase_direction_min = std::numeric_limits<double>::infinity();
    double slope_direction_min = std::numeric_limits<double>::infinity();
    for (const auto &draw : draws) {
      const auto &metric = draw.channels[c];
      support_min = std::min(support_min, metric.support_retention);
      support_max = std::max(support_max, metric.support_retention);
      terminal_support_min =
          std::min(terminal_support_min, metric.terminal_support);
      terminal_support_max =
          std::max(terminal_support_max, metric.terminal_support);
      order_min = std::min(order_min, metric.temporal_pair_concordance);
      phase_direction_min =
          std::min(phase_direction_min, metric.phase_future_direction_accuracy);
      slope_direction_min =
          std::min(slope_direction_min, metric.future_slope_direction_accuracy);
    }
    const std::string prefix =
        "seed_suite.h" + std::to_string(kChannelHistories[c]) + ".";
    std::cout << prefix << "support_retention_min=" << support_min << '\n';
    std::cout << prefix << "support_retention_max=" << support_max << '\n';
    std::cout << prefix << "terminal_support_min=" << terminal_support_min
              << '\n';
    std::cout << prefix << "terminal_support_max=" << terminal_support_max
              << '\n';
    std::cout << prefix << "temporal_pair_concordance_min=" << order_min
              << '\n';
    std::cout << prefix
              << "phase_future_direction_accuracy_min=" << phase_direction_min
              << '\n';
    std::cout << prefix
              << "future_slope_direction_accuracy_min=" << slope_direction_min
              << '\n';
  }
  double coupling_max = 0.0;
  double lag_max = 0.0;
  for (const auto &draw : draws) {
    coupling_max = std::max(coupling_max, draw.cross_channel_coupling_nrmse);
    lag_max = std::max(lag_max, draw.cross_channel_lag_rmse_steps);
  }
  std::cout << "seed_suite.cross_channel.coupling_nrmse_max=" << coupling_max
            << '\n';
  std::cout << "seed_suite.cross_channel.lag_rmse_steps_max=" << lag_max
            << '\n';
}

void print_attribution_arm(const ArmEvaluation &arm) {
  const auto gate = classify_arm(arm);
  const std::string prefix = "attribution." + arm.name + ".";
  double terminal_support_min = std::numeric_limits<double>::infinity();
  double terminal_anchor_max = 0.0;
  double terminal_value_max = 0.0;
  double order_min = std::numeric_limits<double>::infinity();
  double phase_direction_min = std::numeric_limits<double>::infinity();
  double phase_correlation_min = std::numeric_limits<double>::infinity();
  double phase_rmse_max = 0.0;
  double slope_direction_min = std::numeric_limits<double>::infinity();
  for (const auto &channel : arm.aggregate.channels) {
    terminal_support_min =
        std::min(terminal_support_min, channel.terminal_support);
    terminal_anchor_max =
        std::max(terminal_anchor_max, channel.terminal_anchor_total_mae);
    terminal_value_max =
        std::max(terminal_value_max, channel.terminal_value_nrmse);
    order_min = std::min(order_min, channel.temporal_pair_concordance);
    phase_direction_min =
        std::min(phase_direction_min, channel.phase_future_direction_accuracy);
    phase_correlation_min =
        std::min(phase_correlation_min, channel.phase_future_correlation);
    phase_rmse_max = std::max(phase_rmse_max, channel.phase_rmse_radians);
    slope_direction_min =
        std::min(slope_direction_min, channel.future_slope_direction_accuracy);
  }

  std::array<double, kChannelCount> support_worst{};
  support_worst.fill(std::numeric_limits<double>::infinity());
  double terminal_support_worst = std::numeric_limits<double>::infinity();
  double order_worst = std::numeric_limits<double>::infinity();
  double phase_direction_worst = std::numeric_limits<double>::infinity();
  double slope_direction_worst = std::numeric_limits<double>::infinity();
  double coupling_worst = 0.0;
  double lag_worst = 0.0;
  for (const auto &draw : arm.draws) {
    coupling_worst =
        std::max(coupling_worst, draw.cross_channel_coupling_nrmse);
    lag_worst = std::max(lag_worst, draw.cross_channel_lag_rmse_steps);
    for (std::size_t c = 0; c < draw.channels.size(); ++c) {
      const auto &channel = draw.channels[c];
      support_worst[c] = std::min(support_worst[c], channel.support_retention);
      terminal_support_worst =
          std::min(terminal_support_worst, channel.terminal_support);
      order_worst = std::min(order_worst, channel.temporal_pair_concordance);
      phase_direction_worst = std::min(phase_direction_worst,
                                       channel.phase_future_direction_accuracy);
      slope_direction_worst = std::min(slope_direction_worst,
                                       channel.future_slope_direction_accuracy);
    }
  }

  for (std::size_t c = 0; c < arm.aggregate.channels.size(); ++c) {
    std::cout << prefix << "aggregate.support_retention_h"
              << kChannelHistories[c] << '='
              << arm.aggregate.channels[c].support_retention << '\n';
    std::cout << prefix << "worst.support_retention_h" << kChannelHistories[c]
              << '=' << support_worst[c] << '\n';
  }
  std::cout << prefix
            << "aggregate.terminal_support_min=" << terminal_support_min
            << '\n';
  std::cout << prefix
            << "aggregate.terminal_anchor_total_mae_max=" << terminal_anchor_max
            << '\n';
  std::cout << prefix
            << "aggregate.terminal_value_nrmse_max=" << terminal_value_max
            << '\n';
  std::cout << prefix << "aggregate.temporal_pair_concordance_min=" << order_min
            << '\n';
  std::cout << prefix << "aggregate.phase_future_direction_accuracy_min="
            << phase_direction_min << '\n';
  std::cout << prefix << "aggregate.phase_future_correlation_min="
            << phase_correlation_min << '\n';
  std::cout << prefix << "aggregate.phase_rmse_radians_max=" << phase_rmse_max
            << '\n';
  std::cout << prefix << "aggregate.future_slope_direction_accuracy_min="
            << slope_direction_min << '\n';
  std::cout << prefix << "aggregate.cross_channel_coupling_nrmse="
            << arm.aggregate.cross_channel_coupling_nrmse << '\n';
  std::cout << prefix << "aggregate.cross_channel_lag_rmse_steps="
            << arm.aggregate.cross_channel_lag_rmse_steps << '\n';
  std::cout << prefix << "worst.terminal_support_min=" << terminal_support_worst
            << '\n';
  std::cout << prefix << "worst.temporal_pair_concordance_min=" << order_worst
            << '\n';
  std::cout << prefix << "worst.phase_future_direction_accuracy_min="
            << phase_direction_worst << '\n';
  std::cout << prefix << "worst.future_slope_direction_accuracy_min="
            << slope_direction_worst << '\n';
  std::cout << prefix << "worst.cross_channel_coupling_nrmse=" << coupling_worst
            << '\n';
  std::cout << prefix << "worst.cross_channel_lag_rmse_steps=" << lag_worst
            << '\n';
  std::cout << prefix << "gate.support=" << gate.support << '\n';
  std::cout << prefix << "gate.terminal=" << gate.terminal << '\n';
  std::cout << prefix << "gate.temporal_order=" << gate.temporal_order << '\n';
  std::cout << prefix << "gate.phase=" << gate.phase << '\n';
  std::cout << prefix << "gate.future_direction=" << gate.future_direction
            << '\n';
  std::cout << prefix
            << "gate.cross_channel_coupling=" << gate.cross_channel_coupling
            << '\n';
  std::cout << prefix << "gate.cross_channel_lag=" << gate.cross_channel_lag
            << '\n';
  std::cout << prefix << "gate.every_draw=" << gate.every_draw << '\n';
  std::cout << prefix
            << "result=" << (gate.qualified ? "QUALIFIED" : "NOT_QUALIFIED")
            << '\n';
}

} // namespace

int main() {
  try {
    torch::NoGradGuard no_grad;
    const auto input = make_semantic_input();
    const auto dirty_input = with_dirty_masked_sentinel(input);

    torch::manual_seed(kSeed);
    const auto neutral =
        launcher::apply_mtf_training_augmentations(input, neutral_config());
    require_contract(torch::equal(neutral.data, input.data),
                     "neutral augmentation changed valid or masked data");
    require_contract(torch::equal(neutral.feature_mask, input.feature_mask),
                     "neutral augmentation changed the mask");

    std::vector<torch::Tensor> active_data;
    std::vector<torch::Tensor> active_masks;
    std::vector<SemanticMetrics> draw_metrics;
    active_data.reserve(kAugmentationDraws);
    active_masks.reserve(kAugmentationDraws);
    draw_metrics.reserve(kAugmentationDraws);
    bool fixed_seed_reproducible = true;
    bool dirty_masked_sentinel_parity = true;
    for (int64_t draw = 0; draw < kAugmentationDraws; ++draw) {
      const uint64_t seed = kSeed + static_cast<uint64_t>(draw);
      torch::manual_seed(seed);
      const auto active_draw =
          launcher::apply_mtf_training_augmentations(input, active_config());
      torch::manual_seed(seed);
      const auto repeated_draw =
          launcher::apply_mtf_training_augmentations(input, active_config());
      torch::manual_seed(seed);
      const auto dirty_draw = launcher::apply_mtf_training_augmentations(
          dirty_input, active_config());
      fixed_seed_reproducible =
          fixed_seed_reproducible &&
          torch::equal(active_draw.data, repeated_draw.data) &&
          torch::equal(active_draw.feature_mask, repeated_draw.feature_mask);
      dirty_masked_sentinel_parity =
          dirty_masked_sentinel_parity &&
          torch::equal(active_draw.data, dirty_draw.data) &&
          torch::equal(active_draw.feature_mask, dirty_draw.feature_mask);
      draw_metrics.push_back(evaluate_semantics(input, active_draw));
      active_data.push_back(active_draw.data);
      active_masks.push_back(active_draw.feature_mask);
    }
    const TestInput repeated_input{
        input.data.repeat({kAugmentationDraws, 1, 1, 1}),
        input.feature_mask.repeat({kAugmentationDraws, 1, 1, 1})};
    const mtf::mtf_input_t active{torch::cat(active_data, /*dim=*/0),
                                  torch::cat(active_masks, /*dim=*/0)};
    const auto metrics = evaluate_semantics(repeated_input, active);

    std::vector<ArmEvaluation> attribution;
    attribution.reserve(9);
    attribution.push_back(
        evaluate_arm(input, "neutral_reference", neutral_config()));
    for (const auto &[name, config] : leave_one_in_configs()) {
      attribution.push_back(evaluate_arm(input, name, config));
    }
    attribution.push_back(
        ArmEvaluation{"full_active_stack", metrics, draw_metrics});

    const bool support_preserved = all_channels(metrics, support_gate);
    const bool terminal_semantics_preserved =
        all_channels(metrics, terminal_gate);
    const bool temporal_order_preserved = all_channels(metrics, order_gate);
    const bool phase_semantics_preserved = all_channels(metrics, phase_gate);
    const bool future_direction_preserved =
        all_channels(metrics, future_direction_gate);
    const bool cross_channel_coupling_preserved =
        metrics.cross_channel_coupling_nrmse <= 0.05;
    const bool cross_channel_lag_preserved =
        metrics.cross_channel_lag_rmse_steps <= 0.25;
    const bool mask_only_future_shortcut_detected =
        metrics.mask_only_future_direction_accuracy > 0.55;
    const bool every_draw_passes_channel_gates = std::all_of(
        draw_metrics.begin(), draw_metrics.end(), semantic_gates_pass);
    const bool semantically_qualified =
        fixed_seed_reproducible && dirty_masked_sentinel_parity &&
        support_preserved && terminal_semantics_preserved &&
        temporal_order_preserved && phase_semantics_preserved &&
        future_direction_preserved && cross_channel_coupling_preserved &&
        cross_channel_lag_preserved && !mask_only_future_shortcut_detected &&
        every_draw_passes_channel_gates;

    std::cout << std::boolalpha << std::setprecision(9);
    std::cout << "schema_id=mtf_augmentation_semantic_qualification.v1\n";
    std::cout << "profile=light_phase_safe_v2\n";
    std::cout << "seed=" << kSeed << '\n';
    std::cout << "seed_suite_first=" << kSeed << '\n';
    std::cout << "seed_suite_last=" << (kSeed + kAugmentationDraws - 1) << '\n';
    std::cout << "temporal_draws=" << kAugmentationDraws << '\n';
    std::cout << "batch_size=" << kBatchSize << '\n';
    std::cout << "channel_histories=4,10,30\n";
    std::cout << "feature_width=" << kFeatureWidth << '\n';
    std::cout << "scope=model_training=false\n";
    std::cout << "scope.outer_augmentation_only=true\n";
    std::cout << "scope.leave_one_in_attribution=true\n";
    std::cout << "contract.neutral_identity=true\n";
    std::cout << "contract.fixed_seed_reproducible=" << fixed_seed_reproducible
              << '\n';
    std::cout << "contract.dirty_masked_sentinel_parity="
              << dirty_masked_sentinel_parity << '\n';
    std::cout << "threshold.minimum_support_retention=0.95\n";
    std::cout << "threshold.minimum_terminal_support=0.99\n";
    std::cout << "threshold.maximum_terminal_anchor_total_mae=0.05\n";
    std::cout << "threshold.maximum_terminal_value_nrmse=0.05\n";
    std::cout << "threshold.minimum_temporal_pair_concordance=0.995\n";
    std::cout << "threshold.minimum_phase_future_direction_accuracy=0.95\n";
    std::cout << "threshold.minimum_phase_future_correlation=0.95\n";
    std::cout << "threshold.maximum_phase_rmse_radians=0.25\n";
    std::cout << "threshold.minimum_future_slope_direction_accuracy=0.99\n";
    std::cout << "threshold.maximum_cross_channel_coupling_nrmse=0.05\n";
    std::cout << "threshold.maximum_cross_channel_lag_rmse_steps=0.25\n";
    std::cout << "threshold.maximum_mask_only_future_accuracy=0.55\n";
    std::cout << "attribution.arm_count=" << attribution.size() << '\n';
    for (const auto &arm : attribution) {
      print_attribution_arm(arm);
    }
    print_channel_metrics(metrics);
    print_seed_suite_ranges(draw_metrics);
    std::cout << "metric.cross_channel.coupling_nrmse="
              << metrics.cross_channel_coupling_nrmse << '\n';
    std::cout << "metric.cross_channel.lag_rmse_steps="
              << metrics.cross_channel_lag_rmse_steps << '\n';
    std::cout << "metric.mask_only.future_direction_accuracy="
              << metrics.mask_only_future_direction_accuracy << '\n';
    std::cout << "metric.mask_only.pattern_count=" << metrics.mask_pattern_count
              << '\n';
    std::cout << "metric.mask_only.structural_history_identity_visible="
              << metrics.structural_history_identity_visible << '\n';
    std::cout << "classification.support_preserved=" << support_preserved
              << '\n';
    std::cout << "classification.terminal_semantics_preserved="
              << terminal_semantics_preserved << '\n';
    std::cout << "classification.temporal_order_preserved="
              << temporal_order_preserved << '\n';
    std::cout << "classification.phase_semantics_preserved="
              << phase_semantics_preserved << '\n';
    std::cout << "classification.future_direction_preserved="
              << future_direction_preserved << '\n';
    std::cout << "classification.cross_channel_coupling_preserved="
              << cross_channel_coupling_preserved << '\n';
    std::cout << "classification.cross_channel_lag_preserved="
              << cross_channel_lag_preserved << '\n';
    std::cout << "classification.mask_only_future_shortcut_detected="
              << mask_only_future_shortcut_detected << '\n';
    std::cout << "classification.every_seed_suite_draw_passes="
              << every_draw_passes_channel_gates << '\n';
    std::cout << "classification.active_stack_semantically_qualified="
              << semantically_qualified << '\n';
    std::cout << "result="
              << (semantically_qualified ? "QUALIFIED" : "NOT_QUALIFIED")
              << '\n';
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "[CONTRACT_ERROR] torch error: " << error.what() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "[CONTRACT_ERROR] " << error.what() << '\n';
  }
  return 1;
}
