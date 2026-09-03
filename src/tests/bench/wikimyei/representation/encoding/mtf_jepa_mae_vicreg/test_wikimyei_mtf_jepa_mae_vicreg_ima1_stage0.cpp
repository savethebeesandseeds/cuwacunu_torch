#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <torch/torch.h>

namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;

namespace {

constexpr int64_t kChannels = 3;
constexpr int64_t kHistory = 30;
constexpr int64_t kFeatures = 9;
constexpr int64_t kLatentDim = 32;
constexpr int64_t kBatchSize = 96;
constexpr int64_t kAuditSteps = 512;
constexpr int64_t kExpectedTokens = 72;
constexpr int64_t kExpectedTimeTargets = 4;
constexpr int64_t kExpectedFrequencyTargets = 2;
constexpr int64_t kExpectedTargets =
    kExpectedTimeTargets + kExpectedFrequencyTargets;
constexpr int64_t kExpectedContexts = 54;
constexpr std::array<int64_t, 3> kSeeds{17, 31, 47};
constexpr std::string_view kAnchorSchema = "oca1.certified_anchor.v1";
constexpr std::string_view kAnchorCertificate =
    "representation_certified_fspa4_minimal_spectral_repair_v1";
constexpr std::string_view kFspa4ProtocolSha256 =
    "4cf4f81ffac1665f85bd233203ccf2f039617ec8d52b41a40258002b42999b00";
constexpr std::string_view kOca1ProtocolSha256 =
    "56bac408b28046e4e014ccd22aab675da9d00b0feb28ed2d25d1debacd57ead2";
constexpr std::string_view kAnchorConfigFnv1a64 = "5482fc2dc41368c0";
constexpr std::string_view kStructuredReadout = "structured_cdsb_sparse_v1";

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
  config.latent_dim = kLatentDim;
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
  config.serving_pool_policy = mtf::mtf_serving_pool_policy_t::all_tokens;
  config.mask_ratio_time = 0.10;
  config.mask_ratio_frequency = 0.05;
  config.mask_ratio_channel = 0.0;
  config.min_context_ratio = 0.75;
  config.lambda_jepa = 1.0;
  config.lambda_mae = 0.25;
  config.lambda_tf_align = 0.10;
  config.lambda_vicreg = 0.05;
  config.lambda_global_vicreg = 0.25;
  config.lambda_channel_vicreg = 1.0;
  config.vicreg_sim_weight = 25.0;
  config.vicreg_var_weight = 25.0;
  config.vicreg_cov_weight = 1.0;
  config.vicreg_variance_floor = 1.0;
  config.vicreg_variance_epsilon = 0.0001;
  config.vicreg_view_gaussian_jitter_std = 0.005;
  config.vicreg_view_time_dropout_scale = 0.10;
  config.target_ema_tau = 0.990;
  config.use_target_ema = true;
  config.stop_gradient_target = true;
  config.return_diagnostics = true;
  config.return_vicreg_debug_tensors = true;
  config.use_mae_decoder = true;
  config.use_jepa_loss = true;
  config.use_tf_align_loss = true;
  config.use_vicreg_loss = true;
  config.use_global_vicreg = true;
  config.use_channel_vicreg = false;
  config.use_raw_reconstruction_targets = true;
  config.strict_finite_loss = true;
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

[[nodiscard]] torch::Tensor deterministic_input(int64_t batch_size) {
  const int64_t elements = batch_size * kChannels * kHistory * kFeatures;
  auto index =
      torch::arange(
          elements,
          torch::TensorOptions().dtype(torch::kFloat32).device(torch::kCPU))
          .reshape({batch_size, kChannels, kHistory, kFeatures});
  return torch::sin(index * 0.017) + 0.35 * torch::cos(index * 0.031) +
         0.001 * index;
}

struct Metadata {
  std::vector<int64_t> start{};
  std::vector<int64_t> width{};
  std::vector<int64_t> scale{};
  std::vector<int64_t> channel{};
  std::vector<int64_t> domain{};

