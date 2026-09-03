#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>

#include <torch/torch.h>

namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;

namespace {

constexpr double kRtol = 1.0e-9;
constexpr double kAtol = 1.0e-10;

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] bool close(const torch::Tensor &lhs, const torch::Tensor &rhs) {
  return lhs.sizes() == rhs.sizes() && torch::allclose(lhs, rhs, kRtol, kAtol);
}

[[nodiscard]] double maximum_abs_delta(const torch::Tensor &lhs,
                                       const torch::Tensor &rhs) {
  check(lhs.sizes() == rhs.sizes(), "delta operands have different shapes");
  return (lhs - rhs).abs().max().item<double>();
}

void check_metadata_equal(const mtf::mtf_token_metadata_t &lhs,
                          const mtf::mtf_token_metadata_t &rhs,
                          const std::string &message) {
  check(torch::equal(lhs.start_index, rhs.start_index) &&
            torch::equal(lhs.width, rhs.width) &&
            torch::equal(lhs.scale_id, rhs.scale_id) &&
            torch::equal(lhs.channel_id, rhs.channel_id) &&
            torch::equal(lhs.domain_id, rhs.domain_id),
        message);
}

void check_masks_equal(const mtf::mtf_token_batch_t &lhs,
                       const mtf::mtf_token_batch_t &rhs,
                       const std::string &message) {
  check(torch::equal(lhs.token_mask, rhs.token_mask) &&
            torch::equal(lhs.time_reconstruction_mask,
                         rhs.time_reconstruction_mask) &&
            torch::equal(lhs.frequency_reconstruction_mask,
                         rhs.frequency_reconstruction_mask),
        message);
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t
base_config(int64_t channels, int64_t history, int64_t features) {
  mtf::mtf_jepa_mae_vicreg_config_t config{};
  config.channel_count = channels;
  config.history_length = history;
  config.input_width = features;
  config.d_model = 32;
  config.latent_dim = 32;
  config.projector_dim = 64;
  config.predictor_hidden_dim = 64;
  config.num_encoder_layers = 2;
  config.num_predictor_layers = 2;
  config.num_decoder_layers = 1;
  config.num_heads = 4;
  config.dropout = 0.0;
  config.use_frequency_tokens = true;
  config.frequency_num_bins = 16;
  config.frequency_log_magnitude = true;
  config.augmentation_profile = "tokenizer_information_test";
  config.dtype = torch::kFloat64;
  config.device = torch::Device(torch::kCPU);
  return config;
}

[[nodiscard]] torch::Tensor
deterministic_non_palindrome(const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  auto input = torch::empty(
      {1, config.channel_count, config.history_length, config.input_width},
      torch::TensorOptions().dtype(config.dtype).device(config.device));
  auto values = input.accessor<double, 4>();
  for (int64_t channel = 0; channel < config.channel_count; ++channel) {
    for (int64_t time = 0; time < config.history_length; ++time) {
      for (int64_t feature = 0; feature < config.input_width; ++feature) {
        const double t = static_cast<double>(time);
        const double f = static_cast<double>(feature + 1);
        const double c = static_cast<double>(channel + 1);
        values[0][channel][time][feature] =
            0.013 * f * t * t + 0.071 * c * t + 0.19 * c - 0.11 * f +
            std::sin((0.17 + 0.013 * f) * t + 0.23 * c);
      }
    }
  }
  return input;
}

[[nodiscard]] torch::Tensor domain_indices(const mtf::mtf_token_batch_t &batch,
                                           int64_t domain) {
  return torch::nonzero(batch.metadata.domain_id.eq(domain)).reshape({-1});
}

void prove_full_history_reversal_collision() {
  auto config = base_config(/*channels=*/1, /*history=*/30, /*features=*/2);
  config.time_scales = {30};
  config.scale_strides = {30};

  const auto input = deterministic_non_palindrome(config);
  const auto reversed = input.flip({2});
  const double raw_delta = maximum_abs_delta(input, reversed);
  check(raw_delta > 1.0, "fixture is accidentally palindromic");

  torch::manual_seed(1201);
  auto builder = mtf::TimeFrequencyViewBuilder(config);
  builder->eval();
  torch::NoGradGuard no_grad;
  const auto original_tokens = builder->forward(input);
  const auto reversed_tokens = builder->forward(reversed);

  check_metadata_equal(original_tokens.metadata, reversed_tokens.metadata,
                       "full-history reversal changed token metadata");
  check_masks_equal(original_tokens, reversed_tokens,
                    "full-history reversal changed token masks");

  const auto time_indices = domain_indices(original_tokens, /*domain=*/0);
  const auto frequency_indices = domain_indices(original_tokens, /*domain=*/1);
  check(time_indices.numel() == 1 && frequency_indices.numel() == 1,
        "full-history-only builder did not produce one token per domain");

  const auto original_time =
      original_tokens.time_reconstruction_targets.index_select(1, time_indices);
  const auto reversed_time =
      reversed_tokens.time_reconstruction_targets.index_select(1, time_indices);
  const auto original_frequency =
      original_tokens.frequency_reconstruction_targets.index_select(
          1, frequency_indices);
  const auto reversed_frequency =
      reversed_tokens.frequency_reconstruction_targets.index_select(
          1, frequency_indices);

  check(close(original_time, reversed_time),
        "time mean/std descriptor retained reversal order");
  check(close(original_frequency, reversed_frequency),
        "frequency-magnitude descriptor retained reversal phase");
  check(close(original_tokens.tokens, reversed_tokens.tokens),
        "projected full-history tokenizer tokens retained reversal order");

  std::cout << "full_history.raw_max_abs_delta=" << raw_delta << '\n';
  std::cout << "full_history.time_descriptor_max_abs_delta="
            << maximum_abs_delta(original_time, reversed_time) << '\n';
  std::cout << "full_history.frequency_descriptor_max_abs_delta="
            << maximum_abs_delta(original_frequency, reversed_frequency)
            << '\n';
  std::cout << "full_history.projected_token_max_abs_delta="
            << maximum_abs_delta(original_tokens.tokens, reversed_tokens.tokens)
            << '\n';
}

void characterize_active_plan() {
  auto config = base_config(/*channels=*/3, /*history=*/30, /*features=*/9);
  config.time_scales = {8, 16, 32, 64};
  config.scale_strides = {4, 8, 16, 32};

  const auto input = deterministic_non_palindrome(config);
  const auto reversed = input.flip({2});
  torch::manual_seed(1202);
  auto builder = mtf::TimeFrequencyViewBuilder(config);
  builder->eval();
  torch::NoGradGuard no_grad;
  const auto original_tokens = builder->forward(input);
  const auto reversed_tokens = builder->forward(reversed);

  check_metadata_equal(original_tokens.metadata, reversed_tokens.metadata,
                       "active-plan reversal changed token metadata");
  check_masks_equal(original_tokens, reversed_tokens,
                    "active-plan reversal changed token masks");
  check(original_tokens.tokens.size(1) == 72,
        "active plan no longer produces 72 tokens at H=30");

  const auto starts =
      original_tokens.metadata.start_index.to(torch::kCPU).contiguous();
  const auto widths =
      original_tokens.metadata.width.to(torch::kCPU).contiguous();
  const auto scales =
      original_tokens.metadata.scale_id.to(torch::kCPU).contiguous();
  const auto domains =
      original_tokens.metadata.domain_id.to(torch::kCPU).contiguous();
  const auto start = starts.accessor<int64_t, 1>();
  const auto width = widths.accessor<int64_t, 1>();
  const auto scale = scales.accessor<int64_t, 1>();
  const auto domain = domains.accessor<int64_t, 1>();

  int64_t clipped_full_history_count = 0;
  int64_t shorter_window_count = 0;
  int64_t shorter_projected_tokens_changed = 0;
  double clipped_max_delta = 0.0;
  double shorter_max_delta = 0.0;

  for (int64_t token = 0; token < original_tokens.tokens.size(1); ++token) {
    const auto original_projected = original_tokens.tokens.index({0, token});
    const auto reversed_projected = reversed_tokens.tokens.index({0, token});
    const double projected_delta =
        maximum_abs_delta(original_projected, reversed_projected);
    const bool clipped_full_history = start[token] == 0 && width[token] == 30 &&
                                      (scale[token] == 2 || scale[token] == 3);
    if (clipped_full_history) {
      ++clipped_full_history_count;
      clipped_max_delta = std::max(clipped_max_delta, projected_delta);
      check(close(original_projected, reversed_projected),
            "active scale-32/64 full-history token retained reversal order");

      const auto original_descriptor =
          domain[token] == 0
              ? original_tokens.time_reconstruction_targets.index({0, token})
              : original_tokens.frequency_reconstruction_targets.index(
                    {0, token});
      const auto reversed_descriptor =
          domain[token] == 0
              ? reversed_tokens.time_reconstruction_targets.index({0, token})
              : reversed_tokens.frequency_reconstruction_targets.index(
                    {0, token});
      check(close(original_descriptor, reversed_descriptor),
            "active clipped full-history descriptor retained reversal order");
      continue;
    }

    check(width[token] < 30,
          "unexpected non-clipped token in exact-active window plan");
    ++shorter_window_count;
    shorter_max_delta = std::max(shorter_max_delta, projected_delta);
    if (!close(original_projected, reversed_projected)) {
      ++shorter_projected_tokens_changed;
    }
  }

  check(clipped_full_history_count == 12,
        "active scales 32/64 did not yield 12 clipped domain/channel tokens");
  check(shorter_window_count == 60,
        "active scales 8/16 did not yield 60 shorter-window tokens");
  check(shorter_projected_tokens_changed == 60,
        "not every active shorter-window token changed under reversal");

  std::cout << "active_plan.total_token_count="
            << original_tokens.tokens.size(1) << '\n';
  std::cout << "active_plan.clipped_full_history_token_count="
            << clipped_full_history_count << '\n';
  std::cout << "active_plan.clipped_full_history_projected_max_abs_delta="
            << clipped_max_delta << '\n';
  std::cout << "active_plan.shorter_window_token_count=" << shorter_window_count
            << '\n';
  std::cout << "active_plan.shorter_projected_tokens_changed="
            << shorter_projected_tokens_changed << '\n';
  std::cout << "active_plan.shorter_projected_max_abs_delta="
            << shorter_max_delta << '\n';
}

} // namespace

int main() {
  try {
    at::set_num_threads(1);
    std::cout << std::setprecision(17);
    prove_full_history_reversal_collision();
    characterize_active_plan();
    std::cout << "[PASS] tokenizer sequence-information limits\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] tokenizer sequence-information limits: "
              << error.what() << '\n';
    return 1;
  }
}
