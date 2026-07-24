#include "jkimyei/training/representation/mtf_jepa_mae_vicreg_graph_first_launcher.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include <torch/torch.h>

namespace launcher = cuwacunu::jkimyei::training::representation::
    mtf_jepa_mae_vicreg_graph_first_launcher_detail;
namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;

namespace {

constexpr int64_t kHistory = 30;
constexpr std::array<int64_t, 3> kChannelHistories{4, 10, 30};
constexpr double kTwoPi = 6.28318530717958647692;

struct TestInput {
  torch::Tensor data;
  torch::Tensor feature_mask;
};

struct PhaseMetric {
  int64_t count{};
  int64_t sign_correct{};
  double prediction_sum{};
  double target_sum{};
  double prediction_sq_sum{};
  double target_sq_sum{};
  double cross_sum{};
  double phase_sq_error{};
};

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

TestInput make_left_padded_input(int64_t batch_size = 1) {
  const auto value_count = static_cast<std::size_t>(batch_size * 3 * kHistory);
  std::vector<float> data_values(value_count, 0.0F);
  std::vector<uint8_t> mask_values(value_count, uint8_t{0});
  const auto offset = [](int64_t b, int64_t c, int64_t t) {
    return static_cast<std::size_t>((b * 3 + c) * kHistory + t);
  };
  for (int64_t b = 0; b < batch_size; ++b) {
    for (int64_t c = 0; c < 3; ++c) {
      const auto length = kChannelHistories[static_cast<std::size_t>(c)];
      const auto start = kHistory - length;
      for (int64_t t = start; t < kHistory; ++t) {
        data_values[offset(b, c, t)] =
            static_cast<float>(1000 * b + 100 * c + t);
        mask_values[offset(b, c, t)] = uint8_t{1};
      }
    }
  }
  auto data = torch::from_blob(data_values.data(), {batch_size, 3, kHistory, 1},
                               torch::kFloat32)
                  .clone();
  auto mask = torch::from_blob(mask_values.data(), {batch_size, 3, kHistory, 1},
                               torch::kUInt8)
                  .to(torch::kBool);
  return {data, mask};
}

std::array<int64_t, 3> valid_counts(const torch::Tensor &mask) {
  std::array<int64_t, 3> counts{};
  for (int64_t c = 0; c < 3; ++c) {
    counts[static_cast<std::size_t>(c)] =
        mask.select(1, c).sum().item<int64_t>();
  }
  return counts;
}

std::array<double, 3> retention_by_channel(const torch::Tensor &before,
                                           const torch::Tensor &after) {
  std::array<double, 3> retention{};
  const auto before_counts = valid_counts(before);
  const auto after_counts = valid_counts(after);
  for (std::size_t c = 0; c < retention.size(); ++c) {
    retention[c] = static_cast<double>(after_counts[c]) /
                   static_cast<double>(before_counts[c]);
  }
  return retention;
}

mtf::mtf_jepa_mae_vicreg_config_t neutral_config() {
  mtf::mtf_jepa_mae_vicreg_config_t config;
  config.channel_count = 3;
  config.history_length = kHistory;
  config.input_width = 1;
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

mtf::mtf_jepa_mae_vicreg_config_t active_phase_safe_config() {
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

void test_temporal_resample_identity_contract() {
  const auto input = make_left_padded_input();
  std::vector<double> positions(kHistory);
  std::iota(positions.begin(), positions.end(), 0.0);
  const auto sampled = launcher::temporal_resample_linear(
      input.data, input.feature_mask, positions);
  check(torch::equal(sampled.first, input.data),
        "identity temporal resample changed data");
  check(torch::equal(sampled.second, input.feature_mask),
        "identity temporal resample changed mask");
}

void test_temporal_resample_left_padding_erosion() {
  const auto input = make_left_padded_input();
  std::vector<double> positions(kHistory);
  for (int64_t t = 0; t < kHistory; ++t) {
    positions[static_cast<std::size_t>(t)] = static_cast<double>(t) + 0.25;
  }
  const auto once = launcher::temporal_resample_linear(
      input.data, input.feature_mask, positions);
  const auto twice =
      launcher::temporal_resample_linear(once.first, once.second, positions);
  check(valid_counts(once.second) == std::array<int64_t, 3>{3, 9, 29},
        "one interpolating resample did not erode histories to 3/9/29");
  check(valid_counts(twice.second) == std::array<int64_t, 3>{2, 8, 28},
        "two interpolating resamples did not erode histories to 2/8/28");
}

void test_outer_no_augmentation_is_identity() {
  const auto input = make_left_padded_input(/*batch_size=*/8);
  torch::manual_seed(17);
  const auto output =
      launcher::apply_mtf_training_augmentations(input, neutral_config());
  check(torch::equal(output.data, input.data),
        "neutral outer augmentation changed data");
  check(torch::equal(output.feature_mask, input.feature_mask),
        "neutral outer augmentation changed mask");
  const auto retention =
      retention_by_channel(input.feature_mask, output.feature_mask);
  check(retention == std::array<double, 3>{1.0, 1.0, 1.0},
        "neutral outer augmentation did not retain every valid feature");
}

void test_active_augmentation_is_reproducible_and_erodes_history() {
  const auto input = make_left_padded_input(/*batch_size=*/64);
  const auto config = active_phase_safe_config();
  torch::manual_seed(17);
  const auto first = launcher::apply_mtf_training_augmentations(input, config);
  torch::manual_seed(17);
  const auto repeated =
      launcher::apply_mtf_training_augmentations(input, config);
  torch::manual_seed(18);
  const auto different =
      launcher::apply_mtf_training_augmentations(input, config);
  check(torch::equal(first.data, repeated.data) &&
            torch::equal(first.feature_mask, repeated.feature_mask),
        "fixed seed did not reproduce active outer augmentation");
  check(!torch::equal(first.data, different.data) ||
            !torch::equal(first.feature_mask, different.feature_mask),
        "different seeds produced identical active outer augmentation");
  const auto retention =
      retention_by_channel(input.feature_mask, first.feature_mask);
  check(retention[0] < 1.0 && retention[1] < 1.0 && retention[2] < 1.0,
        "active temporal augmentation unexpectedly retained all histories");
  std::cout << "active_retention_h4=" << retention[0]
            << " active_retention_h10=" << retention[1]
            << " active_retention_h30=" << retention[2] << '\n';
}

TestInput make_periodic_input(int64_t batch_size, std::vector<double> &target,
                              std::vector<double> &phase) {
  auto input = make_left_padded_input(batch_size);
  input.data.zero_();
  auto data = input.data.accessor<float, 4>();
  target.resize(static_cast<std::size_t>(batch_size));
  phase.resize(static_cast<std::size_t>(batch_size));
  const double omega = kTwoPi / 8.0;
  for (int64_t b = 0; b < batch_size; ++b) {
    const double sample_phase = kTwoPi * (static_cast<double>(b) + 0.37) /
                                static_cast<double>(batch_size);
    phase[static_cast<std::size_t>(b)] = sample_phase;
    target[static_cast<std::size_t>(b)] =
        std::sin(omega * static_cast<double>(kHistory) + sample_phase);
    for (int64_t c = 0; c < 3; ++c) {
      const auto length = kChannelHistories[static_cast<std::size_t>(c)];
      for (int64_t t = kHistory - length; t < kHistory; ++t) {
        data[b][c][t][0] = static_cast<float>(
            std::sin(omega * static_cast<double>(t) + sample_phase));
      }
    }
  }
  return input;
}

double circular_difference(double lhs, double rhs) {
  return std::atan2(std::sin(lhs - rhs), std::cos(lhs - rhs));
}

std::array<PhaseMetric, 3> score_phase(const mtf::mtf_input_t &input,
                                       const std::vector<double> &target,
                                       const std::vector<double> &phase) {
  std::array<PhaseMetric, 3> metrics{};
  const double omega = kTwoPi / 8.0;
  const auto data = input.data.to(torch::kCPU).contiguous();
  const auto mask = input.feature_mask.to(torch::kCPU).contiguous();
  const auto d = data.accessor<float, 4>();
  const auto m = mask.accessor<bool, 4>();
  for (int64_t b = 0; b < data.size(0); ++b) {
    for (int64_t c = 0; c < 3; ++c) {
      double ss = 0.0;
      double sc = 0.0;
      double cc = 0.0;
      double sy = 0.0;
      double cy = 0.0;
      for (int64_t t = 0; t < kHistory; ++t) {
        if (!m[b][c][t][0]) {
          continue;
        }
        const auto sine = std::sin(omega * static_cast<double>(t));
        const auto cosine = std::cos(omega * static_cast<double>(t));
        const auto value = static_cast<double>(d[b][c][t][0]);
        ss += sine * sine;
        sc += sine * cosine;
        cc += cosine * cosine;
        sy += sine * value;
        cy += cosine * value;
      }
      const auto determinant = ss * cc - sc * sc;
      if (std::fabs(determinant) < 1.0e-10) {
        continue;
      }
      const auto sine_coefficient = (sy * cc - cy * sc) / determinant;
      const auto cosine_coefficient = (cy * ss - sy * sc) / determinant;
      const auto prediction =
          sine_coefficient * std::sin(omega * static_cast<double>(kHistory)) +
          cosine_coefficient * std::cos(omega * static_cast<double>(kHistory));
      const auto actual = target[static_cast<std::size_t>(b)];
      if (std::fabs(actual) <= 0.001) {
        continue;
      }
      auto &metric = metrics[static_cast<std::size_t>(c)];
      ++metric.count;
      if ((prediction > 0.0) == (actual > 0.0)) {
        ++metric.sign_correct;
      }
      metric.prediction_sum += prediction;
      metric.target_sum += actual;
      metric.prediction_sq_sum += prediction * prediction;
      metric.target_sq_sum += actual * actual;
      metric.cross_sum += prediction * actual;
      const auto estimated_phase =
          std::atan2(cosine_coefficient, sine_coefficient);
      const auto phase_error = circular_difference(
          estimated_phase, phase[static_cast<std::size_t>(b)]);
      metric.phase_sq_error += phase_error * phase_error;
    }
  }
  return metrics;
}

double correlation(const PhaseMetric &metric) {
  const auto count = static_cast<double>(metric.count);
  const auto numerator =
      metric.cross_sum - metric.prediction_sum * metric.target_sum / count;
  const auto left = metric.prediction_sq_sum -
                    metric.prediction_sum * metric.prediction_sum / count;
  const auto right =
      metric.target_sq_sum - metric.target_sum * metric.target_sum / count;
  return numerator / std::sqrt(left * right);
}

void test_periodic_direction_preservation_characterization() {
  std::vector<double> target;
  std::vector<double> phase;
  const auto input = make_periodic_input(/*batch_size=*/128, target, phase);
  torch::manual_seed(17);
  const auto neutral =
      launcher::apply_mtf_training_augmentations(input, neutral_config());
  torch::manual_seed(17);
  const auto active = launcher::apply_mtf_training_augmentations(
      input, active_phase_safe_config());
  const auto neutral_metrics = score_phase(neutral, target, phase);
  const auto active_metrics = score_phase(active, target, phase);
  for (std::size_t c = 0; c < neutral_metrics.size(); ++c) {
    const auto &baseline = neutral_metrics[c];
    const auto &augmented = active_metrics[c];
    check(baseline.count > 0 && augmented.count > 0,
          "periodic phase probe produced no valid predictions");
    const auto baseline_direction =
        static_cast<double>(baseline.sign_correct) / baseline.count;
    const auto baseline_correlation = correlation(baseline);
    const auto baseline_phase_rmse =
        std::sqrt(baseline.phase_sq_error / baseline.count);
    check(baseline_direction == 1.0 && baseline_correlation > 0.999999 &&
              baseline_phase_rmse < 1.0e-6,
          "neutral augmentation did not preserve the periodic oracle");
    const auto active_direction =
        static_cast<double>(augmented.sign_correct) / augmented.count;
    const auto active_correlation = correlation(augmented);
    const auto active_phase_rmse =
        std::sqrt(augmented.phase_sq_error / augmented.count);
    check(std::isfinite(active_direction) &&
              std::isfinite(active_correlation) &&
              std::isfinite(active_phase_rmse),
          "active phase characterization was non-finite");
    std::cout << "phase_probe_h"
              << kChannelHistories[static_cast<std::size_t>(c)]
              << "_direction=" << active_direction
              << " correlation=" << active_correlation
              << " phase_rmse=" << active_phase_rmse << '\n';
  }
}

} // namespace

int main() {
  try {
    test_temporal_resample_identity_contract();
    test_temporal_resample_left_padding_erosion();
    test_outer_no_augmentation_is_identity();
    test_active_augmentation_is_reproducible_and_erodes_history();
    test_periodic_direction_preservation_characterization();
    std::cout << "[PASS] MTF augmentation contracts and phase diagnostics\n";
    return 0;
  } catch (const c10::Error &error) {
    std::cerr << "[FAIL] torch error: " << error.what() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] " << error.what() << '\n';
  }
  return 1;
}