  [[nodiscard]] int64_t size() const {
    return static_cast<int64_t>(start.size());
  }
};

[[nodiscard]] std::vector<int64_t>
tensor_to_vector(const torch::Tensor &input) {
  const auto cpu = input.to(torch::kCPU, torch::kInt64).contiguous();
  const auto *data = cpu.data_ptr<int64_t>();
  return std::vector<int64_t>(data, data + cpu.numel());
}

[[nodiscard]] Metadata metadata_from(const mtf::mtf_token_batch_t &batch) {
  Metadata result{};
  result.start = tensor_to_vector(batch.metadata.start_index);
  result.width = tensor_to_vector(batch.metadata.width);
  result.scale = tensor_to_vector(batch.metadata.scale_id);
  result.channel = tensor_to_vector(batch.metadata.channel_id);
  result.domain = tensor_to_vector(batch.metadata.domain_id);
  check(result.start.size() == result.width.size() &&
            result.start.size() == result.scale.size() &&
            result.start.size() == result.channel.size() &&
            result.start.size() == result.domain.size(),
        "IMA-1 metadata vectors have inconsistent sizes");
  return result;
}

[[nodiscard]] std::vector<int64_t> domain_indices(const Metadata &metadata,
                                                  int64_t domain) {
  std::vector<int64_t> result;
  for (int64_t token = 0; token < metadata.size(); ++token) {
    if (metadata.domain[static_cast<std::size_t>(token)] == domain) {
      result.push_back(token);
    }
  }
  return result;
}

[[nodiscard]] bool raw_intervals_overlap(const Metadata &metadata, int64_t left,
                                         int64_t right) {
  const int64_t left_begin = metadata.start[static_cast<std::size_t>(left)];
  const int64_t left_end =
      left_begin + metadata.width[static_cast<std::size_t>(left)];
  const int64_t right_begin = metadata.start[static_cast<std::size_t>(right)];
  const int64_t right_end =
      right_begin + metadata.width[static_cast<std::size_t>(right)];
  return std::max(left_begin, right_begin) < std::min(left_end, right_end);
}

[[nodiscard]] double raw_overlap_ratio(const Metadata &metadata, int64_t left,
                                       int64_t right) {
  const int64_t left_begin = metadata.start[static_cast<std::size_t>(left)];
  const int64_t left_end =
      left_begin + metadata.width[static_cast<std::size_t>(left)];
  const int64_t right_begin = metadata.start[static_cast<std::size_t>(right)];
  const int64_t right_end =
      right_begin + metadata.width[static_cast<std::size_t>(right)];
  const int64_t overlap = std::max<int64_t>(
      0, std::min(left_end, right_end) - std::max(left_begin, right_begin));
  return static_cast<double>(overlap) /
         static_cast<double>(std::max<int64_t>(
             1, std::min(metadata.width[static_cast<std::size_t>(left)],
                         metadata.width[static_cast<std::size_t>(right)])));
}

[[nodiscard]] bool is_exact_cross_domain_alias(const Metadata &metadata,
                                               int64_t left, int64_t right) {
  return metadata.channel[static_cast<std::size_t>(left)] ==
             metadata.channel[static_cast<std::size_t>(right)] &&
         metadata.domain[static_cast<std::size_t>(left)] !=
             metadata.domain[static_cast<std::size_t>(right)] &&
         metadata.start[static_cast<std::size_t>(left)] ==
             metadata.start[static_cast<std::size_t>(right)] &&
         metadata.width[static_cast<std::size_t>(left)] ==
             metadata.width[static_cast<std::size_t>(right)];
}

[[nodiscard]] int64_t raw_cross_domain_alias_pairs(const Metadata &metadata) {
  int64_t result = 0;
  for (int64_t left = 0; left < metadata.size(); ++left) {
    for (int64_t right = left + 1; right < metadata.size(); ++right) {
      result += is_exact_cross_domain_alias(metadata, left, right) ? 1 : 0;
    }
  }
  return result;
}

[[nodiscard]] std::vector<int64_t> selected_indices(const torch::Tensor &mask) {
  check(mask.dim() == 2 && mask.size(0) == 1,
        "IMA-1 selected-index mask must be [1,N]");
  const auto cpu = mask.to(torch::kCPU, torch::kBool).contiguous();
  const auto values = cpu.accessor<bool, 2>();
  std::vector<int64_t> result;
  for (int64_t token = 0; token < cpu.size(1); ++token) {
    if (values[0][token]) {
      result.push_back(token);
    }
  }
  return result;
}

[[nodiscard]] bool contains(const std::vector<int64_t> &indices,
                            int64_t value) {
  return std::find(indices.begin(), indices.end(), value) != indices.end();
}

[[nodiscard]] int64_t
strict_context_capacity(const Metadata &metadata,
                        const std::vector<int64_t> &targets,
                        bool same_channel_only) {
  int64_t result = 0;
  for (int64_t candidate = 0; candidate < metadata.size(); ++candidate) {
    if (contains(targets, candidate)) {
      continue;
    }
    bool overlaps_target = false;
    for (const int64_t target : targets) {
      if (same_channel_only &&
          metadata.channel[static_cast<std::size_t>(candidate)] !=
              metadata.channel[static_cast<std::size_t>(target)]) {
        continue;
      }
      if (raw_intervals_overlap(metadata, candidate, target)) {
        overlaps_target = true;
        break;
      }
    }
    if (!overlaps_target) {
      ++result;
    }
  }
  return result;
}

[[nodiscard]] bool raw_union_connected(const Metadata &metadata,
                                       const std::vector<int64_t> &targets) {
  std::vector<std::pair<int64_t, int64_t>> intervals;
  intervals.reserve(targets.size());
  for (const int64_t target : targets) {
    const int64_t begin = metadata.start[static_cast<std::size_t>(target)];
    intervals.push_back(
        {begin, begin + metadata.width[static_cast<std::size_t>(target)]});
  }
  std::sort(intervals.begin(), intervals.end());
  int64_t connected_end = intervals.front().second;
  for (std::size_t index = 1; index < intervals.size(); ++index) {
    if (intervals[index].first > connected_end) {
      return false;
    }
    connected_end = std::max(connected_end, intervals[index].second);
  }
  return true;
}

struct ExhaustiveJepaResult {
  int64_t current_target_sets{0};
  int64_t current_capacity_min{kExpectedTokens};
  int64_t current_capacity_max{0};
  int64_t current_capacity_ge_54{0};
  int64_t conservative_capacity_max{0};
  int64_t same_channel_same_scale_blocks{0};
  int64_t cross_scale_blocks{0};
  int64_t cross_channel_blocks{0};
  int64_t coherent_t1_sets{0};
  int64_t coherent_t1_time_only_capacity_upper_bound{0};
};

[[nodiscard]] ExhaustiveJepaResult
exhaustive_jepa_feasibility(const Metadata &metadata) {
  const auto time = domain_indices(metadata, 0);
  const auto frequency = domain_indices(metadata, 1);
  check(time.size() == 36 && frequency.size() == 36,
        "IMA-1 active tokenizer did not expose 36 tokens per domain");

  ExhaustiveJepaResult result{};
  for (std::size_t block_start = 0;
       block_start + kExpectedTimeTargets <= time.size(); ++block_start) {
    std::vector<int64_t> time_targets(
        time.begin() + static_cast<std::ptrdiff_t>(block_start),
        time.begin() +
            static_cast<std::ptrdiff_t>(block_start + kExpectedTimeTargets));
    const bool same_channel =
        std::all_of(time_targets.begin(), time_targets.end(), [&](int64_t x) {
          return metadata.channel[static_cast<std::size_t>(x)] ==
                 metadata
                     .channel[static_cast<std::size_t>(time_targets.front())];
        });
    const bool same_scale =
        std::all_of(time_targets.begin(), time_targets.end(), [&](int64_t x) {
          return metadata.scale[static_cast<std::size_t>(x)] ==
                 metadata.scale[static_cast<std::size_t>(time_targets.front())];
        });
    if (same_channel && same_scale) {
      ++result.same_channel_same_scale_blocks;
    } else if (same_channel) {
      ++result.cross_scale_blocks;
    } else {
      ++result.cross_channel_blocks;
    }

    for (std::size_t first = 0; first < frequency.size(); ++first) {
      for (std::size_t second = first + 1; second < frequency.size();
           ++second) {
        auto targets = time_targets;
        targets.push_back(frequency[first]);
        targets.push_back(frequency[second]);
        const int64_t same_channel_capacity =
            strict_context_capacity(metadata, targets,
                                    /*same_channel_only=*/true);
        const int64_t conservative_capacity =
            strict_context_capacity(metadata, targets,
                                    /*same_channel_only=*/false);
        result.current_capacity_min =
            std::min(result.current_capacity_min, same_channel_capacity);
        result.current_capacity_max =
            std::max(result.current_capacity_max, same_channel_capacity);
        result.conservative_capacity_max =
            std::max(result.conservative_capacity_max, conservative_capacity);
        result.current_capacity_ge_54 +=
            same_channel_capacity >= kExpectedContexts ? 1 : 0;
        ++result.current_target_sets;
      }
    }
  }

  for (std::size_t a = 0; a + 3 < time.size(); ++a) {
    for (std::size_t b = a + 1; b + 2 < time.size(); ++b) {
      for (std::size_t c = b + 1; c + 1 < time.size(); ++c) {
        for (std::size_t d = c + 1; d < time.size(); ++d) {
          const std::vector<int64_t> targets{time[a], time[b], time[c],
                                             time[d]};
          std::array<int64_t, kChannels> channel_counts{};
          std::array<int64_t, 4> scale_counts{};
          for (const int64_t target : targets) {
            ++channel_counts[static_cast<std::size_t>(
                metadata.channel[static_cast<std::size_t>(target)])];
            ++scale_counts[static_cast<std::size_t>(
                metadata.scale[static_cast<std::size_t>(target)])];
          }
          const auto [min_channel, max_channel] =
              std::minmax_element(channel_counts.begin(), channel_counts.end());
          if (*max_channel - *min_channel > 1 ||
              *std::max_element(scale_counts.begin(), scale_counts.end()) > 2 ||
              !raw_union_connected(metadata, targets)) {
            continue;
          }
          ++result.coherent_t1_sets;
          result.coherent_t1_time_only_capacity_upper_bound =
              std::max(result.coherent_t1_time_only_capacity_upper_bound,
                       strict_context_capacity(metadata, targets,
                                               /*same_channel_only=*/true));
        }
      }
    }
  }
  return result;
}

struct ScheduledJepaResult {
  int64_t samples{0};
  int64_t mechanics_failures{0};
  int64_t coherent_blocks{0};
  int64_t cross_scale_blocks{0};
  int64_t cross_channel_blocks{0};
  int64_t samples_with_relaxation{0};
  int64_t relaxed_soft_forbids{0};
  int64_t samples_with_same_channel_overlap{0};
  int64_t samples_with_exact_alias{0};
  int64_t same_channel_capacity_min{kExpectedTokens};
  int64_t same_channel_capacity_max{0};
  double maximum_retained_same_channel_overlap{0.0};
  std::vector<int64_t> selection_counts{};
};

[[nodiscard]] ScheduledJepaResult
audit_scheduled_masks(const mtf::mtf_token_batch_t &tokens,
                      const Metadata &metadata,
                      const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  ScheduledJepaResult result{};
  result.selection_counts.assign(static_cast<std::size_t>(metadata.size()), 0);
  mtf::JEPAContextTargetMasker masker(config);
  for (const int64_t seed : kSeeds) {
    for (int64_t step = 0; step < kAuditSteps; ++step) {
      torch::manual_seed(paired_step_seed(seed, step));
      const auto masks = masker.create_masks(tokens);
      const auto targets = selected_indices(masks.target_mask);
      const auto contexts = selected_indices(masks.context_mask);
      const int64_t time_targets = static_cast<int64_t>(
          std::count_if(targets.begin(), targets.end(), [&](int64_t target) {
            return metadata.domain[static_cast<std::size_t>(target)] == 0;
          }));
      const int64_t frequency_targets =
          static_cast<int64_t>(targets.size()) - time_targets;
      const bool subsets =
          masks.target_mask.logical_and(masks.valid_mask.logical_not())
                  .any()
                  .item<bool>() == false &&
          masks.context_mask.logical_and(masks.valid_mask.logical_not())
                  .any()
                  .item<bool>() == false;
      const bool disjoint =
          !masks.target_mask.logical_and(masks.context_mask).any().item<bool>();
      if (static_cast<int64_t>(targets.size()) != kExpectedTargets ||
          static_cast<int64_t>(contexts.size()) != kExpectedContexts ||
          time_targets != kExpectedTimeTargets ||
          frequency_targets != kExpectedFrequencyTargets || !subsets ||
          !disjoint) {
        ++result.mechanics_failures;
      }

      std::vector<int64_t> selected_time;
      for (const int64_t target : targets) {
        ++result.selection_counts[static_cast<std::size_t>(target)];
        if (metadata.domain[static_cast<std::size_t>(target)] == 0) {
          selected_time.push_back(target);
        }
      }
      const bool same_channel = std::all_of(
          selected_time.begin(), selected_time.end(), [&](int64_t x) {
            return metadata.channel[static_cast<std::size_t>(x)] ==
                   metadata.channel[static_cast<std::size_t>(
                       selected_time.front())];
          });
      const bool same_scale = std::all_of(
          selected_time.begin(), selected_time.end(), [&](int64_t x) {
            return metadata.scale[static_cast<std::size_t>(x)] ==
                   metadata
                       .scale[static_cast<std::size_t>(selected_time.front())];
          });
      if (same_channel && same_scale) {
        ++result.coherent_blocks;
      } else if (same_channel) {
        ++result.cross_scale_blocks;
      } else {
        ++result.cross_channel_blocks;
      }

      const int64_t capacity =
          strict_context_capacity(metadata, targets,
                                  /*same_channel_only=*/true);
      result.same_channel_capacity_min =
          std::min(result.same_channel_capacity_min, capacity);
      result.same_channel_capacity_max =
          std::max(result.same_channel_capacity_max, capacity);
      if (masks.relaxed_soft_forbidden_count > 0) {
        ++result.samples_with_relaxation;
        result.relaxed_soft_forbids += masks.relaxed_soft_forbidden_count;
      }

      bool has_same_channel_overlap = false;
      bool has_exact_alias = false;
      for (const int64_t context : contexts) {
        for (const int64_t target : targets) {
          if (metadata.channel[static_cast<std::size_t>(context)] ==
                  metadata.channel[static_cast<std::size_t>(target)] &&
              raw_intervals_overlap(metadata, context, target)) {
            has_same_channel_overlap = true;
            result.maximum_retained_same_channel_overlap =
                std::max(result.maximum_retained_same_channel_overlap,
                         raw_overlap_ratio(metadata, context, target));
          }
          if (is_exact_cross_domain_alias(metadata, context, target)) {
            has_exact_alias = true;
          }
        }
      }
      result.samples_with_same_channel_overlap +=
          has_same_channel_overlap ? 1 : 0;
      result.samples_with_exact_alias += has_exact_alias ? 1 : 0;
      ++result.samples;
    }
  }
  return result;
}

[[nodiscard]] std::filesystem::path anchor_path(int64_t seed) {
  return std::filesystem::path(".build") / "tests" / "oca1" /
         ("certified_fspa4_seed_" + std::to_string(seed) + ".pt");
}

[[nodiscard]] std::string tensor_string(const torch::Tensor &value) {
  const auto bytes = value.detach().to(torch::kCPU, torch::kUInt8).contiguous();
  check(bytes.dim() == 1 && bytes.numel() > 0,
        "IMA-1 anchor string tensor contract failed");
  return {reinterpret_cast<const char *>(bytes.data_ptr<uint8_t>()),
          static_cast<std::size_t>(bytes.numel())};
}

[[nodiscard]] bool load_certified_anchor(mtf::MtfJepaMaeVicreg &model,
                                         int64_t seed) {
  const auto path = anchor_path(seed);
  check(std::filesystem::is_regular_file(path),
        "IMA-1 frozen FSPA-4 anchor is missing: " + path.string());

  torch::serialize::InputArchive root;
  root.load_from(path.string(), torch::Device(torch::kCPU));
  torch::Tensor schema{};
  torch::Tensor saved_seed{};
  torch::Tensor certificate{};
  torch::Tensor fspa_hash{};
  torch::Tensor oca_hash{};
  torch::Tensor config_hash{};
  torch::Tensor readout{};
  root.read("meta/schema", schema);
  root.read("meta/seed", saved_seed);
  root.read("meta/certificate_id", certificate);
  root.read("meta/fspa4_protocol_sha256", fspa_hash);
  root.read("meta/oca1_protocol_sha256", oca_hash);
  root.read("meta/config_fnv1a64", config_hash);
  root.read("meta/readout_policy", readout);
  const bool metadata_exact =
      tensor_string(schema) == kAnchorSchema && saved_seed.numel() == 1 &&
      saved_seed.item<int64_t>() == seed &&
      tensor_string(certificate) == kAnchorCertificate &&
      tensor_string(fspa_hash) == kFspa4ProtocolSha256 &&
      tensor_string(oca_hash) == kOca1ProtocolSha256 &&
      tensor_string(config_hash) == kAnchorConfigFnv1a64 &&
      tensor_string(readout) == kStructuredReadout;

  torch::serialize::InputArchive model_archive;
  root.read("model", model_archive);
  model->load(model_archive);
  return metadata_exact;
}

struct ParameterSnapshot {
  std::vector<std::string> names{};
  std::vector<torch::Tensor> values{};
};

[[nodiscard]] ParameterSnapshot
snapshot_parameters(const mtf::MtfJepaMaeVicreg &model) {
  ParameterSnapshot result{};
  torch::NoGradGuard no_grad;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    result.names.push_back(item.key());
    result.values.push_back(item.value().detach().clone());
  }
  return result;
}

