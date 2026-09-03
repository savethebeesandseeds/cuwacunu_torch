#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

#include <torch/torch.h>

namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;

namespace {

constexpr int64_t kChannels = 3;
constexpr int64_t kHistory = 30;
constexpr int64_t kFeatures = 9;
constexpr int64_t kExpectedTokens = 72;
constexpr int64_t kExpectedTargets = 2;
constexpr int64_t kExpectedContexts = 54;
constexpr int64_t kAuditSteps = 512;
constexpr std::array<int64_t, 3> kSeeds{17, 31, 47};

void check(bool condition, const std::string &message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

[[nodiscard]] uint64_t splitmix64(uint64_t value) {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

[[nodiscard]] int64_t paired_step_seed(int64_t model_seed, int64_t step) {
  const auto mixed =
      splitmix64(0x6f626a5f6d61736bULL ^ static_cast<uint64_t>(model_seed) ^
                 (static_cast<uint64_t>(step) << 32U));
  return static_cast<int64_t>(mixed & 0x7fffffffffffffffULL);
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t active_config() {
  mtf::mtf_jepa_mae_vicreg_config_t config{};
  config.channel_count = kChannels;
  config.history_length = kHistory;
  config.input_width = kFeatures;
  config.d_model = 32;
  config.latent_dim = 32;
  config.projector_dim = 64;
  config.predictor_hidden_dim = 64;
  config.num_encoder_layers = 2;
  config.num_predictor_layers = 2;
  config.num_decoder_layers = 1;
  config.num_heads = 4;
  config.dropout = 0.0;
  config.time_scales = {8, 16, 32, 64};
  config.scale_strides = {4, 8, 16, 32};
  config.use_frequency_tokens = true;
  config.frequency_num_bins = 16;
  config.frequency_log_magnitude = true;
  config.mask_ratio_time = 0.10;
  config.mask_ratio_frequency = 0.05;
  config.mask_ratio_channel = 0.0;
  config.min_context_ratio = 0.75;
  config.couple_time_frequency_masks = false;
  config.mask_same_window_across_domains = false;
  config.mask_same_channel_block = false;
  config.max_context_target_time_overlap = 0.50;
  config.augmentation_profile = "light_phase_safe_v2";
  config.dtype = torch::kFloat32;
  config.device = torch::Device(torch::kCPU);
  return config;
}

[[nodiscard]] torch::Tensor deterministic_input() {
  const int64_t elements = kChannels * kHistory * kFeatures;
  auto index =
      torch::arange(
          elements,
          torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU))
          .reshape({1, kChannels, kHistory, kFeatures});
  return torch::sin(index * 0.017) + 0.35 * torch::cos(index * 0.031) +
         0.001 * index;
}

struct Metadata {
  std::vector<int64_t> start{};
  std::vector<int64_t> width{};
  std::vector<int64_t> scale{};
  std::vector<int64_t> channel{};
  std::vector<int64_t> domain{};
};

[[nodiscard]] std::vector<int64_t> tensor_vector(const torch::Tensor &value) {
  const auto cpu = value.to(torch::kCPU, torch::kInt64).contiguous();
  const auto *data = cpu.data_ptr<int64_t>();
  return {data, data + cpu.numel()};
}

[[nodiscard]] Metadata metadata_from(const mtf::mtf_token_batch_t &batch) {
  return {.start = tensor_vector(batch.metadata.start_index),
          .width = tensor_vector(batch.metadata.width),
          .scale = tensor_vector(batch.metadata.scale_id),
          .channel = tensor_vector(batch.metadata.channel_id),
          .domain = tensor_vector(batch.metadata.domain_id)};
}

[[nodiscard]] std::vector<int64_t> selected(const torch::Tensor &mask) {
  const auto cpu = mask.to(torch::kCPU, torch::kBool).contiguous();
  check(cpu.dim() == 2 && cpu.size(0) == 1,
        "IMA-2 mask must contain one sample");
  auto values = cpu.accessor<bool, 2>();
  std::vector<int64_t> result;
  for (int64_t token = 0; token < cpu.size(1); ++token) {
    if (values[0][token]) {
      result.push_back(token);
    }
  }
  return result;
}

[[nodiscard]] bool raw_support_overlaps(const Metadata &metadata, int64_t left,
                                        int64_t right) {
  if (metadata.channel[static_cast<std::size_t>(left)] !=
      metadata.channel[static_cast<std::size_t>(right)]) {
    return false;
  }
  const int64_t left_end = metadata.start[static_cast<std::size_t>(left)] +
                           metadata.width[static_cast<std::size_t>(left)];
  const int64_t right_end = metadata.start[static_cast<std::size_t>(right)] +
                            metadata.width[static_cast<std::size_t>(right)];
  return std::max(metadata.start[static_cast<std::size_t>(left)],
                  metadata.start[static_cast<std::size_t>(right)]) <
         std::min(left_end, right_end);
}

struct PairCandidate {
  int64_t time{0};
  int64_t frequency{0};
};

[[nodiscard]] std::vector<PairCandidate>
finest_pairs(const Metadata &metadata) {
  int64_t minimum_width = std::numeric_limits<int64_t>::max();
  std::vector<PairCandidate> result;
  for (int64_t time = 0; time < kExpectedTokens; ++time) {
    if (metadata.domain[static_cast<std::size_t>(time)] != 0) {
      continue;
    }
    for (int64_t frequency = 0; frequency < kExpectedTokens; ++frequency) {
      if (metadata.domain[static_cast<std::size_t>(frequency)] != 1 ||
          metadata.channel[static_cast<std::size_t>(time)] !=
              metadata.channel[static_cast<std::size_t>(frequency)] ||
          metadata.scale[static_cast<std::size_t>(time)] !=
              metadata.scale[static_cast<std::size_t>(frequency)] ||
          metadata.start[static_cast<std::size_t>(time)] !=
              metadata.start[static_cast<std::size_t>(frequency)] ||
          metadata.width[static_cast<std::size_t>(time)] !=
              metadata.width[static_cast<std::size_t>(frequency)]) {
        continue;
      }
      const int64_t width = metadata.width[static_cast<std::size_t>(time)];
      if (width < minimum_width) {
        minimum_width = width;
        result.clear();
      }
      if (width == minimum_width) {
        result.push_back({time, frequency});
      }
    }
  }
  return result;
}

[[nodiscard]] int64_t strict_capacity(const Metadata &metadata,
                                      const PairCandidate &pair) {
  int64_t result = 0;
  for (int64_t token = 0; token < kExpectedTokens; ++token) {
    if (token == pair.time || token == pair.frequency) {
      continue;
    }
    if (!raw_support_overlaps(metadata, token, pair.time)) {
      ++result;
    }
  }
  return result;
}

[[nodiscard]] bool
mask_outputs_equal(const mtf::jepa_context_target_mask_t &left,
                   const mtf::jepa_context_target_mask_t &right) {
  return torch::equal(left.context_mask, right.context_mask) &&
         torch::equal(left.target_mask, right.target_mask) &&
         torch::equal(left.valid_mask, right.valid_mask) &&
         torch::equal(left.mask_ratio_actual, right.mask_ratio_actual) &&
         left.num_context_tokens == right.num_context_tokens &&
         left.num_target_tokens == right.num_target_tokens &&
         left.hard_forbidden_count == right.hard_forbidden_count &&
         left.soft_forbidden_count == right.soft_forbidden_count &&
         left.relaxed_soft_forbidden_count ==
             right.relaxed_soft_forbidden_count &&
         left.reduced_targets_for_context_count ==
             right.reduced_targets_for_context_count &&
         left.context_starved_sample_count ==
             right.context_starved_sample_count &&
         left.min_context_satisfied_count ==
             right.min_context_satisfied_count &&
         left.support_separated_sample_count ==
             right.support_separated_sample_count &&
         left.retained_legacy_context_count ==
             right.retained_legacy_context_count &&
         left.replaced_context_count == right.replaced_context_count;
}

[[nodiscard]] std::size_t
selected_pair_index(const std::vector<PairCandidate> &pairs,
                    const std::vector<int64_t> &targets) {
  for (std::size_t index = 0; index < pairs.size(); ++index) {
    if (targets.size() == 2 &&
        std::find(targets.begin(), targets.end(), pairs[index].time) !=
            targets.end() &&
        std::find(targets.begin(), targets.end(), pairs[index].frequency) !=
            targets.end()) {
      return index;
    }
  }
  return pairs.size();
}

} // namespace

int main() {
  try {
    at::set_num_threads(1);
    std::cout << std::boolalpha << std::setprecision(17);
    auto legacy_config = active_config();
    auto repaired_config = legacy_config;
    repaired_config.jepa_mask_policy =
        mtf::mtf_jepa_mask_policy_t::support_separated_pair_v1;

    const auto data = deterministic_input();
    const auto feature_mask =
        torch::ones_like(data, torch::TensorOptions().dtype(torch::kBool));
    auto tokenizer = mtf::TimeFrequencyViewBuilder(legacy_config);
    tokenizer->eval();
    mtf::mtf_token_batch_t tokens{};
    {
      torch::NoGradGuard no_grad;
      tokens = tokenizer->forward(data, feature_mask);
    }
    check(tokens.tokens.size(1) == kExpectedTokens,
          "IMA-2 expected the active 72-token layout");
    const auto metadata = metadata_from(tokens);
    const auto pairs = finest_pairs(metadata);
    check(pairs.size() == 21,
          "IMA-2 expected 21 finest-scale time/frequency target pairs");

    int64_t capacity_min = kExpectedTokens;
    int64_t capacity_max = 0;
    for (const auto &pair : pairs) {
      const int64_t capacity = strict_capacity(metadata, pair);
      capacity_min = std::min(capacity_min, capacity);
      capacity_max = std::max(capacity_max, capacity);
    }
    check(capacity_min >= kExpectedContexts,
          "IMA-2 exhaustive geometry cannot supply 54 strict contexts");

    mtf::JEPAContextTargetMasker legacy_masker(legacy_config);
    mtf::JEPAContextTargetMasker repaired_masker(repaired_config);
    torch::manual_seed(paired_step_seed(kSeeds.front(), 0));
    const auto default_legacy = legacy_masker.create_masks(tokens);
    auto explicit_legacy_config = legacy_config;
    explicit_legacy_config.jepa_mask_policy =
        mtf::mtf_jepa_mask_policy_t::legacy_soft_overlap;
    mtf::JEPAContextTargetMasker explicit_legacy_masker(explicit_legacy_config);
    torch::manual_seed(paired_step_seed(kSeeds.front(), 0));
    const auto explicit_legacy = explicit_legacy_masker.create_masks(tokens);
    const bool rollback_exact =
        mask_outputs_equal(default_legacy, explicit_legacy);
    check(rollback_exact, "IMA-2 default legacy rollback changed");

    int64_t mechanics_failures = 0;
    int64_t rng_schedule_failures = 0;
    int64_t deterministic_replay_failures = 0;
    int64_t retained_context_min = kExpectedContexts;
    int64_t retained_context_max = 0;
    int64_t replacement_total = 0;
    std::vector<int64_t> pair_selection_counts(pairs.size(), 0);
    for (const int64_t seed : kSeeds) {
      for (int64_t step = 0; step < kAuditSteps; ++step) {
        const int64_t schedule_seed = paired_step_seed(seed, step);
        torch::manual_seed(schedule_seed);
        static_cast<void>(legacy_masker.create_masks(tokens));
        const auto legacy_rng_state =
            at::detail::getDefaultCPUGenerator().get_state().clone();

        torch::manual_seed(schedule_seed);
        const auto repaired = repaired_masker.create_masks(tokens);
        const auto repaired_rng_state =
            at::detail::getDefaultCPUGenerator().get_state().clone();
        rng_schedule_failures +=
            torch::equal(legacy_rng_state, repaired_rng_state) ? 0 : 1;

        torch::manual_seed(schedule_seed);
        const auto replayed = repaired_masker.create_masks(tokens);
        deterministic_replay_failures +=
            mask_outputs_equal(repaired, replayed) ? 0 : 1;

        const auto targets = selected(repaired.target_mask);
        const auto contexts = selected(repaired.context_mask);
        const std::size_t pair_index = selected_pair_index(pairs, targets);
        bool separated = true;
        for (const int64_t context : contexts) {
          for (const int64_t target : targets) {
            separated =
                separated && !raw_support_overlaps(metadata, context, target);
          }
        }
        const bool tensor_disjoint =
            !repaired.target_mask.logical_and(repaired.context_mask)
                 .any()
                 .item<bool>();
        const bool subsets =
            !repaired.target_mask.logical_and(repaired.valid_mask.logical_not())
                 .any()
                 .item<bool>() &&
            !repaired.context_mask
                 .logical_and(repaired.valid_mask.logical_not())
                 .any()
                 .item<bool>();
        const bool diagnostics =
            repaired.num_target_tokens == kExpectedTargets &&
            repaired.num_context_tokens == kExpectedContexts &&
            repaired.soft_forbidden_count == 0 &&
            repaired.relaxed_soft_forbidden_count == 0 &&
            repaired.reduced_targets_for_context_count == 0 &&
            repaired.context_starved_sample_count == 0 &&
            repaired.min_context_satisfied_count == 1 &&
            repaired.support_separated_sample_count == 1 &&
            repaired.retained_legacy_context_count +
                    repaired.replaced_context_count ==
                kExpectedContexts;
        if (targets.size() != kExpectedTargets ||
            contexts.size() != kExpectedContexts ||
            pair_index == pairs.size() || !separated || !tensor_disjoint ||
            !subsets || !diagnostics) {
          ++mechanics_failures;
        } else {
          ++pair_selection_counts[pair_index];
        }
        retained_context_min = std::min(retained_context_min,
                                        repaired.retained_legacy_context_count);
        retained_context_max = std::max(retained_context_max,
                                        repaired.retained_legacy_context_count);
        replacement_total += repaired.replaced_context_count;
      }
    }

    bool fail_closed = false;
    auto starved_tokens = tokens;
    const auto channel_ids = tokens.metadata.channel_id.to(torch::kCPU);
    starved_tokens.token_mask =
        channel_ids.eq(0).unsqueeze(0).to(tokens.token_mask.device());
    try {
      torch::manual_seed(paired_step_seed(kSeeds.front(), 0));
      static_cast<void>(repaired_masker.create_masks(starved_tokens));
    } catch (const std::exception &error) {
      fail_closed = std::string(error.what())
                        .find("support-separated JEPA context is geometrically "
                              "infeasible") != std::string::npos;
    }
    check(fail_closed, "IMA-2 infeasible sample did not fail closed");

    const int64_t selected_pair_count = static_cast<int64_t>(std::count_if(
        pair_selection_counts.begin(), pair_selection_counts.end(),
        [](int64_t count) { return count > 0; }));
    const int64_t selection_min = *std::min_element(
        pair_selection_counts.begin(), pair_selection_counts.end());
    const int64_t selection_max = *std::max_element(
        pair_selection_counts.begin(), pair_selection_counts.end());
    const bool pass = capacity_min >= kExpectedContexts && rollback_exact &&
                      mechanics_failures == 0 && rng_schedule_failures == 0 &&
                      deterministic_replay_failures == 0 && fail_closed &&
                      selected_pair_count == static_cast<int64_t>(pairs.size());

    std::cout << "ima2.optimizer_steps=0\n";
    std::cout << "ima2.ema_updates=0\n";
    std::cout << "ima2.policy="
              << mtf::mtf_jepa_mask_policy_name(
                     repaired_config.jepa_mask_policy)
              << '\n';
    std::cout << "ima2.target_contract=time_frequency_pair\n";
    std::cout << "ima2.target_tokens=" << kExpectedTargets << '\n';
    std::cout << "ima2.context_tokens=" << kExpectedContexts << '\n';
    std::cout << "ima2.exhaustive_pair_candidates=" << pairs.size() << '\n';
    std::cout << "ima2.exhaustive_strict_capacity_min=" << capacity_min << '\n';
    std::cout << "ima2.exhaustive_strict_capacity_max=" << capacity_max << '\n';
    std::cout << "ima2.scheduled_samples="
              << static_cast<int64_t>(kSeeds.size()) * kAuditSteps << '\n';
    std::cout << "ima2.scheduled_mechanics_failures=" << mechanics_failures
              << '\n';
    std::cout << "ima2.rng_schedule_failures=" << rng_schedule_failures << '\n';
    std::cout << "ima2.deterministic_replay_failures="
              << deterministic_replay_failures << '\n';
    std::cout << "ima2.selected_pair_candidates=" << selected_pair_count
              << '\n';
    std::cout << "ima2.pair_selection_count_min=" << selection_min << '\n';
    std::cout << "ima2.pair_selection_count_max=" << selection_max << '\n';
    std::cout << "ima2.retained_legacy_context_min=" << retained_context_min
              << '\n';
    std::cout << "ima2.retained_legacy_context_max=" << retained_context_max
              << '\n';
    std::cout << "ima2.context_replacement_total=" << replacement_total << '\n';
    std::cout << "ima2.legacy_rollback_exact=" << rollback_exact << '\n';
    std::cout << "ima2.infeasible_sample_fails_closed=" << fail_closed << '\n';
    std::cout << "ima2.pass=" << pass << '\n';
    std::cout << "ima2.training_authorized=" << pass << '\n';
    std::cout << "ima2.decision="
              << (pass ? "mask_contract_feasible"
                       : "mask_contract_not_verified")
              << '\n';
    check(pass, "IMA-2 support-separated mask contract failed");
    std::cout << "[PASS] IMA-2 feasible support-separated mask contract\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] IMA-2 mask contract: " << error.what() << '\n';
    return 1;
  }
}