[[nodiscard]] double
parameter_max_abs_diff(const mtf::MtfJepaMaeVicreg &model,
                       const ParameterSnapshot &reference) {
  const auto parameters = model->named_parameters(/*recurse=*/true);
  check(parameters.size() == reference.values.size(),
        "IMA-1 parameter snapshot size changed");
  double maximum = 0.0;
  std::size_t index = 0;
  torch::NoGradGuard no_grad;
  for (const auto &item : parameters) {
    check(item.key() == reference.names[index] &&
              item.value().sizes() == reference.values[index].sizes(),
          "IMA-1 parameter snapshot layout changed");
    maximum =
        std::max(maximum, (item.value().detach() - reference.values[index])
                              .abs()
                              .max()
                              .item<double>());
    ++index;
  }
  return maximum;
}

[[nodiscard]] double gradient_norm(const mtf::MtfJepaMaeVicreg &model,
                                   const std::vector<std::string> &prefixes) {
  double square_sum = 0.0;
  std::size_t included = 0;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    const bool include =
        std::any_of(prefixes.begin(), prefixes.end(), [&](const auto &prefix) {
          return item.key().rfind(prefix, 0) == 0;
        });
    if (!include || !item.value().requires_grad()) {
      continue;
    }
    ++included;
    if (item.value().grad().defined()) {
      square_sum += item.value().grad().detach().pow(2).sum().item<double>();
    }
  }
  check(included > 0, "IMA-1 gradient partition is empty");
  check(std::isfinite(square_sum), "IMA-1 gradient norm is non-finite");
  return std::sqrt(square_sum);
}

struct GeneratedViews {
  mtf::mtf_input_t a{};
  mtf::mtf_input_t b{};
  torch::Tensor post_rng_state{};
};

[[nodiscard]] GeneratedViews
generate_default_views(const torch::Tensor &data, const torch::Tensor &mask,
                       const mtf::mtf_jepa_mae_vicreg_config_t &config,
                       int64_t seed) {
  torch::manual_seed(seed);
  GeneratedViews result{};
  result.a =
      mtf::detail::apply_vicreg_weak_view_augmentation(data, mask, config);
  result.b =
      mtf::detail::apply_vicreg_weak_view_augmentation(data, mask, config);
  result.post_rng_state =
      at::detail::getDefaultCPUGenerator().get_state().clone();
  return result;
}

struct ComponentGradient {
  double tokenizer{0.0};
  double encoder{0.0};
  double projector{0.0};
};

struct VicregTreatmentResult {
  std::string name{};
  bool branch_inputs_exact{false};
  bool sample_valid_exact_and_full{false};
  bool channel_valid_exact_and_full{false};
  double invariance_loss{0.0};
  double variance_loss{0.0};
  double covariance_loss{0.0};
  std::array<ComponentGradient, 3> gradients{};
};

[[nodiscard]] VicregTreatmentResult
evaluate_vicreg_treatment(mtf::MtfJepaMaeVicreg &model, const std::string &name,
                          const mtf::mtf_input_t &view_a,
                          const mtf::mtf_input_t &view_b,
                          const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  auto encoded_a = model->encode(view_a.data, view_a.feature_mask);
  auto encoded_b = model->encode(view_b.data, view_b.feature_mask);
  auto projected_a =
      model->project_vicreg(encoded_a.pooled_embedding).unsqueeze(1);
  auto projected_b =
      model->project_vicreg(encoded_b.pooled_embedding).unsqueeze(1);
  auto sample_mask_a = encoded_a.sample_valid_mask.unsqueeze(1);
  auto sample_mask_b = encoded_b.sample_valid_mask.unsqueeze(1);

  mtf::vicreg_stability_loss_options_t options{};
  options.invariance_weight = config.vicreg_sim_weight;
  options.variance_weight = config.vicreg_var_weight;
  options.covariance_weight = config.vicreg_cov_weight;
  options.variance_floor = config.vicreg_variance_floor;
  options.eps = config.vicreg_variance_epsilon;
  const auto loss = mtf::compute_vicreg_stability_loss(
      projected_a, sample_mask_a, projected_b, sample_mask_b, options);

  VicregTreatmentResult result{};
  result.name = name;
  result.branch_inputs_exact =
      torch::equal(view_a.data, view_b.data) &&
      torch::equal(view_a.feature_mask, view_b.feature_mask);
  result.sample_valid_exact_and_full =
      torch::equal(encoded_a.sample_valid_mask, encoded_b.sample_valid_mask) &&
      encoded_a.sample_valid_mask.all().item<bool>() &&
      encoded_b.sample_valid_mask.all().item<bool>();
  result.channel_valid_exact_and_full =
      torch::equal(encoded_a.channel_valid_mask,
                   encoded_b.channel_valid_mask) &&
      encoded_a.channel_valid_mask.all().item<bool>() &&
      encoded_b.channel_valid_mask.all().item<bool>();
  result.invariance_loss = loss.invariance_loss.item<double>();
  result.variance_loss = loss.variance_loss.item<double>();
  result.covariance_loss = loss.covariance_loss.item<double>();

  const double outer_scale = config.lambda_vicreg * config.lambda_global_vicreg;
  const std::array<torch::Tensor, 3> weighted_components{
      outer_scale * config.vicreg_sim_weight * loss.invariance_loss,
      outer_scale * config.vicreg_var_weight * loss.variance_loss,
      outer_scale * config.vicreg_cov_weight * loss.covariance_loss};
  for (std::size_t component = 0; component < weighted_components.size();
       ++component) {
    model->zero_grad();
    weighted_components[component].backward(
        /*gradient=*/{}, /*retain_graph=*/true);
    result.gradients[component].tokenizer =
        gradient_norm(model, {"tokenizer."});
    result.gradients[component].encoder = gradient_norm(model, {"encoder."});
    result.gradients[component].projector =
        gradient_norm(model, {"vicreg_stability_head."});
  }
  model->zero_grad();
  return result;
}

void emit_vicreg_treatment(const VicregTreatmentResult &result) {
  const std::string prefix = "ima1.stage0.vicreg." + result.name;
  std::cout << prefix << ".branch_inputs_exact=" << result.branch_inputs_exact
            << '\n';
  std::cout << prefix << ".sample_valid_exact_and_full="
            << result.sample_valid_exact_and_full << '\n';
  std::cout << prefix << ".channel_valid_exact_and_full="
            << result.channel_valid_exact_and_full << '\n';
  std::cout << prefix << ".invariance_loss=" << result.invariance_loss << '\n';
  std::cout << prefix << ".variance_loss=" << result.variance_loss << '\n';
  std::cout << prefix << ".covariance_loss=" << result.covariance_loss << '\n';
  constexpr std::array<const char *, 3> component_names{
      "invariance", "variance", "covariance"};
  for (std::size_t component = 0; component < component_names.size();
       ++component) {
    const auto &gradient = result.gradients[component];
    const std::string component_prefix =
        prefix + "." + component_names[component];
    std::cout << component_prefix
              << ".tokenizer_gradient_norm=" << gradient.tokenizer << '\n';
    std::cout << component_prefix
              << ".encoder_gradient_norm=" << gradient.encoder << '\n';
    std::cout << component_prefix
              << ".projector_gradient_norm=" << gradient.projector << '\n';
  }
}

struct VicregAuditResult {
  int64_t seed{0};
  bool pass{false};
  bool anchor_metadata_exact{false};
  bool rng_post_state_exact{false};
  bool clean_identity_exact{false};
  bool parameters_unchanged{false};
  double parameter_max_abs_delta{0.0};
  VicregTreatmentResult current{};
  VicregTreatmentResult tied{};
  VicregTreatmentResult clean{};
};

[[nodiscard]] VicregAuditResult
audit_vicreg(const torch::Tensor &data, const torch::Tensor &mask,
             const mtf::mtf_jepa_mae_vicreg_config_t &config, int64_t seed) {
  torch::manual_seed(seed);
  auto model = mtf::MtfJepaMaeVicreg(config);
  model->eval();
  VicregAuditResult result{};
  result.seed = seed;
  result.anchor_metadata_exact = load_certified_anchor(model, seed);
  const auto parameters_before = snapshot_parameters(model);
  const auto clean = mtf::detail::canonicalize_input(data, mask, config);
  const int64_t view_seed = paired_step_seed(seed, 0);

  const auto current_generated =
      generate_default_views(data, mask, config, view_seed);
  const auto tied_generated =
      generate_default_views(data, mask, config, view_seed);
  const auto clean_generated =
      generate_default_views(data, mask, config, view_seed);

  result.rng_post_state_exact = torch::equal(current_generated.post_rng_state,
                                             tied_generated.post_rng_state) &&
                                torch::equal(current_generated.post_rng_state,
                                             clean_generated.post_rng_state);
  result.clean_identity_exact =
      torch::equal(clean.data, data) && torch::equal(clean.feature_mask, mask);
  result.current = evaluate_vicreg_treatment(
      model, "seed_" + std::to_string(seed) + ".v0_current",
      current_generated.a, current_generated.b, config);
  result.tied = evaluate_vicreg_treatment(
      model, "seed_" + std::to_string(seed) + ".v1_tied", tied_generated.a,
      tied_generated.a, config);
  result.clean = evaluate_vicreg_treatment(
      model, "seed_" + std::to_string(seed) + ".v2_clean", clean, clean,
      config);
  result.parameter_max_abs_delta =
      parameter_max_abs_diff(model, parameters_before);
  result.parameters_unchanged = result.parameter_max_abs_delta == 0.0;

  constexpr double kZeroTolerance = 1.0e-12;
  const auto identical_invariance_zero =
      [&](const VicregTreatmentResult &treatment) {
        return treatment.invariance_loss <= kZeroTolerance &&
               treatment.gradients[0].tokenizer <= kZeroTolerance &&
               treatment.gradients[0].encoder <= kZeroTolerance &&
               treatment.gradients[0].projector <= kZeroTolerance;
      };
  const bool validity_pass = result.current.sample_valid_exact_and_full &&
                             result.current.channel_valid_exact_and_full &&
                             result.tied.sample_valid_exact_and_full &&
                             result.tied.channel_valid_exact_and_full &&
                             result.clean.sample_valid_exact_and_full &&
                             result.clean.channel_valid_exact_and_full;
  result.pass = result.anchor_metadata_exact && result.rng_post_state_exact &&
                result.clean_identity_exact &&
                result.tied.branch_inputs_exact &&
                result.clean.branch_inputs_exact &&
                identical_invariance_zero(result.tied) &&
                identical_invariance_zero(result.clean) && validity_pass &&
                result.parameters_unchanged;
  return result;
}

} // namespace

int main() {
  try {
    at::set_num_threads(1);
    std::cout << std::boolalpha << std::setprecision(17);
    const auto config = active_config();
    const auto data = deterministic_input(kBatchSize);
    const auto mask = torch::ones(
        data.sizes(),
        torch::TensorOptions().dtype(torch::kBool).device(torch::kCPU));

    auto tokenizer = mtf::TimeFrequencyViewBuilder(config);
    tokenizer->eval();
    mtf::mtf_token_batch_t one_sample_tokens{};
    {
      torch::NoGradGuard no_grad;
      one_sample_tokens =
          tokenizer->forward(data.narrow(0, 0, 1), mask.narrow(0, 0, 1));
    }
    check(one_sample_tokens.tokens.size(1) == kExpectedTokens,
          "IMA-1 Stage 0 expected exactly 72 active tokens");
    const auto metadata = metadata_from(one_sample_tokens);
    const int64_t raw_alias_pairs = raw_cross_domain_alias_pairs(metadata);
    const auto exhaustive = exhaustive_jepa_feasibility(metadata);
    const auto scheduled =
        audit_scheduled_masks(one_sample_tokens, metadata, config);

    const bool current_target_space_exhaustive =
        exhaustive.current_target_sets == 33 * 630;
    const bool current_context_strictly_feasible =
        exhaustive.current_capacity_ge_54 == exhaustive.current_target_sets;
    const bool coherent_t1_exists = exhaustive.coherent_t1_sets > 0;
    const bool coherent_t1_context_upper_bound_feasible =
        exhaustive.coherent_t1_time_only_capacity_upper_bound >=
        kExpectedContexts;
    const bool jepa_mechanics_pass =
        scheduled.mechanics_failures == 0 &&
        scheduled.samples == static_cast<int64_t>(kSeeds.size()) * kAuditSteps;
    const bool jepa_stage0_pass =
        current_target_space_exhaustive && jepa_mechanics_pass &&
        current_context_strictly_feasible && coherent_t1_exists &&
        coherent_t1_context_upper_bound_feasible;

    std::cout << "ima1.stage0.optimizer_steps=0\n";
    std::cout << "ima1.stage0.ema_updates=0\n";
    std::cout << "ima1.stage0.jepa.total_tokens=" << metadata.size() << '\n';
    std::cout << "ima1.stage0.jepa.raw_cross_domain_alias_pairs="
              << raw_alias_pairs << '\n';
    std::cout << "ima1.stage0.jepa.exhaustive_current_target_sets="
              << exhaustive.current_target_sets << '\n';
    std::cout << "ima1.stage0.jepa.current_target_space_exhaustive="
              << current_target_space_exhaustive << '\n';
    std::cout << "ima1.stage0.jepa.same_channel_same_scale_blocks="
              << exhaustive.same_channel_same_scale_blocks << '\n';
    std::cout << "ima1.stage0.jepa.cross_scale_blocks="
              << exhaustive.cross_scale_blocks << '\n';
    std::cout << "ima1.stage0.jepa.cross_channel_blocks="
              << exhaustive.cross_channel_blocks << '\n';
    std::cout << "ima1.stage0.jepa.current_strict_context_capacity_min="
              << exhaustive.current_capacity_min << '\n';
    std::cout << "ima1.stage0.jepa.current_strict_context_capacity_max="
              << exhaustive.current_capacity_max << '\n';
    std::cout << "ima1.stage0.jepa.current_strict_capacity_ge_54="
              << exhaustive.current_capacity_ge_54 << '\n';
    std::cout << "ima1.stage0.jepa.conservative_capacity_max="
              << exhaustive.conservative_capacity_max << '\n';
    std::cout << "ima1.stage0.jepa.coherent_t1_sets="
              << exhaustive.coherent_t1_sets << '\n';
    std::cout << "ima1.stage0.jepa.coherent_t1_time_only_capacity_upper_bound="
              << exhaustive.coherent_t1_time_only_capacity_upper_bound << '\n';
    std::cout << "ima1.stage0.jepa.scheduled_samples=" << scheduled.samples
              << '\n';
    std::cout << "ima1.stage0.jepa.scheduled_mechanics_failures="
              << scheduled.mechanics_failures << '\n';
    std::cout << "ima1.stage0.jepa.scheduled_coherent_blocks="
              << scheduled.coherent_blocks << '\n';
    std::cout << "ima1.stage0.jepa.scheduled_cross_scale_blocks="
              << scheduled.cross_scale_blocks << '\n';
    std::cout << "ima1.stage0.jepa.scheduled_cross_channel_blocks="
              << scheduled.cross_channel_blocks << '\n';
    std::cout << "ima1.stage0.jepa.samples_with_soft_relaxation="
              << scheduled.samples_with_relaxation << '\n';
    std::cout << "ima1.stage0.jepa.relaxed_soft_forbid_count="
              << scheduled.relaxed_soft_forbids << '\n';
    std::cout << "ima1.stage0.jepa.samples_with_same_channel_overlap="
              << scheduled.samples_with_same_channel_overlap << '\n';
    std::cout << "ima1.stage0.jepa.samples_with_exact_cross_domain_alias="
              << scheduled.samples_with_exact_alias << '\n';
    std::cout << "ima1.stage0.jepa.maximum_retained_same_channel_overlap="
              << scheduled.maximum_retained_same_channel_overlap << '\n';
    std::cout << "ima1.stage0.jepa.scheduled_strict_context_capacity_min="
              << scheduled.same_channel_capacity_min << '\n';
    std::cout << "ima1.stage0.jepa.scheduled_strict_context_capacity_max="
              << scheduled.same_channel_capacity_max << '\n';
    std::cout << "ima1.stage0.jepa.mechanics_pass=" << jepa_mechanics_pass
              << '\n';
    std::cout << "ima1.stage0.jepa.current_context_strictly_feasible="
              << current_context_strictly_feasible << '\n';
    std::cout << "ima1.stage0.jepa.coherent_t1_exists=" << coherent_t1_exists
              << '\n';
    std::cout << "ima1.stage0.jepa.coherent_t1_context_upper_bound_feasible="
              << coherent_t1_context_upper_bound_feasible << '\n';
    std::cout << "ima1.stage0.jepa.pass=" << jepa_stage0_pass << '\n';

    bool vicreg_pass = true;
    bool anchor_metadata_exact = true;
    for (const int64_t seed : kSeeds) {
      const auto vicreg = audit_vicreg(data, mask, config, seed);
      emit_vicreg_treatment(vicreg.current);
      emit_vicreg_treatment(vicreg.tied);
      emit_vicreg_treatment(vicreg.clean);
      const std::string prefix =
          "ima1.stage0.vicreg.seed_" + std::to_string(seed);
      std::cout << prefix
                << ".anchor_metadata_exact=" << vicreg.anchor_metadata_exact
                << '\n';
      std::cout << prefix
                << ".rng_post_state_exact=" << vicreg.rng_post_state_exact
                << '\n';
      std::cout << prefix
                << ".clean_identity_exact=" << vicreg.clean_identity_exact
                << '\n';
      std::cout << prefix
                << ".parameter_max_abs_delta=" << vicreg.parameter_max_abs_delta
                << '\n';
      std::cout << prefix
                << ".parameters_unchanged=" << vicreg.parameters_unchanged
                << '\n';
      std::cout << prefix << ".pass=" << vicreg.pass << '\n';
      anchor_metadata_exact =
          anchor_metadata_exact && vicreg.anchor_metadata_exact;
      vicreg_pass = vicreg_pass && vicreg.pass;
    }
    std::cout << "ima1.stage0.vicreg.anchor_config_fnv1a64="
              << kAnchorConfigFnv1a64 << '\n';
    std::cout << "ima1.stage0.vicreg.all_anchor_metadata_exact="
              << anchor_metadata_exact << '\n';
    std::cout << "ima1.stage0.vicreg.pass=" << vicreg_pass << '\n';

    const bool overall_pass = jepa_stage0_pass && vicreg_pass;
    std::cout << "ima1.stage0.pass=" << overall_pass << '\n';
    std::cout << "ima1.stage0.training_authorized=" << overall_pass << '\n';
    std::cout << "ima1.stage0.decision="
              << (overall_pass ? "proceed_to_bounded_training"
                               : "current_tokenizer_mask_contract_infeasible")
              << '\n';
    std::cout << "execution_status=ima1_stage0_measurements_complete\n";
    std::cout << "[PASS] IMA-1 Stage 0 audit completed\n";
    return 0;
  } catch (const std::exception &error) {
    std::cerr << "[FAIL] IMA-1 Stage 0 audit: " << error.what() << '\n';
    return 1;
  }
}
