#include "jkimyei/training/representation/mtf_jepa_mae_vicreg_graph_first_launcher.h"
#include "pooling_structure_mechanism_map_gate.h"
#include "representation_jepa_mae_core_decomposition_gate.h"
#include "representation_objective_repair_gate.h"
#include "representation_outer_augmentation_training_gate.h"
#include "representation_surface_sufficiency_map_gate.h"
#include "representation_vicreg_variance_necessity_gate.h"
#include "wikimyei/representation/encoding/mtf_jepa_mae_vicreg/mtf_jepa_mae_vicreg.h"

#include <ATen/CPUGeneratorImpl.h>
#include <ATen/Context.h>
#include <ATen/cuda/CUDAGeneratorImpl.h>
#include <ATen/ops/cholesky_solve.h>
#include <ATen/ops/linalg_cholesky_ex.h>
#include <ATen/ops/linalg_eigvalsh.h>
#include <ATen/ops/linalg_qr.h>
#include <torch/cuda.h>
#include <torch/torch.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <locale>
#include <numeric>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace mtf =
    cuwacunu::wikimyei::representation::encoding::mtf_jepa_mae_vicreg;
namespace repair_gate = cuwacunu::tests::mtf_objective_repair_gate;
namespace jmcd_gate = cuwacunu::tests::mtf_jepa_mae_core_decomposition_gate;
namespace outer_gate = cuwacunu::tests::mtf_outer_augmentation_training_gate;
namespace psm_gate =
    cuwacunu::tests::pooling_structure_mechanism_map_gate;
namespace rssm_gate = cuwacunu::tests::mtf_surface_sufficiency_map_gate;
namespace variance_gate = cuwacunu::tests::mtf_vicreg_variance_necessity_gate;
namespace launcher_augmentation = cuwacunu::jkimyei::training::representation::
    mtf_jepa_mae_vicreg_graph_first_launcher_detail;

namespace {

constexpr int64_t kChannels = 3;
constexpr int64_t kHistory = 30;
constexpr int64_t kFeatures = 9;
constexpr int64_t kLatentDim = 32;
constexpr int64_t kServedWidth = kChannels * kLatentDim;
constexpr int64_t kRawChannelWidth = kHistory * kFeatures;
constexpr int64_t kTargets = 12;
constexpr int64_t kFamilies = 4;
constexpr int64_t kModelRowBatchSize = 96;
constexpr int64_t kActiveNodeCountAssumption = 3;
constexpr int64_t kActiveAnchorBatchEquivalent =
    kModelRowBatchSize / kActiveNodeCountAssumption;
static_assert(kActiveAnchorBatchEquivalent == 32);
constexpr double kPi = 3.141592653589793238462643383279502884;
constexpr std::array<int64_t, kTargets> kTargetFamily{
    0, 0, 0, 0, 0, // multiscale state
    1, 1,          // temporal order/regime
    2, 2,          // cross-channel dynamics
    3, 3, 3};      // multi-horizon future
constexpr std::array<const char *, kFamilies> kFamilyNames{
    "multiscale_state", "order_regime", "cross_channel", "future"};
constexpr std::array<double, 6> kRidgeGrid{1.0e-5, 1.0e-4, 1.0e-3,
                                           1.0e-2, 1.0e-1, 1.0};

struct Options {
  std::string experiment{"quality"};
  std::string tier{"fast"};
  std::string device{"cpu"};
  std::string qualifier_log{};
  int64_t seeds{-1};
  int64_t steps{-1};
  bool weak_views{true};
  bool verbose{false};
};

struct Tier {
  int64_t ssl_groups{0};
  int64_t probe_train_groups{0};
  int64_t probe_validation_groups{0};
  int64_t test_groups{0};
  int64_t steps{0};
  int64_t seeds{0};
  int64_t bootstrap_replicates{0};
  std::vector<int64_t> sample_ladder{};
};

struct Factors {
  double trend{0.0};
  double slow_amplitude{0.0};
  double mid_amplitude{0.0};
  double phase{0.0};
  int64_t change_index{0};
  double jump{0.0};
  int64_t lag{0};
  double coupling{0.0};
  double idiosyncratic_phase{0.0};
  double observation_noise{0.0};
};

struct Dataset {
  torch::Tensor data{};   // [S,C,H,F], float32
  torch::Tensor mask{};   // [S,C,H,F], bool
  torch::Tensor target{}; // [S,T], float64
  int64_t group_begin{0};
};

struct Normalization {
  torch::Tensor mean{};    // [1,C,1,F]
  torch::Tensor inv_std{}; // [1,C,1,F]
};

struct Embeddings {
  torch::Tensor by_channel{}; // [S,C,32], float64 CPU
  torch::Tensor flat{};       // [S,96], float64 CPU
};

struct VicregGeometrySurfaces {
  torch::Tensor global_preprojector{};  // [S,32], float64 CPU
  torch::Tensor projected_by_channel{}; // [S,C,64], float64 CPU
};

struct TrainingStats {
  int64_t completed_steps{0};
  double first_window_loss{std::numeric_limits<double>::quiet_NaN()};
  double last_window_loss{std::numeric_limits<double>::quiet_NaN()};
  bool finite{false};
  bool improved{false};
};

struct RidgeModel {
  torch::Tensor mean{};
  torch::Tensor inv_std{};
  torch::Tensor weights{};
  torch::Tensor bias{};
};

struct ScoreSummary {
  std::array<double, kTargets> task{};
  std::array<double, kFamilies> family{};
  double macro{0.0};
};

struct ProbePoint {
  int64_t samples{0};
  torch::Tensor prediction{}; // [test,T], float64
  ScoreSummary score{};
  std::array<double, kTargets> selected_alpha{};
};

struct ProbeCurve {
  std::vector<ProbePoint> points{};
  double area{0.0};
};

struct Geometry {
  double effective_rank_ratio{0.0};
  double participation_rank_ratio{0.0};
  double top_eigenvalue_share{1.0};
  double active_dimension_fraction{0.0};
  bool passed{false};
};

[[nodiscard]] bool geometry_exact(const Geometry &left, const Geometry &right) {
  return left.effective_rank_ratio == right.effective_rank_ratio &&
         left.participation_rank_ratio == right.participation_rank_ratio &&
         left.top_eigenvalue_share == right.top_eigenvalue_share &&
         left.active_dimension_fraction == right.active_dimension_fraction &&
         left.passed == right.passed;
}

[[nodiscard]] bool
geometry_exact(const std::array<Geometry, kChannels> &left,
               const std::array<Geometry, kChannels> &right) {
  return std::equal(
      left.begin(), left.end(), right.begin(),
      [](const Geometry &left_channel, const Geometry &right_channel) {
        return geometry_exact(left_channel, right_channel);
      });
}

struct PairSeparation {
  double nuisance_distance{0.0};
  double semantic_distance{0.0};
  double semantic_over_nuisance_mean_ratio{0.0};
  double semantic_over_nuisance_fraction{0.0};
};

struct Interval {
  double low{0.0};
  double high{0.0};
};

struct PairedBootstrapIntervals {
  Interval trained_minus_untrained_area{};
  Interval trained_minus_raw_control_area{};
};

[[nodiscard]] uint64_t splitmix64(uint64_t value) {
  value += 0x9e3779b97f4a7c15ULL;
  value = (value ^ (value >> 30U)) * 0xbf58476d1ce4e5b9ULL;
  value = (value ^ (value >> 27U)) * 0x94d049bb133111ebULL;
  return value ^ (value >> 31U);
}

[[nodiscard]] double uniform01(uint64_t key) {
  return static_cast<double>(splitmix64(key) >> 11U) *
         (1.0 / 9007199254740992.0);
}

[[nodiscard]] uint64_t key(int64_t group, int64_t field, int64_t index = 0,
                           int64_t view = 0) {
  uint64_t value = 0x6d74665f7175616cULL;
  value ^= splitmix64(static_cast<uint64_t>(group) + 0x100000001b3ULL);
  value ^= splitmix64(static_cast<uint64_t>(field) + 0x9e3779b9ULL);
  value ^= splitmix64(static_cast<uint64_t>(index) + 0x85ebca6bULL);
  value ^= splitmix64(static_cast<uint64_t>(view) + 0xc2b2ae35ULL);
  return splitmix64(value);
}

[[nodiscard]] double signed_uniform(uint64_t value) {
  return 2.0 * uniform01(value) - 1.0;
}

[[nodiscard]] Factors factors_for(int64_t group, bool semantic_counterfactual) {
  Factors out{};
  out.trend = 0.18 * signed_uniform(key(group, 0));
  out.slow_amplitude = 0.55 + 0.90 * uniform01(key(group, 1));
  out.mid_amplitude = 0.20 + 0.65 * uniform01(key(group, 2));
  out.phase = 2.0 * kPi * uniform01(key(group, 3));
  if (semantic_counterfactual) {
    out.phase += 0.5 * kPi;
  }
  out.change_index = 8 + static_cast<int64_t>(uniform01(key(group, 4)) * 14.0);
  const double jump_sign = uniform01(key(group, 5)) < 0.5 ? -1.0 : 1.0;
  out.jump = jump_sign * (0.30 + 0.85 * uniform01(key(group, 6)));
  out.lag = 1 + static_cast<int64_t>(uniform01(key(group, 7)) * 4.0);
  out.coupling = 0.85 * signed_uniform(key(group, 8));
  out.idiosyncratic_phase = 2.0 * kPi * uniform01(key(group, 9));
  out.observation_noise = 0.015 + 0.040 * uniform01(key(group, 10));
  return out;
}

[[nodiscard]] double base_signal(const Factors &factors, int64_t time) {
  const double centered =
      (static_cast<double>(time) - 0.5 * static_cast<double>(kHistory - 1)) /
      static_cast<double>(kHistory);
  const double slow =
      factors.slow_amplitude *
      std::sin(2.0 * kPi * static_cast<double>(time) / 24.0 + factors.phase);
  const double mid = factors.mid_amplitude *
                     std::sin(2.0 * kPi * static_cast<double>(time) / 9.0 +
                              0.55 * factors.phase);
  const double high =
      0.18 * std::cos(2.0 * kPi * static_cast<double>(time) / 4.5 +
                      1.3 * factors.phase);
  const double regime = time >= factors.change_index ? factors.jump : 0.0;
  return 2.0 * factors.trend * centered + slow + mid + high + regime;
}

[[nodiscard]] double idiosyncratic_signal(const Factors &factors,
                                          int64_t channel, int64_t time) {
  const double phase = factors.idiosyncratic_phase + 0.9 * channel;
  return 0.75 * std::sin(2.0 * kPi * static_cast<double>(time) /
                             (6.0 + 2.0 * channel) +
                         phase) +
         0.30 *
             std::cos(2.0 * kPi * static_cast<double>(time) / (13.0 + channel) +
                      0.7 * phase);
}

[[nodiscard]] double noiseless_value(const Factors &factors, int64_t channel,
                                     int64_t time) {
  if (channel == 0) {
    return base_signal(factors, time);
  }
  if (channel == 1) {
    const double residual =
        std::sqrt(std::max(0.0, 1.0 - factors.coupling * factors.coupling));
    return factors.coupling * base_signal(factors, time - factors.lag) +
           residual * idiosyncratic_signal(factors, channel, time);
  }
  return 0.55 * base_signal(factors, time - 2 * factors.lag) +
         0.45 * idiosyncratic_signal(factors, channel, time);
}

[[nodiscard]] double observed_value(const Factors &factors, int64_t group,
                                    int64_t channel, int64_t time,
                                    int64_t view) {
  const double multiplier = view == 1 ? 1.5 : 1.0;
  return noiseless_value(factors, channel, time) +
         multiplier * factors.observation_noise *
             signed_uniform(key(group, 20 + channel, time + 64, view));
}

[[nodiscard]] Dataset generate_dataset(int64_t group_begin, int64_t groups,
                                       int64_t view = 0,
                                       bool semantic_counterfactual = false) {
  auto data = torch::empty({groups, kChannels, kHistory, kFeatures},
                           torch::TensorOptions().dtype(torch::kFloat32));
  auto mask = torch::ones({groups, kChannels, kHistory, kFeatures},
                          torch::TensorOptions().dtype(torch::kBool));
  auto target = torch::empty({groups, kTargets}, torch::kFloat64);
  auto x = data.accessor<float, 4>();
  auto valid = mask.accessor<bool, 4>();
  auto y = target.accessor<double, 2>();

  for (int64_t row = 0; row < groups; ++row) {
    const int64_t group = group_begin + row;
    const auto factors = factors_for(group, semantic_counterfactual);
    y[row][0] = factors.trend;
    y[row][1] = factors.slow_amplitude;
    y[row][2] = factors.mid_amplitude;
    y[row][3] = std::sin(factors.phase);
    y[row][4] = std::cos(factors.phase);
    y[row][5] = static_cast<double>(factors.change_index) /
                static_cast<double>(kHistory - 1);
    y[row][6] = factors.jump;
    y[row][7] = static_cast<double>(factors.lag) / 4.0;
    y[row][8] = factors.coupling;
    for (int64_t horizon_index = 0; horizon_index < 3; ++horizon_index) {
      const int64_t horizon = int64_t{1} << horizon_index;
      double sum = 0.0;
      for (int64_t offset = 0; offset < horizon; ++offset) {
        sum += noiseless_value(factors, 0, kHistory + offset);
      }
      y[row][9 + horizon_index] = sum / static_cast<double>(horizon);
    }

    for (int64_t channel = 0; channel < kChannels; ++channel) {
      for (int64_t history = 0; history < kHistory; ++history) {
        const double current =
            observed_value(factors, group, channel, history, view);
        const double previous =
            observed_value(factors, group, channel, history - 1, view);
        double mean3 = 0.0;
        double square3 = 0.0;
        double mean8 = 0.0;
        double square8 = 0.0;
        for (int64_t offset = 0; offset < 8; ++offset) {
          const double value =
              observed_value(factors, group, channel, history - offset, view);
          mean8 += value;
          square8 += value * value;
          if (offset < 3) {
            mean3 += value;
            square3 += value * value;
          }
        }
        mean3 /= 3.0;
        mean8 /= 8.0;
        const double std3 =
            std::sqrt(std::max(0.0, square3 / 3.0 - mean3 * mean3));
        const double std8 =
            std::sqrt(std::max(0.0, square8 / 8.0 - mean8 * mean8));
        const int64_t other = (channel + 1) % kChannels;
        const double other_value =
            observed_value(factors, group, other, history, view);
        const std::array<double, kFeatures> features{current,
                                                     current - previous,
                                                     std::fabs(current),
                                                     current * current,
                                                     mean3,
                                                     std3,
                                                     mean8,
                                                     std8,
                                                     current * other_value};
        for (int64_t feature = 0; feature < kFeatures; ++feature) {
          x[row][channel][history][feature] =
              static_cast<float>(features[static_cast<std::size_t>(feature)]);
          if (view == 1 &&
              uniform01(key(group, 40 + channel, history * kFeatures + feature,
                            view)) < 0.04) {
            valid[row][channel][history][feature] = false;
          }
        }
      }
    }
  }
  return {.data = std::move(data),
          .mask = std::move(mask),
          .target = std::move(target),
          .group_begin = group_begin};
}

[[nodiscard]] Normalization fit_normalization(const Dataset &dataset) {
  const auto arranged = dataset.data.permute({1, 3, 0, 2})
                            .contiguous()
                            .view({kChannels, kFeatures, -1});
  const auto mean = arranged.mean(/*dim=*/2);
  const auto variance = (arranged - mean.unsqueeze(-1)).pow(2).mean(/*dim=*/2);
  const auto inv_std = torch::where(variance > 1.0e-12, variance.rsqrt(),
                                    torch::ones_like(variance));
  return {.mean = mean.view({1, kChannels, 1, kFeatures}),
          .inv_std = inv_std.view({1, kChannels, 1, kFeatures})};
}

void normalize(Dataset &dataset, const Normalization &normalization) {
  dataset.data = ((dataset.data - normalization.mean) * normalization.inv_std)
                     .masked_fill(dataset.mask.logical_not(), 0.0)
                     .contiguous();
}

void validate_dataset(const Dataset &dataset) {
  if (dataset.data.dim() != 4 || dataset.data.size(1) != kChannels ||
      dataset.data.size(2) != kHistory || dataset.data.size(3) != kFeatures ||
      dataset.mask.sizes() != dataset.data.sizes() ||
      dataset.target.sizes() !=
          torch::IntArrayRef({dataset.data.size(0), kTargets}) ||
      !torch::isfinite(dataset.data).all().item<bool>() ||
      !torch::isfinite(dataset.target).all().item<bool>()) {
    throw std::runtime_error("generated dataset contract failed");
  }
}

[[nodiscard]] torch::Tensor make_raw_equal_width_projection() {
  torch::NoGradGuard no_grad;
  auto dense = torch::empty({kRawChannelWidth, kLatentDim}, torch::kFloat64);
  auto values = dense.accessor<double, 2>();
  for (int64_t row = 0; row < kRawChannelWidth; ++row) {
    for (int64_t column = 0; column < kLatentDim; ++column) {
      const uint64_t projection_key = splitmix64(
          0x7261775f70726f6aULL ^ splitmix64(static_cast<uint64_t>(row)) ^
          splitmix64(static_cast<uint64_t>(column) << 32U));
      values[row][column] = signed_uniform(projection_key);
    }
  }
  auto [projection, ignored] = at::linalg_qr(dense, "reduced");
  (void)ignored;
  const auto identity = torch::eye(kLatentDim, torch::kFloat64);
  const double orthogonality_error =
      (projection.transpose(0, 1).matmul(projection) - identity)
          .abs()
          .max()
          .item<double>();
  if (projection.sizes() !=
          torch::IntArrayRef({kRawChannelWidth, kLatentDim}) ||
      !torch::isfinite(projection).all().item<bool>() ||
      orthogonality_error > 1.0e-10) {
    throw std::runtime_error("raw equal-width projection contract failed");
  }
  return projection.contiguous();
}

[[nodiscard]] torch::Tensor
raw_equal_width_features(const Dataset &dataset,
                         const torch::Tensor &projection) {
  if (dataset.data.dim() != 4 || dataset.data.size(1) != kChannels ||
      dataset.data.size(2) != kHistory || dataset.data.size(3) != kFeatures ||
      !dataset.mask.all().item<bool>()) {
    throw std::runtime_error("raw control requires complete history rows");
  }
  const auto raw =
      dataset.data.to(torch::kCPU, torch::kFloat64)
          .reshape({dataset.data.size(0), kChannels, kRawChannelWidth});
  const auto projected = raw.matmul(projection);
  if (projected.sizes() !=
      torch::IntArrayRef({dataset.data.size(0), kChannels, kLatentDim})) {
    throw std::runtime_error("raw equal-width feature contract failed");
  }
  return projected.reshape({dataset.data.size(0), kServedWidth}).contiguous();
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t
active_config(const torch::Device &device, bool weak_views) {
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
  config.vicreg_view_gaussian_jitter_std = weak_views ? 0.005 : 0.0;
  config.vicreg_view_time_dropout_scale = weak_views ? 0.10 : 0.0;
  config.target_ema_tau = 0.990;
  config.use_target_ema = true;
  config.stop_gradient_target = true;
  config.return_diagnostics = true;
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
  config.device = device;
  return config;
}

[[nodiscard]] Embeddings extract_embeddings(mtf::MtfJepaMaeVicreg &model,
                                            const Dataset &dataset,
                                            const torch::Device &device) {
  const bool was_training = model->is_training();
  model->eval();
  torch::NoGradGuard no_grad;
  std::vector<torch::Tensor> chunks;
  for (int64_t begin = 0; begin < dataset.data.size(0);
       begin += kModelRowBatchSize) {
    const int64_t size =
        std::min<int64_t>(kModelRowBatchSize, dataset.data.size(0) - begin);
    const auto encoded =
        model->encode(dataset.data.narrow(0, begin, size).to(device),
                      dataset.mask.narrow(0, begin, size).to(device));
    const auto served = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::all_tokens, model->config());
    if (!served.valid_mask.all().item<bool>() ||
        served.values.sizes() !=
            torch::IntArrayRef({size, kChannels, kLatentDim})) {
      throw std::runtime_error("active serving surface contract failed");
    }
    chunks.push_back(served.values.detach().to(torch::kCPU, torch::kFloat64));
  }
  model->train(was_training);
  auto by_channel = torch::cat(chunks, 0).contiguous();
  return {.by_channel = by_channel,
          .flat = by_channel.reshape({by_channel.size(0), kServedWidth})};
}

[[nodiscard]] VicregGeometrySurfaces
extract_vicreg_geometry_surfaces(mtf::MtfJepaMaeVicreg &model,
                                 const Dataset &dataset,
                                 const torch::Device &device) {
  const bool was_training = model->is_training();
  model->eval();
  torch::NoGradGuard no_grad;
  std::vector<torch::Tensor> global_chunks;
  std::vector<torch::Tensor> projected_channel_chunks;
  for (int64_t begin = 0; begin < dataset.data.size(0);
       begin += kModelRowBatchSize) {
    const int64_t size =
        std::min<int64_t>(kModelRowBatchSize, dataset.data.size(0) - begin);
    const auto encoded =
        model->encode(dataset.data.narrow(0, begin, size).to(device),
                      dataset.mask.narrow(0, begin, size).to(device));
    const auto projected = model->project_vicreg(encoded.pooled_by_channel);
    if (!encoded.sample_valid_mask.all().item<bool>() ||
        !encoded.channel_valid_mask.all().item<bool>() ||
        encoded.pooled_embedding.sizes() !=
            torch::IntArrayRef({size, kLatentDim}) ||
        projected.sizes() != torch::IntArrayRef({size, kChannels, 64})) {
      throw std::runtime_error("clean VICReg geometry surface contract failed");
    }
    global_chunks.push_back(
        encoded.pooled_embedding.detach().to(torch::kCPU, torch::kFloat64));
    projected_channel_chunks.push_back(
        projected.detach().to(torch::kCPU, torch::kFloat64));
  }
  model->train(was_training);
  return {.global_preprojector =
              torch::cat(global_chunks, /*dim=*/0).contiguous(),
          .projected_by_channel =
              torch::cat(projected_channel_chunks, /*dim=*/0).contiguous()};
}

[[nodiscard]] std::vector<int64_t>
epoch_permutation(int64_t samples, int64_t seed, int64_t epoch) {
  std::vector<int64_t> order(static_cast<std::size_t>(samples));
  std::iota(order.begin(), order.end(), 0);
  uint64_t state = splitmix64(static_cast<uint64_t>(seed) ^
                              (static_cast<uint64_t>(epoch) << 32U));
  for (int64_t index = samples - 1; index > 0; --index) {
    state = splitmix64(state);
    const int64_t other =
        static_cast<int64_t>(state % static_cast<uint64_t>(index + 1));
    std::swap(order[static_cast<std::size_t>(index)],
              order[static_cast<std::size_t>(other)]);
  }
  return order;
}

[[nodiscard]] TrainingStats train_model(mtf::MtfJepaMaeVicreg &model,
                                        const Dataset &ssl,
                                        const torch::Device &device,
                                        int64_t seed, int64_t steps) {
  auto parameters = model->parameters();
  torch::optim::Adam optimizer(parameters, torch::optim::AdamOptions(1.0e-3));
  std::vector<double> losses;
  losses.reserve(static_cast<std::size_t>(steps));
  const int64_t batches_per_epoch = std::max<int64_t>(
      1, (ssl.data.size(0) + kModelRowBatchSize - 1) / kModelRowBatchSize);
  std::vector<int64_t> order;
  int64_t current_epoch = -1;
  torch::manual_seed(seed * 100003 + 701);
  model->train();
  TrainingStats stats{};
  stats.finite = true;
  for (int64_t step = 0; step < steps; ++step) {
    const int64_t epoch = step / batches_per_epoch;
    if (epoch != current_epoch) {
      order = epoch_permutation(ssl.data.size(0), seed, epoch);
      current_epoch = epoch;
    }
    const int64_t start = (step % batches_per_epoch) * kModelRowBatchSize;
    std::vector<int64_t> rows;
    rows.reserve(kModelRowBatchSize);
    for (int64_t index = 0; index < kModelRowBatchSize; ++index) {
      rows.push_back(
          order[static_cast<std::size_t>((start + index) % ssl.data.size(0))]);
    }
    const auto row_index = torch::tensor(rows, torch::kInt64);
    optimizer.zero_grad();
    const auto output =
        model->forward(ssl.data.index_select(0, row_index).to(device),
                       ssl.mask.index_select(0, row_index).to(device));
    const double loss = output.loss.item<double>();
    if (!std::isfinite(loss)) {
      stats.finite = false;
      break;
    }
    output.loss.backward();
    double gradient_square_sum = 0.0;
    for (const auto &parameter : parameters) {
      if (!parameter.grad().defined()) {
        continue;
      }
      if (!torch::isfinite(parameter.grad()).all().item<bool>()) {
        stats.finite = false;
        break;
      }
      gradient_square_sum +=
          parameter.grad().detach().pow(2).sum().item<double>();
    }
    if (!stats.finite) {
      break;
    }
    const double gradient_norm = std::sqrt(gradient_square_sum);
    if (gradient_norm > 5.0) {
      const double scale = 5.0 / std::max(gradient_norm, 1.0e-30);
      torch::NoGradGuard no_grad;
      for (const auto &parameter : parameters) {
        if (parameter.grad().defined()) {
          parameter.grad().mul_(scale);
        }
      }
    }
    optimizer.step();
    model->update_target_network();
    losses.push_back(loss);
    ++stats.completed_steps;
  }
  if (!losses.empty()) {
    const std::size_t window = std::min<std::size_t>(16, losses.size());
    stats.first_window_loss =
        std::accumulate(losses.begin(), losses.begin() + window, 0.0) /
        static_cast<double>(window);
    stats.last_window_loss =
        std::accumulate(losses.end() - window, losses.end(), 0.0) /
        static_cast<double>(window);
    stats.improved = stats.last_window_loss <= 0.95 * stats.first_window_loss;
  }
  stats.finite = stats.finite && stats.completed_steps == steps;
  return stats;
}

[[nodiscard]] RidgeModel fit_ridge(const torch::Tensor &features_input,
                                   const torch::Tensor &target_input,
                                   double alpha) {
  torch::NoGradGuard no_grad;
  const auto features = features_input.to(torch::kCPU, torch::kFloat64);
  const auto target = target_input.to(torch::kCPU, torch::kFloat64);
  const auto mean = features.mean(0);
  const auto variance = (features - mean).pow(2).mean(0);
  const auto inv_std = torch::where(variance > 1.0e-12, variance.rsqrt(),
                                    torch::ones_like(variance));
  const auto x = (features - mean) * inv_std;
  const auto bias = target.mean(0);
  const auto y = target - bias;
  auto gram = x.transpose(0, 1).matmul(x);
  gram.diagonal(0, 0, 1).add_(features.size(0) * alpha);
  const auto rhs = x.transpose(0, 1).matmul(y);
  auto [cholesky, info] = at::linalg_cholesky_ex(gram, false, false);
  if (info.max().item<int64_t>() != 0) {
    throw std::runtime_error("ridge Cholesky factorization failed");
  }
  auto weights = at::cholesky_solve(rhs, cholesky, false);
  if (!torch::isfinite(weights).all().item<bool>()) {
    throw std::runtime_error("ridge solution is non-finite");
  }
  return {.mean = mean,
          .inv_std = inv_std,
          .weights = std::move(weights),
          .bias = bias};
}

[[nodiscard]] torch::Tensor predict(const RidgeModel &model,
                                    const torch::Tensor &features) {
  return ((features.to(torch::kCPU, torch::kFloat64) - model.mean) *
          model.inv_std)
             .matmul(model.weights) +
         model.bias;
}

[[nodiscard]] ScoreSummary score(const torch::Tensor &prediction_input,
                                 const torch::Tensor &target_input) {
  const auto prediction = prediction_input.to(torch::kCPU, torch::kFloat64);
  const auto target = target_input.to(torch::kCPU, torch::kFloat64);
  const auto centered = target - target.mean(0);
  const auto sse = (prediction - target).pow(2).sum(0);
  const auto sst = centered.pow(2).sum(0);
  const auto r2 = 1.0 - sse / sst.clamp_min(1.0e-12);
  const auto contiguous_r2 = r2.contiguous();
  const auto values = contiguous_r2.accessor<double, 1>();
  ScoreSummary result{};
  std::array<int64_t, kFamilies> family_counts{};
  for (int64_t task = 0; task < kTargets; ++task) {
    const int64_t family = kTargetFamily[static_cast<std::size_t>(task)];
    result.task[static_cast<std::size_t>(task)] = values[task];
    result.family[static_cast<std::size_t>(family)] += values[task];
    ++family_counts[static_cast<std::size_t>(family)];
  }
  for (int64_t family = 0; family < kFamilies; ++family) {
    result.family[static_cast<std::size_t>(family)] /=
        static_cast<double>(family_counts[static_cast<std::size_t>(family)]);
    result.macro += result.family[static_cast<std::size_t>(family)];
  }
  result.macro /= static_cast<double>(kFamilies);
  return result;
}

[[nodiscard]] ProbePoint
probe_at_sample_count(const torch::Tensor &train_features,
                      const torch::Tensor &validation_features,
                      const torch::Tensor &test_features,
                      const torch::Tensor &train_target,
                      const torch::Tensor &validation_target,
                      const torch::Tensor &test_target, int64_t samples) {
  const auto x = train_features.narrow(0, 0, samples);
  const auto y = train_target.narrow(0, 0, samples);
  std::array<torch::Tensor, kRidgeGrid.size()> validation_predictions{};
  std::array<torch::Tensor, kRidgeGrid.size()> test_predictions{};
  for (std::size_t index = 0; index < kRidgeGrid.size(); ++index) {
    const auto model = fit_ridge(x, y, kRidgeGrid[index]);
    validation_predictions[index] = predict(model, validation_features);
    test_predictions[index] = predict(model, test_features);
  }
  auto chosen = torch::empty({test_target.size(0), kTargets}, torch::kFloat64);
  ProbePoint result{};
  result.samples = samples;
  for (int64_t task = 0; task < kTargets; ++task) {
    double best_mse = std::numeric_limits<double>::infinity();
    std::size_t best = 0;
    for (std::size_t index = 0; index < kRidgeGrid.size(); ++index) {
      const double mse = (validation_predictions[index].select(1, task) -
                          validation_target.select(1, task))
                             .pow(2)
                             .mean()
                             .item<double>();
      if (mse < best_mse) {
        best_mse = mse;
        best = index;
      }
    }
    chosen.select(1, task).copy_(test_predictions[best].select(1, task));
    result.selected_alpha[static_cast<std::size_t>(task)] = kRidgeGrid[best];
  }
  result.prediction = std::move(chosen);
  result.score = score(result.prediction, test_target);
  return result;
}

[[nodiscard]] ProbeCurve probe_curve(
    const torch::Tensor &train_features,
    const torch::Tensor &validation_features,
    const torch::Tensor &test_features, const torch::Tensor &train_target,
    const torch::Tensor &validation_target, const torch::Tensor &test_target,
    const std::vector<int64_t> &sample_ladder) {
  ProbeCurve result{};
  for (const int64_t samples : sample_ladder) {
    result.points.push_back(probe_at_sample_count(
        train_features, validation_features, test_features, train_target,
        validation_target, test_target, samples));
    result.area += result.points.back().score.macro;
  }
  result.area /= static_cast<double>(result.points.size());
  return result;
}

[[nodiscard]] Interval percentile_interval(std::vector<double> values) {
  if (values.empty() ||
      !std::all_of(values.begin(), values.end(),
                   [](double value) { return std::isfinite(value); })) {
    throw std::runtime_error("bootstrap values must be finite and non-empty");
  }
  std::sort(values.begin(), values.end());
  const auto quantile = [&](double probability) {
    const double position =
        probability * static_cast<double>(values.size() - 1);
    const auto lower = static_cast<std::size_t>(std::floor(position));
    const auto upper = static_cast<std::size_t>(std::ceil(position));
    const double fraction = position - static_cast<double>(lower);
    return values[lower] + fraction * (values[upper] - values[lower]);
  };
  return {.low = quantile(0.025), .high = quantile(0.975)};
}

[[nodiscard]] double resampled_curve_area(const ProbeCurve &curve,
                                          const torch::Tensor &resampled_target,
                                          const torch::Tensor &row_index,
                                          int64_t expected_rows,
                                          std::size_t expected_points) {
  if (curve.points.size() != expected_points || curve.points.empty()) {
    throw std::runtime_error("paired bootstrap curve ladder mismatch");
  }
  double area = 0.0;
  for (const auto &point : curve.points) {
    if (point.prediction.dim() != 2 ||
        point.prediction.size(0) != expected_rows ||
        point.prediction.size(1) != kTargets) {
      throw std::runtime_error("paired bootstrap prediction shape mismatch");
    }
    area += score(point.prediction.index_select(0, row_index), resampled_target)
                .macro;
  }
  return area / static_cast<double>(curve.points.size());
}

[[nodiscard]] PairedBootstrapIntervals
paired_group_bootstrap(const std::vector<ProbeCurve> &trained_curves,
                       const std::vector<ProbeCurve> &untrained_curves,
                       const ProbeCurve &raw_control_curve,
                       const torch::Tensor &test_target, int64_t replicates,
                       uint64_t bootstrap_seed) {
  if (replicates <= 0 || trained_curves.empty() ||
      trained_curves.size() != untrained_curves.size() ||
      test_target.dim() != 2 || test_target.size(0) < 2 ||
      test_target.size(1) != kTargets || raw_control_curve.points.empty()) {
    throw std::runtime_error("paired group-bootstrap contract failed");
  }
  const int64_t groups = test_target.size(0);
  const std::size_t ladder_points = raw_control_curve.points.size();
  std::vector<double> trained_minus_untrained;
  std::vector<double> trained_minus_raw_control;
  trained_minus_untrained.reserve(static_cast<std::size_t>(replicates));
  trained_minus_raw_control.reserve(static_cast<std::size_t>(replicates));
  for (int64_t replicate = 0; replicate < replicates; ++replicate) {
    uint64_t state = splitmix64(bootstrap_seed ^
                                splitmix64(static_cast<uint64_t>(replicate)));
    std::vector<int64_t> sampled_rows;
    sampled_rows.reserve(static_cast<std::size_t>(groups));
    for (int64_t draw = 0; draw < groups; ++draw) {
      state = splitmix64(state);
      sampled_rows.push_back(
          static_cast<int64_t>(state % static_cast<uint64_t>(groups)));
    }
    const auto row_index = torch::tensor(sampled_rows, torch::kInt64);
    const auto resampled_target = test_target.index_select(0, row_index);
    double trained_area = 0.0;
    double untrained_area = 0.0;
    for (std::size_t seed = 0; seed < trained_curves.size(); ++seed) {
      trained_area +=
          resampled_curve_area(trained_curves[seed], resampled_target,
                               row_index, groups, ladder_points);
      untrained_area +=
          resampled_curve_area(untrained_curves[seed], resampled_target,
                               row_index, groups, ladder_points);
    }
    trained_area /= static_cast<double>(trained_curves.size());
    untrained_area /= static_cast<double>(untrained_curves.size());
    const double raw_control_area = resampled_curve_area(
        raw_control_curve, resampled_target, row_index, groups, ladder_points);
    trained_minus_untrained.push_back(trained_area - untrained_area);
    trained_minus_raw_control.push_back(trained_area - raw_control_area);
  }
  return {.trained_minus_untrained_area =
              percentile_interval(std::move(trained_minus_untrained)),
          .trained_minus_raw_control_area =
              percentile_interval(std::move(trained_minus_raw_control))};
}

[[nodiscard]] Geometry geometry_for_channel(const torch::Tensor &features) {
  torch::NoGradGuard no_grad;
  const auto values = features.to(torch::kCPU, torch::kFloat64);
  if (values.dim() != 2 || values.size(1) <= 0) {
    throw std::runtime_error("geometry features must be [samples,dimensions]");
  }
  const int64_t feature_dim = values.size(1);
  const auto centered = values - values.mean(0);
  const auto covariance =
      centered.transpose(0, 1).matmul(centered) /
      static_cast<double>(std::max<int64_t>(1, values.size(0) - 1));
  auto eigenvalues = at::linalg_eigvalsh(covariance, "L").clamp_min(0.0);
  const double total = eigenvalues.sum().item<double>();
  if (!(total > 1.0e-20)) {
    return {};
  }
  const auto probabilities = eigenvalues / total;
  const double entropy =
      -(probabilities * probabilities.clamp_min(1.0e-30).log())
           .sum()
           .item<double>();
  Geometry result{};
  result.effective_rank_ratio =
      std::exp(entropy) / static_cast<double>(feature_dim);
  result.participation_rank_ratio = total * total /
                                    eigenvalues.pow(2).sum().item<double>() /
                                    static_cast<double>(feature_dim);
  result.top_eigenvalue_share = eigenvalues.max().item<double>() / total;
  const auto std = covariance.diagonal().clamp_min(0.0).sqrt();
  const double scale = std::sqrt(total / static_cast<double>(feature_dim));
  result.active_dimension_fraction = std.gt(std::max(1.0e-12, 1.0e-3 * scale))
                                         .to(torch::kFloat64)
                                         .mean()
                                         .item<double>();
  result.passed = result.effective_rank_ratio >= 0.25 &&
                  result.participation_rank_ratio >= 0.20 &&
                  result.top_eigenvalue_share <= 0.80 &&
                  result.active_dimension_fraction >= 0.75;
  return result;
}

[[nodiscard]] std::array<Geometry, kChannels>
geometry(const Embeddings &embeddings) {
  std::array<Geometry, kChannels> result{};
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    result[static_cast<std::size_t>(channel)] =
        geometry_for_channel(embeddings.by_channel.select(1, channel));
  }
  return result;
}

[[nodiscard]] PairSeparation
pair_separation(const torch::Tensor &base_input,
                const torch::Tensor &nuisance_input,
                const torch::Tensor &semantic_input) {
  const auto unit = [](const torch::Tensor &input) {
    return input / input.norm(2, 1, true).clamp_min(1.0e-12);
  };
  const auto base_values = base_input.to(torch::kFloat64);
  const auto center = base_values.mean(0, true);
  const auto base = unit(base_values - center);
  const auto nuisance = unit(nuisance_input.to(torch::kFloat64) - center);
  const auto semantic = unit(semantic_input.to(torch::kFloat64) - center);
  const auto nuisance_distance = 1.0 - (base * nuisance).sum(1);
  const auto semantic_distance = 1.0 - (base * semantic).sum(1);
  const double mean_nuisance_distance = nuisance_distance.mean().item<double>();
  const double mean_semantic_distance = semantic_distance.mean().item<double>();
  return {
      .nuisance_distance = mean_nuisance_distance,
      .semantic_distance = mean_semantic_distance,
      .semantic_over_nuisance_mean_ratio =
          mean_semantic_distance / std::max(1.0e-12, mean_nuisance_distance),
      .semantic_over_nuisance_fraction = semantic_distance.gt(nuisance_distance)
                                             .to(torch::kFloat64)
                                             .mean()
                                             .item<double>()};
}

[[nodiscard]] Tier resolve_tier(const Options &options) {
  if (options.tier == "fast") {
    Tier tier{.ssl_groups = 256,
              .probe_train_groups = 256,
              .probe_validation_groups = 128,
              .test_groups = 256,
              .steps = 128,
              .seeds = 1,
              .bootstrap_replicates = 512,
              .sample_ladder = {32, 64, 128, 256}};
    if (options.steps > 0) {
      tier.steps = options.steps;
    }
    if (options.seeds > 0) {
      tier.seeds = options.seeds;
    }
    return tier;
  }
  if (options.tier == "active") {
    Tier tier{.ssl_groups = 4096,
              .probe_train_groups = 1024,
              .probe_validation_groups = 512,
              .test_groups = 1024,
              .steps = 3000,
              .seeds = 3,
              .bootstrap_replicates = 512,
              .sample_ladder = {32, 64, 128, 256, 512, 1024}};
    if (options.steps > 0) {
      tier.steps = options.steps;
    }
    if (options.seeds > 0) {
      tier.seeds = options.seeds;
    }
    return tier;
  }
  throw std::runtime_error("--tier must be fast or active");
}

[[nodiscard]] Options parse_options(int argc, char **argv) {
  Options options{};
  for (int index = 1; index < argc; ++index) {
    const std::string argument = argv[index];
    const auto value = [&](const char *name) -> std::string {
      if (++index >= argc) {
        throw std::runtime_error(std::string("missing value for ") + name);
      }
      return argv[index];
    };
    if (argument == "--tier") {
      options.tier = value("--tier");
    } else if (argument == "--experiment") {
      options.experiment = value("--experiment");
    } else if (argument == "--device") {
      options.device = value("--device");
    } else if (argument == "--qualifier-log") {
      options.qualifier_log = value("--qualifier-log");
    } else if (argument == "--seeds") {
      options.seeds = std::stoll(value("--seeds"));
    } else if (argument == "--steps") {
      options.steps = std::stoll(value("--steps"));
    } else if (argument == "--weak-views") {
      const auto mode = value("--weak-views");
      if (mode == "on") {
        options.weak_views = true;
      } else if (mode == "off") {
        options.weak_views = false;
      } else {
        throw std::runtime_error("--weak-views must be on or off");
      }
    } else if (argument == "--verbose") {
      options.verbose = true;
    } else {
      throw std::runtime_error("unknown argument: " + argument);
    }
  }
  return options;
}

struct SemanticQualifierEvidence {
  int64_t schema_count{0};
  int64_t candidate_qualified_count{0};
  int64_t full_active_not_qualified_count{0};
  int64_t global_not_qualified_count{0};
  bool schema_value_exact{true};
  bool candidate_value_exact{true};
  bool full_active_value_exact{true};
  bool global_value_exact{true};
  bool pass{false};
};

[[nodiscard]] SemanticQualifierEvidence
parse_semantic_qualifier_evidence(const std::string &path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("cannot open semantic qualifier log: " + path);
  }
  SemanticQualifierEvidence evidence{};
  std::string line;
  while (std::getline(input, line)) {
    if (!line.empty() && line.back() == '\r') {
      line.pop_back();
    }
    const auto separator = line.find('=');
    if (separator == std::string::npos) {
      continue;
    }
    const auto key = line.substr(0, separator);
    if (key == "schema_id") {
      ++evidence.schema_count;
      evidence.schema_value_exact =
          evidence.schema_value_exact &&
          line == "schema_id=mtf_augmentation_semantic_qualification.v1";
    } else if (key == "attribution.candidate_safe_stack.result") {
      ++evidence.candidate_qualified_count;
      evidence.candidate_value_exact =
          evidence.candidate_value_exact &&
          line == "attribution.candidate_safe_stack.result=QUALIFIED";
    } else if (key == "attribution.full_active_stack.result") {
      ++evidence.full_active_not_qualified_count;
      evidence.full_active_value_exact =
          evidence.full_active_value_exact &&
          line == "attribution.full_active_stack.result=NOT_QUALIFIED";
    } else if (key == "result") {
      ++evidence.global_not_qualified_count;
      evidence.global_value_exact =
          evidence.global_value_exact && line == "result=NOT_QUALIFIED";
    }
  }
  evidence.pass =
      evidence.schema_count == 1 && evidence.candidate_qualified_count == 1 &&
      evidence.full_active_not_qualified_count == 1 &&
      evidence.global_not_qualified_count == 1 && evidence.schema_value_exact &&
      evidence.candidate_value_exact && evidence.full_active_value_exact &&
      evidence.global_value_exact;
  return evidence;
}

[[nodiscard]] bool
all_geometry_passed(const std::array<Geometry, kChannels> &summaries) {
  return std::all_of(summaries.begin(), summaries.end(),
                     [](const Geometry &summary) { return summary.passed; });
}

void emit_geometry_summary(const std::string &prefix, const Geometry &summary,
                           bool emit_passed = true) {
  std::cout << prefix
            << ".effective_rank_ratio=" << summary.effective_rank_ratio << '\n';
  std::cout << prefix
            << ".participation_rank_ratio=" << summary.participation_rank_ratio
            << '\n';
  std::cout << prefix
            << ".top_eigenvalue_share=" << summary.top_eigenvalue_share << '\n';
  std::cout << prefix << ".active_dimension_fraction="
            << summary.active_dimension_fraction << '\n';
  if (emit_passed) {
    std::cout << prefix << ".passed=" << summary.passed << '\n';
  }
}

void emit_geometry(const std::string &prefix,
                   const std::array<Geometry, kChannels> &summaries,
                   bool emit_passed = true) {
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    emit_geometry_summary(prefix + ".channel_" + std::to_string(channel),
                          summaries[static_cast<std::size_t>(channel)],
                          emit_passed);
  }
}

int run(const Options &options) {
  const Tier tier = resolve_tier(options);
  if (tier.seeds <= 0 || tier.seeds > 5 || tier.steps <= 0) {
    throw std::runtime_error(
        "seeds must be in [1,5] and steps must be positive");
  }
  torch::Device device(torch::kCPU);
  if (options.device == "cuda") {
    if (!torch::cuda::is_available()) {
      throw std::runtime_error("CUDA requested but unavailable");
    }
    device = torch::Device(torch::kCUDA, 0);
  } else if (options.device != "cpu") {
    throw std::runtime_error("--device must be cpu or cuda");
  }
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout
      << "schema=wikimyei.mtf_jepa_mae_vicreg.representation_quality.v1\n";
  std::cout << "module_only=true\n";
  std::cout << "tier=" << options.tier << '\n';
  std::cout << "device=" << options.device << '\n';
  std::cout << "architecture_scope=exact_active_H30_F9_D32\n";
  std::cout << "primary_surface=all_tokens_3x32\n";
  std::cout << "control=same_seed_untrained_3x32\n";
  std::cout
      << "raw_equal_width_control=fixed_orthonormal_32_per_channel_96_total\n";
  std::cout << "training_scope=representation_core_only\n";
  std::cout << "launcher_augmentation=false\n";
  std::cout << "vicreg_weak_view_augmentation=" << options.weak_views << '\n';
  std::cout << "model_row_batch_size=" << kModelRowBatchSize << '\n';
  std::cout << "active_anchor_batch_equivalent=" << kActiveAnchorBatchEquivalent
            << '\n';
  std::cout << "active_node_count_assumption=" << kActiveNodeCountAssumption
            << '\n';
  std::cout << "training_steps=" << tier.steps << '\n';
  std::cout << "model_seeds=" << tier.seeds << '\n';
  std::cout << "probe_selection=validation_selected_ridge\n";
  std::cout << "uncertainty_scope=paired_group_bootstrap_fixed_seed_mean_plus_"
               "seed_point_estimates\n";
  std::cout << "paired_group_bootstrap_replicates=" << tier.bootstrap_replicates
            << '\n';
  std::cout << "paired_group_bootstrap_unit=generated_group_row\n";
  std::cout << "paired_group_bootstrap_seed_aggregation=fixed_seed_mean\n";

  auto ssl = generate_dataset(0, tier.ssl_groups);
  auto probe_train = generate_dataset(1000000, tier.probe_train_groups);
  auto probe_validation =
      generate_dataset(2000000, tier.probe_validation_groups);
  auto test = generate_dataset(3000000, tier.test_groups);
  auto test_nuisance = generate_dataset(3000000, tier.test_groups, 1, false);
  auto test_semantic = generate_dataset(3000000, tier.test_groups, 0, true);
  const auto raw_projection = make_raw_equal_width_projection();
  const auto raw_control_train =
      raw_equal_width_features(probe_train, raw_projection);
  const auto raw_control_validation =
      raw_equal_width_features(probe_validation, raw_projection);
  const auto raw_control_test = raw_equal_width_features(test, raw_projection);
  const auto normalization = fit_normalization(ssl);
  for (Dataset *dataset : {&ssl, &probe_train, &probe_validation, &test,
                           &test_nuisance, &test_semantic}) {
    normalize(*dataset, normalization);
    validate_dataset(*dataset);
  }
  if (!probe_train.target.var(0, false).gt(1.0e-8).all().item<bool>()) {
    throw std::runtime_error("one or more probe targets lack variance");
  }
  const auto raw_control_probe =
      probe_curve(raw_control_train, raw_control_validation, raw_control_test,
                  probe_train.target, probe_validation.target, test.target,
                  tier.sample_ladder);
  const auto &raw_control_final = raw_control_probe.points.back().score;
  std::cout << "control.raw_equal_width.area=" << raw_control_probe.area
            << '\n';
  std::cout << "control.raw_equal_width.final_macro_r2="
            << raw_control_final.macro << '\n';
  for (int64_t task = 0; task < kTargets; ++task) {
    std::cout << "control.raw_equal_width.task_" << task << "_r2="
              << raw_control_final.task[static_cast<std::size_t>(task)] << '\n';
  }
  for (int64_t family = 0; family < kFamilies; ++family) {
    std::cout << "control.raw_equal_width.family_"
              << kFamilyNames[static_cast<std::size_t>(family)] << "_r2="
              << raw_control_final.family[static_cast<std::size_t>(family)]
              << '\n';
  }

  constexpr std::array<int64_t, 5> seeds{17, 31, 47, 59, 71};
  int64_t mechanics_pass_count = 0;
  int64_t seed_quality_pass_count = 0;
  int64_t geometry_pass_count = 0;
  int64_t probe_pass_count = 0;
  int64_t robustness_pass_count = 0;
  std::vector<ProbeCurve> untrained_probe_curves;
  std::vector<ProbeCurve> trained_probe_curves;
  untrained_probe_curves.reserve(static_cast<std::size_t>(tier.seeds));
  trained_probe_curves.reserve(static_cast<std::size_t>(tier.seeds));
  for (int64_t seed_index = 0; seed_index < tier.seeds; ++seed_index) {
    const int64_t seed = seeds[static_cast<std::size_t>(seed_index)];
    torch::manual_seed(seed);
    auto model =
        mtf::MtfJepaMaeVicreg(active_config(device, options.weak_views));
    const auto untrained_train = extract_embeddings(model, probe_train, device);
    const auto untrained_validation =
        extract_embeddings(model, probe_validation, device);
    const auto untrained_test = extract_embeddings(model, test, device);
    const auto untrained_nuisance =
        extract_embeddings(model, test_nuisance, device);
    const auto untrained_semantic =
        extract_embeddings(model, test_semantic, device);
    const auto untrained_probe =
        probe_curve(untrained_train.flat, untrained_validation.flat,
                    untrained_test.flat, probe_train.target,
                    probe_validation.target, test.target, tier.sample_ladder);

    const auto training = train_model(model, ssl, device, seed, tier.steps);
    const auto trained_train = extract_embeddings(model, probe_train, device);
    const auto trained_validation =
        extract_embeddings(model, probe_validation, device);
    const auto trained_test = extract_embeddings(model, test, device);
    const auto trained_nuisance =
        extract_embeddings(model, test_nuisance, device);
    const auto trained_semantic =
        extract_embeddings(model, test_semantic, device);
    const auto trained_probe =
        probe_curve(trained_train.flat, trained_validation.flat,
                    trained_test.flat, probe_train.target,
                    probe_validation.target, test.target, tier.sample_ladder);
    const auto untrained_geometry = geometry(untrained_test);
    const auto trained_geometry = geometry(trained_test);
    const auto untrained_pairs = pair_separation(
        untrained_test.flat, untrained_nuisance.flat, untrained_semantic.flat);
    const auto trained_pairs = pair_separation(
        trained_test.flat, trained_nuisance.flat, trained_semantic.flat);

    const double area_delta = trained_probe.area - untrained_probe.area;
    const double raw_control_area_delta =
        trained_probe.area - raw_control_probe.area;
    const auto &trained_final = trained_probe.points.back().score;
    const auto &untrained_final = untrained_probe.points.back().score;
    int64_t improved_families = 0;
    bool material_family_regression = false;
    for (int64_t family = 0; family < kFamilies; ++family) {
      const double delta =
          trained_final.family[static_cast<std::size_t>(family)] -
          untrained_final.family[static_cast<std::size_t>(family)];
      improved_families += delta > 0.0 ? 1 : 0;
      material_family_regression = material_family_regression || delta < -0.02;
    }
    const bool mechanics_pass = training.finite && training.improved;
    const bool geometry_pass = all_geometry_passed(trained_geometry);
    const bool probe_pass = area_delta > 0.02 && improved_families >= 3 &&
                            !material_family_regression;
    const bool raw_control_point_pass = raw_control_area_delta > 0.02;
    const bool robustness_pass =
        trained_pairs.semantic_over_nuisance_fraction >= 0.75 &&
        trained_pairs.semantic_over_nuisance_mean_ratio >= 2.0 &&
        trained_pairs.semantic_over_nuisance_fraction >
            untrained_pairs.semantic_over_nuisance_fraction;
    const bool seed_pass =
        mechanics_pass && geometry_pass && probe_pass && robustness_pass;
    mechanics_pass_count += mechanics_pass ? 1 : 0;
    geometry_pass_count += geometry_pass ? 1 : 0;
    probe_pass_count += probe_pass ? 1 : 0;
    robustness_pass_count += robustness_pass ? 1 : 0;
    seed_quality_pass_count += seed_pass ? 1 : 0;

    const std::string prefix = "seed_" + std::to_string(seed);
    std::cout << prefix
              << ".training.completed_steps=" << training.completed_steps
              << '\n';
    std::cout << prefix
              << ".training.first_window_loss=" << training.first_window_loss
              << '\n';
    std::cout << prefix
              << ".training.last_window_loss=" << training.last_window_loss
              << '\n';
    std::cout << prefix << ".training.improved=" << training.improved << '\n';
    std::cout << prefix << ".probe.untrained_area=" << untrained_probe.area
              << '\n';
    std::cout << prefix << ".probe.trained_area=" << trained_probe.area << '\n';
    std::cout << prefix << ".probe.trained_minus_untrained_area=" << area_delta
              << '\n';
    std::cout << prefix << ".probe.trained_minus_raw_equal_width_area="
              << raw_control_area_delta << '\n';
    std::cout << prefix
              << ".probe.raw_equal_width_point_pass=" << raw_control_point_pass
              << '\n';
    for (int64_t family = 0; family < kFamilies; ++family) {
      const std::string family_prefix =
          prefix + ".family_" + kFamilyNames[static_cast<std::size_t>(family)];
      std::cout << family_prefix << ".untrained_r2="
                << untrained_final.family[static_cast<std::size_t>(family)]
                << '\n';
      std::cout << family_prefix << ".trained_r2="
                << trained_final.family[static_cast<std::size_t>(family)]
                << '\n';
      std::cout << family_prefix << ".delta_r2="
                << trained_final.family[static_cast<std::size_t>(family)] -
                       untrained_final.family[static_cast<std::size_t>(family)]
                << '\n';
    }
    std::cout << prefix << ".pairs.untrained_separation_fraction="
              << untrained_pairs.semantic_over_nuisance_fraction << '\n';
    std::cout << prefix << ".pairs.trained_separation_fraction="
              << trained_pairs.semantic_over_nuisance_fraction << '\n';
    std::cout << prefix << ".pairs.untrained_mean_distance_ratio="
              << untrained_pairs.semantic_over_nuisance_mean_ratio << '\n';
    std::cout << prefix << ".pairs.trained_mean_distance_ratio="
              << trained_pairs.semantic_over_nuisance_mean_ratio << '\n';
    std::cout << prefix << ".pairs.trained_nuisance_distance="
              << trained_pairs.nuisance_distance << '\n';
    std::cout << prefix << ".pairs.trained_semantic_distance="
              << trained_pairs.semantic_distance << '\n';
    emit_geometry(prefix + ".geometry.untrained", untrained_geometry);
    emit_geometry(prefix + ".geometry.trained", trained_geometry);
    std::cout << prefix << ".mechanics_pass=" << mechanics_pass << '\n';
    std::cout << prefix << ".probe_pass=" << probe_pass << '\n';
    std::cout << prefix << ".robustness_pass=" << robustness_pass << '\n';
    std::cout << prefix << ".geometry_pass=" << geometry_pass << '\n';
    std::cout << prefix << ".quality_pass=" << seed_pass << '\n';
    untrained_probe_curves.push_back(untrained_probe);
    trained_probe_curves.push_back(trained_probe);
  }

  const bool all_mechanics = mechanics_pass_count == tier.seeds;
  const bool all_seeds_quality = seed_quality_pass_count == tier.seeds;
  bool paired_bootstrap_control_pass = false;
  std::cout << "summary.paired_group_bootstrap.available="
            << (tier.bootstrap_replicates > 0) << '\n';
  if (tier.bootstrap_replicates > 0) {
    const auto bootstrap = paired_group_bootstrap(
        trained_probe_curves, untrained_probe_curves, raw_control_probe,
        test.target, tier.bootstrap_replicates, 0x7175616c6974795fULL);
    paired_bootstrap_control_pass =
        bootstrap.trained_minus_untrained_area.low > 0.02 &&
        bootstrap.trained_minus_raw_control_area.low > 0.02;
    std::cout
        << "summary.paired_group_bootstrap.trained_minus_untrained_area.low="
        << bootstrap.trained_minus_untrained_area.low << '\n';
    std::cout
        << "summary.paired_group_bootstrap.trained_minus_untrained_area.high="
        << bootstrap.trained_minus_untrained_area.high << '\n';
    std::cout << "summary.paired_group_bootstrap."
                 "trained_minus_raw_equal_width_area.low="
              << bootstrap.trained_minus_raw_control_area.low << '\n';
    std::cout << "summary.paired_group_bootstrap."
                 "trained_minus_raw_equal_width_area.high="
              << bootstrap.trained_minus_raw_control_area.high << '\n';
  }
  std::cout << "summary.paired_group_bootstrap.control_pass="
            << paired_bootstrap_control_pass << '\n';
  // The added raw/equal-width control and paired group interval strengthen V1,
  // but the broader preregistered release protocol remains intentionally
  // outside this executable's qualification claim.
  const bool full_qualification = false;
  std::string classification;
  if (!all_mechanics) {
    classification = "training_mechanics_failed";
  } else if (geometry_pass_count != tier.seeds) {
    classification = "served_representation_geometry_failed";
  } else if (probe_pass_count != tier.seeds) {
    classification = "no_robust_linear_representation_improvement";
  } else if (robustness_pass_count != tier.seeds) {
    classification = "semantic_nuisance_separation_failed";
  } else if (options.tier == "fast") {
    classification = "fast_screen_pass_active_release_required";
  } else if (all_seeds_quality) {
    classification = "active_screen_pass_controls_and_ci_required";
  } else {
    classification = "active_quality_evidence_incomplete";
  }
  std::cout << "summary.mechanics_pass_count=" << mechanics_pass_count << '\n';
  std::cout << "summary.probe_pass_count=" << probe_pass_count << '\n';
  std::cout << "summary.robustness_pass_count=" << robustness_pass_count
            << '\n';
  std::cout << "summary.geometry_pass_count=" << geometry_pass_count << '\n';
  std::cout << "summary.seed_quality_pass_count=" << seed_quality_pass_count
            << '\n';
  std::cout << "classification=" << classification << '\n';
  std::cout << "full_quality_qualification=" << full_qualification << '\n';
  return all_mechanics ? 0 : 3;
}

constexpr int64_t kAttributionSteps = 32;
constexpr int64_t kAttributionMidpoint = 16;
constexpr int64_t kAttributionBootstrapReplicates = 512;
constexpr double kGradientMatchedTfInitialWeight = 0.0165124;
constexpr double kGradientMatchedTfFinalWeight = 0.10;
constexpr int64_t kGradientMatchedTfRampUpdates = 16;
constexpr uint64_t kRepairPrimaryBootstrapSeed = 0x74665f7761726d75ULL;
constexpr std::size_t kRepairJmIndex = 2;
constexpr std::size_t kRepairFixedTfIndex = 4;
constexpr std::size_t kRepairGlobalVicregIndex = 5;
constexpr std::size_t kRepairWarmupIndex = 6;
constexpr std::size_t kRepairStratifiedVicregIndex = 7;
constexpr std::array<int64_t, 3> kAttributionSeeds{17, 31, 47};
static_assert(kAttributionSeeds.size() == repair_gate::kRepairSeedCount);
static_assert(static_cast<std::size_t>(kFamilies) ==
              repair_gate::kRepairFamilyCount);
constexpr std::array<const char *, 4> kAttributionBranchNames{
    "jepa", "mae", "tf_align", "vicreg"};
constexpr std::array<const char *, 3> kVicregComponentNames{"sim", "var",
                                                            "cov"};

struct AttributionArm {
  const char *name;
  double lambda_jepa;
  double lambda_mae;
  double lambda_tf_align;
  double lambda_vicreg;
  double max_context_target_time_overlap;
  bool tf_gradient_matched_warmup{false};
  bool projected_channel_stratified_vicreg{false};
  double vicreg_var_weight{25.0};
};

constexpr std::array<AttributionArm, 8> kAttributionArms{{
    {.name = "full_soft",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.10,
     .lambda_vicreg = 0.05,
     .max_context_target_time_overlap = 0.50},
    {.name = "no_jepa_mae_gradient",
     .lambda_jepa = 0.0,
     .lambda_mae = 0.0,
     .lambda_tf_align = 0.10,
     .lambda_vicreg = 0.05,
     .max_context_target_time_overlap = 0.50},
    {.name = "jepa_mae_only",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.0,
     .lambda_vicreg = 0.0,
     .max_context_target_time_overlap = 0.50},
    {.name = "full_overlap_allowed",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.10,
     .lambda_vicreg = 0.05,
     .max_context_target_time_overlap = 1.0},
    {.name = "jepa_mae_plus_tf",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.10,
     .lambda_vicreg = 0.0,
     .max_context_target_time_overlap = 0.50},
    {.name = "jepa_mae_plus_vicreg",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.0,
     .lambda_vicreg = 0.05,
     .max_context_target_time_overlap = 0.50},
    {.name = "jepa_mae_plus_tf_gradient_matched_warmup",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.10,
     .lambda_vicreg = 0.0,
     .max_context_target_time_overlap = 0.50,
     .tf_gradient_matched_warmup = true},
    {.name = "jepa_mae_plus_projected_channel_stratified_vicreg",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.0,
     .lambda_vicreg = 0.05,
     .max_context_target_time_overlap = 0.50,
     .projected_channel_stratified_vicreg = true},
}};

constexpr std::array<AttributionArm, 3> kVarianceNecessityArms{{
    {.name = "jepa_mae_only",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.0,
     .lambda_vicreg = 0.0,
     .max_context_target_time_overlap = 0.50},
    {.name = "jepa_mae_plus_projected_channel_stratified_vicreg",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.0,
     .lambda_vicreg = 0.05,
     .max_context_target_time_overlap = 0.50,
     .projected_channel_stratified_vicreg = true},
    {.name = "jepa_mae_plus_projected_channel_stratified_vicreg_no_variance",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.0,
     .lambda_vicreg = 0.05,
     .max_context_target_time_overlap = 0.50,
     .projected_channel_stratified_vicreg = true,
     .vicreg_var_weight = 0.0},
}};

constexpr std::size_t kOuterNeutralIndex = 0;
constexpr std::size_t kOuterFullActiveIndex = 1;
constexpr std::size_t kOuterQualifiedIndex = 2;
constexpr uint64_t kOuterAugmentationSeedDomain = 0x6f75746572617567ULL;

constexpr std::array<AttributionArm, 3> kOuterAugmentationArms{{
    {.name = "jepa_mae_only",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.0,
     .lambda_vicreg = 0.0,
     .max_context_target_time_overlap = 0.50},
    {.name = "jepa_mae_plus_full_active_outer_augmentation",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.0,
     .lambda_vicreg = 0.0,
     .max_context_target_time_overlap = 0.50},
    {.name = "jepa_mae_plus_qualified_outer_augmentation",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.0,
     .lambda_vicreg = 0.0,
     .max_context_target_time_overlap = 0.50},
}};
static_assert(kOuterAugmentationArms.size() == kAttributionSeeds.size());

constexpr std::size_t kJmcdCombinedIndex = 0;
constexpr std::size_t kJmcdJepaIndex = 1;
constexpr std::size_t kJmcdMaeIndex = 2;
constexpr std::size_t kJmcdNullIndex = 3;
constexpr std::array<AttributionArm, 4> kJmcdArms{{
    {.name = "jepa_mae_only",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.0,
     .lambda_vicreg = 0.0,
     .max_context_target_time_overlap = 0.50},
    {.name = "jepa_only",
     .lambda_jepa = 1.0,
     .lambda_mae = 0.0,
     .lambda_tf_align = 0.0,
     .lambda_vicreg = 0.0,
     .max_context_target_time_overlap = 0.50},
    {.name = "mae_only",
     .lambda_jepa = 0.0,
     .lambda_mae = 0.25,
     .lambda_tf_align = 0.0,
     .lambda_vicreg = 0.0,
     .max_context_target_time_overlap = 0.50},
    {.name = "core_objective_null",
     .lambda_jepa = 0.0,
     .lambda_mae = 0.0,
     .lambda_tf_align = 0.0,
     .lambda_vicreg = 0.0,
     .max_context_target_time_overlap = 0.50},
}};
static_assert(kAttributionSeeds.size() == jmcd_gate::kSeedCount);
static_assert(static_cast<std::size_t>(kFamilies) == jmcd_gate::kFamilyCount);

struct GradientDiagnostic {
  std::array<double, 4> raw_loss{};
  std::array<double, 4> served_norm{};
  std::array<double, 4> tokenizer_norm{};
  std::array<double, 4> encoder_norm{};
  std::array<double, 4> predictor_norm{};
  std::array<double, 4> mae_decoder_norm{};
  std::array<double, 4> vicreg_head_norm{};
  std::array<double, 6> served_cosine{};
  std::array<double, 3> vicreg_component_raw_loss{};
  std::array<double, 3> vicreg_component_raw_trunk_norm{};
  std::array<double, 3> vicreg_component_raw_head_norm{};
  std::array<double, 3> vicreg_component_effective_weight{};
  std::array<double, 3> vicreg_component_effective_trunk_norm{};
  std::array<double, 3> vicreg_component_effective_head_norm{};
  std::array<double, kChannels> vicreg_view_a_variance_floor_fraction{};
  std::array<double, kChannels> vicreg_view_b_variance_floor_fraction{};
  std::array<int64_t, kChannels> vicreg_variance_floor_valid_rows{};
  double jepa_mae_norm{0.0};
  double tf_vicreg_norm{0.0};
  double canonical_total_norm{0.0};
  double cancellation_ratio{0.0};
  double tf_weighted_served_norm_ratio{0.0};
  double actual_arm_weighted_cancellation_ratio{0.0};
  double actual_arm_all_trainable_norm{0.0};
  double actual_arm_served_norm{0.0};
  double all_trainable_relative_decomposition_error{0.0};
  double served_relative_decomposition_error{0.0};
  double vicreg_component_trunk_relative_decomposition_error{0.0};
  double vicreg_component_head_relative_decomposition_error{0.0};
  double vicreg_component_single_graph_trunk_relative_decomposition_error{0.0};
  double vicreg_component_single_graph_head_relative_decomposition_error{0.0};
  double vicreg_component_separate_sum_trunk_relative_error{0.0};
  double vicreg_component_separate_sum_head_relative_error{0.0};
  double vicreg_inner_multiplier{0.0};
  bool vicreg_component_surface_is_channel{false};
  bool repeated_weak_views_exact{false};
  bool parameters_and_ema_exact{false};
  bool training_state_exact{false};
  bool optimizer_state_checked{false};
  bool optimizer_state_exact{false};
  double optimizer_tf_coefficient{0.0};
  int64_t vicreg_channel_active_groups{0};
  int64_t vicreg_channel_valid_rows{0};
};

struct AttributionCheckpoint {
  int64_t step{0};
  ProbeCurve probe{};
  std::array<Geometry, kChannels> geometry{};
  Geometry vicreg_clean_global_preprojector_geometry{};
  std::array<Geometry, kChannels> vicreg_clean_projected_channel_geometry{};
  torch::Tensor test_embeddings{}; // CPU float64, pairing contract only.
  GradientDiagnostic gradients{};
};

struct MaskDigest {
  uint64_t target{0xcbf29ce484222325ULL};
  uint64_t context{0xcbf29ce484222325ULL};
  int64_t target_tokens{0};
  int64_t context_tokens{0};
  int64_t hard_forbidden{0};
  int64_t soft_forbidden{0};
  int64_t relaxed_soft_forbidden{0};
};

struct WeakViewDigest {
  uint64_t view_a_data{0};
  uint64_t view_a_feature_mask{0};
  uint64_t view_b_data{0};
  uint64_t view_b_feature_mask{0};
};

struct GeneratorStateDigest {
  uint64_t cpu{0};
  uint64_t cuda{0};
};

struct GeneratorStateSnapshot {
  torch::Tensor cpu_state{};
  torch::Tensor cuda_state{};
  GeneratorStateDigest digest{};
};

[[nodiscard]] bool
generator_state_digest_equal(const GeneratorStateDigest &left,
                             const GeneratorStateDigest &right) {
  return left.cpu == right.cpu && left.cuda == right.cuda;
}

struct OuterAugmentationRetention {
  double overall{0.0};
  std::array<double, kChannels> channel{};
  double terminal{0.0};
  std::array<double, kChannels> terminal_channel{};
  int64_t clean_valid{0};
  int64_t augmented_valid{0};
  int64_t preserved{0};
  int64_t added{0};
  int64_t removed{0};
  std::array<int64_t, kChannels> clean_valid_channel{};
  std::array<int64_t, kChannels> augmented_valid_channel{};
  std::array<int64_t, kChannels> clean_valid_terminal_channel{};
  std::array<int64_t, kChannels> augmented_valid_terminal_channel{};
  std::array<int64_t, kChannels> preserved_valid_terminal_channel{};
  bool every_sample_channel_nonempty{false};
};

struct OuterAugmentationUpdate {
  int64_t seed{0};
  uint64_t clean_data_hash{0};
  uint64_t clean_mask_hash{0};
  uint64_t served_data_hash{0};
  uint64_t served_mask_hash{0};
  uint64_t replay_data_hash{0};
  uint64_t replay_mask_hash{0};
  uint64_t actual_forward_data_hash{0};
  uint64_t actual_forward_mask_hash{0};
  GeneratorStateDigest augmentation_original{};
  GeneratorStateDigest augmentation_served_consumed{};
  GeneratorStateDigest augmentation_replay_consumed{};
  GeneratorStateDigest augmentation_restored{};
  GeneratorStateDigest augmented_preview_post{};
  GeneratorStateDigest augmented_preview_replay_post{};
  GeneratorStateDigest module_forward_pre{};
  GeneratorStateDigest module_forward_post{};
  GeneratorStateSnapshot module_forward_pre_snapshot{};
  GeneratorStateSnapshot module_forward_post_snapshot{};
  mtf::jepa_context_target_mask_t clean_mask_plan{};
  torch::Tensor clean_data_cpu{};
  torch::Tensor clean_mask_cpu{};
  bool augmentation_replay_exact{false};
  bool augmentation_consumed_state_exact{false};
  bool augmentation_cuda_unchanged{false};
  bool augmentation_state_restored{false};
  bool neutral_identity_exact{false};
  bool qualified_data_changed{false};
  bool qualified_mask_exact{false};
  bool masked_values_zero{false};
  bool preview_replay_exact{false};
  bool support_counterfactual_exact{false};
  bool actual_masks_match_preview{false};
  bool actual_input_matches_served{false};
  OuterAugmentationRetention retention{};
};

struct AttributionTraining {
  std::vector<double> total_losses{};
  std::vector<double> pre_clip_gradient_norms{};
  std::vector<double> clip_factors{};
  std::vector<double> optimizer_tf_coefficients{};
  std::vector<torch::Tensor> target_masks{};  // Exact pairing assertions.
  std::vector<torch::Tensor> context_masks{}; // Exact pairing assertions.
  std::vector<std::vector<int64_t>> batch_rows{};
  std::vector<uint64_t> batch_row_hashes{};
  std::vector<WeakViewDigest> weak_view_digests{};
  std::vector<uint64_t> clean_data_hashes{};
  std::vector<uint64_t> clean_mask_hashes{};
  std::vector<GeneratorStateSnapshot> module_forward_pre_states{};
  std::vector<GeneratorStateSnapshot> module_forward_post_states{};
  std::vector<torch::Tensor> outer_view_a_feature_masks{};
  std::vector<torch::Tensor> outer_view_b_feature_masks{};
  std::vector<double> served_update_norms{};
  std::array<double, 4> component_loss_sums{};
  MaskDigest masks{};
  std::vector<OuterAugmentationUpdate> outer_augmentation_updates{};
  uint64_t model_config_fingerprint{0};
  uint64_t preprocessing_config_fingerprint{0};
};

struct AttributionArmResult {
  int64_t seed{0};
  AttributionArm arm{};
  std::vector<AttributionCheckpoint> checkpoints{};
  AttributionTraining training{};
  double initial_parameter_max_abs_diff{0.0};
  double initial_embedding_max_abs_diff{0.0};
  double initial_probe_prediction_max_abs_diff{0.0};
  double final_all_trainable_max_abs_diff{0.0};
  double final_served_max_abs_diff{0.0};
  double final_predictor_max_abs_diff{0.0};
  double final_mae_decoder_max_abs_diff{0.0};
  double final_vicreg_head_max_abs_diff{0.0};
  double final_target_ema_max_abs_diff{0.0};
};

struct ParameterSnapshot {
  std::vector<std::string> names{};
  std::vector<torch::Tensor> values{};
};

void require_finite(double value, const std::string &surface) {
  if (!std::isfinite(value)) {
    throw std::runtime_error(surface + " is non-finite");
  }
}

template <std::size_t Size>
void require_finite(const std::array<double, Size> &values,
                    const std::string &surface) {
  for (std::size_t index = 0; index < Size; ++index) {
    require_finite(values[index], surface + "[" + std::to_string(index) + "]");
  }
}

void validate_probe_curve_finite(const ProbeCurve &curve,
                                 const std::string &surface) {
  if (curve.points.empty()) {
    throw std::runtime_error(surface + " has no probe points");
  }
  require_finite(curve.area, surface + ".area");
  for (std::size_t point_index = 0; point_index < curve.points.size();
       ++point_index) {
    const auto &point = curve.points[point_index];
    const std::string point_surface =
        surface + ".point[" + std::to_string(point_index) + "]";
    if (point.samples <= 0 || !point.prediction.defined() ||
        !torch::isfinite(point.prediction).all().item<bool>()) {
      throw std::runtime_error(point_surface +
                               " prediction/sample contract failed");
    }
    require_finite(point.score.task, point_surface + ".task_score");
    require_finite(point.score.family, point_surface + ".family_score");
    require_finite(point.score.macro, point_surface + ".macro_score");
    require_finite(point.selected_alpha, point_surface + ".selected_alpha");
  }
}

void validate_geometry_finite(const Geometry &geometry,
                              const std::string &surface) {
  require_finite(geometry.effective_rank_ratio,
                 surface + ".effective_rank_ratio");
  require_finite(geometry.participation_rank_ratio,
                 surface + ".participation_rank_ratio");
  require_finite(geometry.top_eigenvalue_share,
                 surface + ".top_eigenvalue_share");
  require_finite(geometry.active_dimension_fraction,
                 surface + ".active_dimension_fraction");
}

void validate_gradient_diagnostic_finite(const GradientDiagnostic &diagnostic,
                                         const std::string &surface) {
  require_finite(diagnostic.raw_loss, surface + ".raw_loss");
  require_finite(diagnostic.served_norm, surface + ".served_norm");
  require_finite(diagnostic.tokenizer_norm, surface + ".tokenizer_norm");
  require_finite(diagnostic.encoder_norm, surface + ".encoder_norm");
  require_finite(diagnostic.predictor_norm, surface + ".predictor_norm");
  require_finite(diagnostic.mae_decoder_norm, surface + ".mae_decoder_norm");
  require_finite(diagnostic.vicreg_head_norm, surface + ".vicreg_head_norm");
  for (std::size_t index = 0; index < diagnostic.served_cosine.size();
       ++index) {
    if (std::isinf(diagnostic.served_cosine[index])) {
      throw std::runtime_error(surface + ".served_cosine[" +
                               std::to_string(index) + "] is infinite");
    }
  }
  require_finite(diagnostic.vicreg_component_raw_loss,
                 surface + ".vicreg_component_raw_loss");
  require_finite(diagnostic.vicreg_component_raw_trunk_norm,
                 surface + ".vicreg_component_raw_trunk_norm");
  require_finite(diagnostic.vicreg_component_raw_head_norm,
                 surface + ".vicreg_component_raw_head_norm");
  require_finite(diagnostic.vicreg_component_effective_weight,
                 surface + ".vicreg_component_effective_weight");
  require_finite(diagnostic.vicreg_component_effective_trunk_norm,
                 surface + ".vicreg_component_effective_trunk_norm");
  require_finite(diagnostic.vicreg_component_effective_head_norm,
                 surface + ".vicreg_component_effective_head_norm");
  require_finite(diagnostic.vicreg_view_a_variance_floor_fraction,
                 surface + ".vicreg_view_a_variance_floor_fraction");
  require_finite(diagnostic.vicreg_view_b_variance_floor_fraction,
                 surface + ".vicreg_view_b_variance_floor_fraction");
  for (std::size_t channel = 0;
       channel < diagnostic.vicreg_variance_floor_valid_rows.size();
       ++channel) {
    if (diagnostic.vicreg_variance_floor_valid_rows[channel] < 2) {
      throw std::runtime_error(surface +
                               ".vicreg_variance_floor_valid_rows is invalid");
    }
  }
  for (const auto value :
       {diagnostic.jepa_mae_norm, diagnostic.tf_vicreg_norm,
        diagnostic.canonical_total_norm, diagnostic.cancellation_ratio,
        diagnostic.tf_weighted_served_norm_ratio,
        diagnostic.actual_arm_weighted_cancellation_ratio,
        diagnostic.actual_arm_all_trainable_norm,
        diagnostic.actual_arm_served_norm,
        diagnostic.all_trainable_relative_decomposition_error,
        diagnostic.served_relative_decomposition_error,
        diagnostic.vicreg_component_trunk_relative_decomposition_error,
        diagnostic.vicreg_component_head_relative_decomposition_error,
        diagnostic
            .vicreg_component_single_graph_trunk_relative_decomposition_error,
        diagnostic
            .vicreg_component_single_graph_head_relative_decomposition_error,
        diagnostic.vicreg_component_separate_sum_trunk_relative_error,
        diagnostic.vicreg_component_separate_sum_head_relative_error,
        diagnostic.vicreg_inner_multiplier,
        diagnostic.optimizer_tf_coefficient}) {
    require_finite(value, surface + ".scalar");
  }
  if (!diagnostic.repeated_weak_views_exact ||
      !diagnostic.parameters_and_ema_exact ||
      !diagnostic.training_state_exact || !diagnostic.optimizer_state_exact) {
    throw std::runtime_error(surface + " neutrality contract failed");
  }
}

void validate_attribution_checkpoint_finite(
    const AttributionCheckpoint &checkpoint, const std::string &surface) {
  validate_probe_curve_finite(checkpoint.probe, surface + ".probe");
  if (!checkpoint.test_embeddings.defined() ||
      !torch::isfinite(checkpoint.test_embeddings).all().item<bool>()) {
    throw std::runtime_error(surface + ".test_embeddings is non-finite");
  }
  for (std::size_t channel = 0; channel < checkpoint.geometry.size();
       ++channel) {
    validate_geometry_finite(checkpoint.geometry[channel],
                             surface + ".served_geometry[" +
                                 std::to_string(channel) + "]");
    validate_geometry_finite(
        checkpoint.vicreg_clean_projected_channel_geometry[channel],
        surface + ".projected_geometry[" + std::to_string(channel) + "]");
  }
  validate_geometry_finite(checkpoint.vicreg_clean_global_preprojector_geometry,
                           surface + ".global_preprojector_geometry");
  validate_gradient_diagnostic_finite(checkpoint.gradients,
                                      surface + ".gradient");
}

void validate_attribution_training_finite(const AttributionTraining &training,
                                          const std::string &surface) {
  for (const auto &entry :
       {std::pair<const std::vector<double> *, const char *>{
            &training.total_losses, "total_losses"},
        {&training.pre_clip_gradient_norms, "pre_clip_gradient_norms"},
        {&training.clip_factors, "clip_factors"},
        {&training.optimizer_tf_coefficients, "optimizer_tf_coefficients"},
        {&training.served_update_norms, "served_update_norms"}}) {
    for (const auto value : *entry.first) {
      require_finite(value, surface + "." + entry.second);
    }
  }
  require_finite(training.component_loss_sums,
                 surface + ".component_loss_sums");
}

[[nodiscard]] int64_t paired_step_seed(int64_t model_seed, int64_t step) {
  const auto mixed =
      splitmix64(0x6f626a5f6d61736bULL ^ static_cast<uint64_t>(model_seed) ^
                 (static_cast<uint64_t>(step) << 32U));
  return static_cast<int64_t>(mixed & 0x7fffffffffffffffULL);
}

[[nodiscard]] int64_t paired_diagnostic_seed(int64_t model_seed) {
  const auto mixed =
      splitmix64(0x646961675f6d6173ULL ^ static_cast<uint64_t>(model_seed));
  return static_cast<int64_t>(mixed & 0x7fffffffffffffffULL);
}

[[nodiscard]] int64_t outer_augmentation_seed(int64_t model_seed,
                                              int64_t zero_based_update) {
  const auto mixed = splitmix64(
      kOuterAugmentationSeedDomain ^ static_cast<uint64_t>(model_seed) ^
      (static_cast<uint64_t>(zero_based_update) << 32U));
  return static_cast<int64_t>(mixed & 0x7fffffffffffffffULL);
}

void validate_outer_augmentation_seed_domain() {
  std::set<int64_t> augmentation_seeds;
  std::set<int64_t> forbidden_seeds;
  for (const auto model_seed : kAttributionSeeds) {
    forbidden_seeds.insert(model_seed);
    forbidden_seeds.insert(paired_diagnostic_seed(model_seed));
    for (int64_t update = 0; update < kAttributionSteps; ++update) {
      forbidden_seeds.insert(paired_step_seed(model_seed, update));
    }
  }
  for (const auto model_seed : kAttributionSeeds) {
    for (int64_t update = 0; update < kAttributionSteps; ++update) {
      const auto seed = outer_augmentation_seed(model_seed, update);
      if (!augmentation_seeds.insert(seed).second ||
          forbidden_seeds.count(seed) != 0) {
        throw std::runtime_error(
            "outer augmentation seed uniqueness/domain contract failed");
      }
    }
  }
  if (augmentation_seeds.size() !=
      kAttributionSeeds.size() * static_cast<std::size_t>(kAttributionSteps)) {
    throw std::runtime_error("outer augmentation seed count contract failed");
  }
}

void set_paired_rng(int64_t seed, const torch::Device &device) {
  torch::manual_seed(seed);
  if (device.is_cuda()) {
    torch::cuda::manual_seed_all(seed);
  }
}

class DefaultGeneratorStateGuard {
public:
  explicit DefaultGeneratorStateGuard(const torch::Device &device)
      : device_(device),
        cpu_state_(at::detail::getDefaultCPUGenerator().get_state().clone()) {
    if (device_.is_cuda()) {
      cuda_state_ = at::cuda::detail::getDefaultCUDAGenerator(device_.index())
                        .get_state()
                        .clone();
    }
  }

  DefaultGeneratorStateGuard(const DefaultGeneratorStateGuard &) = delete;
  DefaultGeneratorStateGuard &
  operator=(const DefaultGeneratorStateGuard &) = delete;

  ~DefaultGeneratorStateGuard() noexcept {
    if (restored_) {
      return;
    }
    try {
      restore();
    } catch (...) {
    }
  }

  void restore() {
    auto cpu_generator = at::detail::getDefaultCPUGenerator();
    cpu_generator.set_state(cpu_state_);
    const bool cpu_exact = torch::equal(cpu_generator.get_state(), cpu_state_);
    bool cuda_exact = true;
    if (device_.is_cuda()) {
      auto generator =
          at::cuda::detail::getDefaultCUDAGenerator(device_.index());
      generator.set_state(cuda_state_);
      cuda_exact = torch::equal(generator.get_state(), cuda_state_);
    }
    if (!cpu_exact || !cuda_exact) {
      throw std::runtime_error(
          "attribution diagnostic generator-state restoration failed");
    }
    restored_ = true;
  }

private:
  torch::Device device_;
  torch::Tensor cpu_state_{};
  torch::Tensor cuda_state_{};
  bool restored_{false};
};

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t
attribution_config(const torch::Device &device, const AttributionArm &arm) {
  auto config = active_config(device, /*weak_views=*/true);
  config.lambda_jepa = arm.lambda_jepa;
  config.lambda_mae = arm.lambda_mae;
  config.lambda_tf_align = arm.lambda_tf_align;
  config.lambda_vicreg = arm.lambda_vicreg;
  config.vicreg_var_weight = arm.vicreg_var_weight;
  config.max_context_target_time_overlap = arm.max_context_target_time_overlap;
  config.use_global_vicreg = !arm.projected_channel_stratified_vicreg;
  config.use_channel_vicreg = arm.projected_channel_stratified_vicreg;
  config.stratify_channel_vicreg_by_channel =
      arm.projected_channel_stratified_vicreg;
  config.lambda_channel_vicreg =
      arm.projected_channel_stratified_vicreg ? 0.25 : 1.0;
  config.return_vicreg_debug_tensors = true;
  // Branches stay enabled even when their gradient coefficient is zero. This
  // preserves execution and the module-owned random draw schedule.
  config.use_jepa_loss = true;
  config.use_mae_decoder = true;
  config.use_tf_align_loss = true;
  config.use_vicreg_loss = true;
  return config;
}

void validate_attribution_arm_configs(const torch::Device &device,
                                      const std::vector<AttributionArm> &arms) {
  for (const auto &arm : arms) {
    const auto config = attribution_config(device, arm);
    const bool stratified = arm.projected_channel_stratified_vicreg;
    if (config.use_global_vicreg == stratified ||
        config.use_channel_vicreg != stratified ||
        config.stratify_channel_vicreg_by_channel != stratified ||
        config.lambda_channel_vicreg != (stratified ? 0.25 : 1.0) ||
        config.vicreg_var_weight != arm.vicreg_var_weight ||
        (arm.vicreg_var_weight != 0.0 && arm.vicreg_var_weight != 25.0) ||
        (arm.vicreg_var_weight == 0.0 &&
         (!stratified || arm.lambda_vicreg != 0.05)) ||
        !config.return_vicreg_debug_tensors) {
      throw std::runtime_error("attribution projected channel-stratified "
                               "VICReg arm contract failed");
    }
  }
}

[[nodiscard]] std::string
canonical_config_manifest(const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::boolalpha << std::defaultfloat << std::setprecision(17);
  const auto add = [&out](const char *name, const auto &value) {
    out << name << '=' << value << '\n';
  };
  const auto add_vector = [&out](const char *name,
                                 const std::vector<int64_t> &values) {
    out << name << '=';
    for (std::size_t index = 0; index < values.size(); ++index) {
      if (index != 0) {
        out << ',';
      }
      out << values[index];
    }
    out << '\n';
  };
  add("channel_count", config.channel_count);
  add("history_length", config.history_length);
  add("input_width", config.input_width);
  add("d_model", config.d_model);
  add("latent_dim", config.latent_dim);
  add("projector_dim", config.projector_dim);
  add("predictor_hidden_dim", config.predictor_hidden_dim);
  add("num_encoder_layers", config.num_encoder_layers);
  add("num_predictor_layers", config.num_predictor_layers);
  add("num_decoder_layers", config.num_decoder_layers);
  add("num_heads", config.num_heads);
  add("dropout", config.dropout);
  add_vector("time_scales", config.time_scales);
  add_vector("scale_strides", config.scale_strides);
  add("use_frequency_tokens", config.use_frequency_tokens);
  add("frequency_num_bins", config.frequency_num_bins);
  add("frequency_log_magnitude", config.frequency_log_magnitude);
  add("serving_pool_policy",
      mtf::mtf_serving_pool_policy_name(config.serving_pool_policy));
  add("mask_ratio_time", config.mask_ratio_time);
  add("mask_ratio_frequency", config.mask_ratio_frequency);
  add("mask_ratio_channel", config.mask_ratio_channel);
  add("min_context_ratio", config.min_context_ratio);
  if (config.jepa_mask_policy !=
      mtf::mtf_jepa_mask_policy_t::legacy_soft_overlap) {
    add("jepa_mask_policy",
        mtf::mtf_jepa_mask_policy_name(config.jepa_mask_policy));
  }
  add("lambda_jepa", config.lambda_jepa);
  add("lambda_mae", config.lambda_mae);
  add("lambda_tf_align", config.lambda_tf_align);
  add("lambda_vicreg", config.lambda_vicreg);
  add("vicreg_sim_weight", config.vicreg_sim_weight);
  add("vicreg_var_weight", config.vicreg_var_weight);
  add("vicreg_cov_weight", config.vicreg_cov_weight);
  add("vicreg_variance_floor", config.vicreg_variance_floor);
  add("vicreg_variance_epsilon", config.vicreg_variance_epsilon);
  add("target_ema_tau", config.target_ema_tau);
  add("use_target_ema", config.use_target_ema);
  add("stop_gradient_target", config.stop_gradient_target);
  add("return_diagnostics", config.return_diagnostics);
  add("return_vicreg_debug_tensors", config.return_vicreg_debug_tensors);
  add("use_mae_decoder", config.use_mae_decoder);
  add("use_jepa_loss", config.use_jepa_loss);
  add("use_tf_align_loss", config.use_tf_align_loss);
  add("use_vicreg_loss", config.use_vicreg_loss);
  add("use_global_vicreg", config.use_global_vicreg);
  add("use_channel_vicreg", config.use_channel_vicreg);
  add("stratify_channel_vicreg_by_channel",
      config.stratify_channel_vicreg_by_channel);
  add("lambda_global_vicreg", config.lambda_global_vicreg);
  add("lambda_channel_vicreg", config.lambda_channel_vicreg);
  add("use_raw_reconstruction_targets", config.use_raw_reconstruction_targets);
  add("strict_finite_loss", config.strict_finite_loss);
  add("couple_time_frequency_masks", config.couple_time_frequency_masks);
  add("mask_same_window_across_domains",
      config.mask_same_window_across_domains);
  add("mask_same_channel_block", config.mask_same_channel_block);
  add("max_context_target_time_overlap",
      config.max_context_target_time_overlap);
  add("vicreg_view_gaussian_jitter_std",
      config.vicreg_view_gaussian_jitter_std);
  add("vicreg_view_time_dropout_scale", config.vicreg_view_time_dropout_scale);
  if (config.vicreg_view_pairing_policy !=
      mtf::mtf_vicreg_view_pairing_policy_t::independent_weak) {
    add("vicreg_view_pairing_policy",
        mtf::mtf_vicreg_view_pairing_policy_name(
            config.vicreg_view_pairing_policy));
  }
  add("augmentation_profile", config.augmentation_profile);
  add("gaussian_jitter_std", config.gaussian_jitter_std);
  add("feature_dropout_prob", config.feature_dropout_prob);
  add("history_dropout_prob", config.history_dropout_prob);
  add("time_crop_jitter_max", config.time_crop_jitter_max);
  add("time_dilation_min", config.time_dilation_min);
  add("time_dilation_max", config.time_dilation_max);
  add("time_warp_max", config.time_warp_max);
  add("amplitude_scale_min", config.amplitude_scale_min);
  add("amplitude_scale_max", config.amplitude_scale_max);
  add("amplitude_shift_std", config.amplitude_shift_std);
  add("frequency_mask_ratio", config.frequency_mask_ratio);
  add("frequency_jitter_std", config.frequency_jitter_std);
  add("phase_jitter_max", config.phase_jitter_max);
  add("channel_dropout_prob", config.channel_dropout_prob);
  add("cross_channel_dropout_prob", config.cross_channel_dropout_prob);
  add("node_dropout_prob", config.node_dropout_prob);
  add("edge_dropout_prob", config.edge_dropout_prob);
  add("magnitude_normalization_noise_std",
      config.magnitude_normalization_noise_std);
  if (config.dtype != torch::kFloat32) {
    throw std::runtime_error("outer augmentation model dtype is not float32");
  }
  add("dtype", "float32");
  if (!config.device.is_cuda() || config.device.index() != 0) {
    throw std::runtime_error("outer augmentation model device is not cuda:0");
  }
  add("device", "cuda:0");
  return out.str();
}

[[nodiscard]] std::string
jmcd_common_config_manifest(const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  std::istringstream input(canonical_config_manifest(config));
  std::ostringstream output;
  std::string line;
  while (std::getline(input, line)) {
    if (line.rfind("lambda_jepa=", 0) == 0 ||
        line.rfind("lambda_mae=", 0) == 0) {
      continue;
    }
    output << line << '\n';
  }
  return output.str();
}

void validate_jmcd_arm_table(const torch::Device &device) {
  constexpr std::array<std::pair<double, double>, 4> expected{
      {{1.0, 0.25}, {1.0, 0.0}, {0.0, 0.25}, {0.0, 0.0}}};
  std::string common_manifest;
  std::set<std::string> full_manifests;
  for (std::size_t index = 0; index < kJmcdArms.size(); ++index) {
    const auto &arm = kJmcdArms[index];
    const auto config = attribution_config(device, arm);
    const bool exact = arm.lambda_jepa == expected[index].first &&
                       arm.lambda_mae == expected[index].second &&
                       arm.lambda_tf_align == 0.0 && arm.lambda_vicreg == 0.0 &&
                       arm.max_context_target_time_overlap == 0.50 &&
                       !arm.tf_gradient_matched_warmup &&
                       !arm.projected_channel_stratified_vicreg &&
                       arm.vicreg_var_weight == 25.0 && config.use_jepa_loss &&
                       config.use_mae_decoder && config.use_tf_align_loss &&
                       config.use_vicreg_loss;
    if (!exact) {
      throw std::runtime_error("JMCD frozen arm table failed");
    }
    const auto full = canonical_config_manifest(config);
    const auto common = jmcd_common_config_manifest(config);
    full_manifests.insert(full);
    if (index == 0) {
      common_manifest = common;
    } else if (common != common_manifest) {
      throw std::runtime_error(
          "JMCD configuration changed outside JEPA/MAE weights");
    }
  }
  if (full_manifests.size() != kJmcdArms.size()) {
    throw std::runtime_error("JMCD objective configurations are not distinct");
  }
}

[[nodiscard]] std::string canonical_preprocessing_manifest(
    const mtf::mtf_jepa_mae_vicreg_config_t &config) {
  std::ostringstream out;
  out.imbue(std::locale::classic());
  out << std::boolalpha << std::defaultfloat << std::setprecision(17);
  const auto add = [&out](const char *name, const auto &value) {
    out << name << '=' << value << '\n';
  };
  add("augmentation_profile", config.augmentation_profile);
  add("gaussian_jitter_std", config.gaussian_jitter_std);
  add("feature_dropout_prob", config.feature_dropout_prob);
  add("history_dropout_prob", config.history_dropout_prob);
  add("time_crop_jitter_max", config.time_crop_jitter_max);
  add("time_dilation_min", config.time_dilation_min);
  add("time_dilation_max", config.time_dilation_max);
  add("time_warp_max", config.time_warp_max);
  add("amplitude_scale_min", config.amplitude_scale_min);
  add("amplitude_scale_max", config.amplitude_scale_max);
  add("amplitude_shift_std", config.amplitude_shift_std);
  add("frequency_mask_ratio", config.frequency_mask_ratio);
  add("frequency_jitter_std", config.frequency_jitter_std);
  add("phase_jitter_max", config.phase_jitter_max);
  add("channel_dropout_prob", config.channel_dropout_prob);
  add("cross_channel_dropout_prob", config.cross_channel_dropout_prob);
  add("node_dropout_prob", config.node_dropout_prob);
  add("edge_dropout_prob", config.edge_dropout_prob);
  add("magnitude_normalization_noise_std",
      config.magnitude_normalization_noise_std);
  return out.str();
}

[[nodiscard]] uint64_t fnv1a64(const std::string &bytes) {
  uint64_t hash = 14695981039346656037ULL;
  for (const unsigned char byte : bytes) {
    hash ^= static_cast<uint64_t>(byte);
    hash *= 1099511628211ULL;
  }
  return hash;
}

void copy_outer_preprocessing_fields(
    mtf::mtf_jepa_mae_vicreg_config_t &destination,
    const mtf::mtf_jepa_mae_vicreg_config_t &source) {
  destination.augmentation_profile = source.augmentation_profile;
  destination.gaussian_jitter_std = source.gaussian_jitter_std;
  destination.feature_dropout_prob = source.feature_dropout_prob;
  destination.history_dropout_prob = source.history_dropout_prob;
  destination.time_crop_jitter_max = source.time_crop_jitter_max;
  destination.time_dilation_min = source.time_dilation_min;
  destination.time_dilation_max = source.time_dilation_max;
  destination.time_warp_max = source.time_warp_max;
  destination.amplitude_scale_min = source.amplitude_scale_min;
  destination.amplitude_scale_max = source.amplitude_scale_max;
  destination.amplitude_shift_std = source.amplitude_shift_std;
  destination.frequency_mask_ratio = source.frequency_mask_ratio;
  destination.frequency_jitter_std = source.frequency_jitter_std;
  destination.phase_jitter_max = source.phase_jitter_max;
  destination.channel_dropout_prob = source.channel_dropout_prob;
  destination.cross_channel_dropout_prob = source.cross_channel_dropout_prob;
  destination.node_dropout_prob = source.node_dropout_prob;
  destination.edge_dropout_prob = source.edge_dropout_prob;
  destination.magnitude_normalization_noise_std =
      source.magnitude_normalization_noise_std;
}

[[nodiscard]] mtf::mtf_jepa_mae_vicreg_config_t
outer_preprocessing_config(const torch::Device &device, std::size_t arm_index) {
  if (arm_index >= kOuterAugmentationArms.size()) {
    throw std::runtime_error("outer augmentation arm index is invalid");
  }
  auto config = attribution_config(device, kOuterAugmentationArms[arm_index]);
  config.feature_dropout_prob = 0.0;
  config.history_dropout_prob = 0.0;
  config.time_crop_jitter_max = 0;
  config.amplitude_shift_std = 0.0;
  config.phase_jitter_max = 0.0;
  config.channel_dropout_prob = 0.0;
  config.cross_channel_dropout_prob = 0.0;
  config.node_dropout_prob = 0.0;
  config.edge_dropout_prob = 0.0;
  config.magnitude_normalization_noise_std = 0.0;
  if (arm_index == kOuterNeutralIndex) {
    config.augmentation_profile = "no_input_augmentation_v1";
    config.gaussian_jitter_std = 0.0;
    config.time_dilation_min = 1.0;
    config.time_dilation_max = 1.0;
    config.time_warp_max = 0.0;
    config.amplitude_scale_min = 1.0;
    config.amplitude_scale_max = 1.0;
    config.frequency_mask_ratio = 0.0;
    config.frequency_jitter_std = 0.0;
  } else if (arm_index == kOuterFullActiveIndex) {
    config.augmentation_profile = "light_phase_safe_v2";
    config.gaussian_jitter_std = 0.001;
    config.time_dilation_min = 0.98;
    config.time_dilation_max = 1.02;
    config.time_warp_max = 0.01;
    config.amplitude_scale_min = 0.98;
    config.amplitude_scale_max = 1.02;
    config.frequency_mask_ratio = 0.02;
    config.frequency_jitter_std = 0.01;
  } else {
    config.augmentation_profile = "attribution_candidate_safe_stack";
    config.gaussian_jitter_std = 0.001;
    config.time_dilation_min = 1.0;
    config.time_dilation_max = 1.0;
    config.time_warp_max = 0.0;
    config.amplitude_scale_min = 0.98;
    config.amplitude_scale_max = 1.02;
    config.frequency_mask_ratio = 0.0;
    config.frequency_jitter_std = 0.01;
  }
  return config;
}

void validate_outer_augmentation_configs(const torch::Device &device) {
  const auto model_reference =
      attribution_config(device, kOuterAugmentationArms.front());
  const auto model_manifest = canonical_config_manifest(model_reference);
  for (std::size_t arm_index = 0; arm_index < kOuterAugmentationArms.size();
       ++arm_index) {
    const auto model_config =
        attribution_config(device, kOuterAugmentationArms[arm_index]);
    if (canonical_config_manifest(model_config) != model_manifest) {
      throw std::runtime_error(
          "outer augmentation model configurations are not exact");
    }
    const auto preprocessing = outer_preprocessing_config(device, arm_index);
    auto normalized = preprocessing;
    copy_outer_preprocessing_fields(normalized, model_reference);
    if (canonical_config_manifest(normalized) != model_manifest) {
      throw std::runtime_error(
          "outer preprocessing changed a non-outer model field");
    }
  }
}

[[nodiscard]] ParameterSnapshot
snapshot_parameters(const mtf::MtfJepaMaeVicreg &model) {
  ParameterSnapshot result{};
  torch::NoGradGuard no_grad;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    result.names.push_back(item.key());
    result.values.push_back(
        item.value().detach().to(torch::kCPU, torch::kFloat64).clone());
  }
  return result;
}

[[nodiscard]] double
parameter_max_abs_diff(const mtf::MtfJepaMaeVicreg &model,
                       const ParameterSnapshot &reference) {
  const auto parameters = model->named_parameters(/*recurse=*/true);
  if (parameters.size() != reference.values.size()) {
    throw std::runtime_error("attribution initial parameter count mismatch");
  }
  double maximum = 0.0;
  std::size_t index = 0;
  torch::NoGradGuard no_grad;
  for (const auto &item : parameters) {
    if (item.key() != reference.names[index] ||
        item.value().sizes() != reference.values[index].sizes()) {
      throw std::runtime_error("attribution initial parameter layout mismatch");
    }
    maximum = std::max(maximum,
                       (item.value().detach().to(torch::kCPU, torch::kFloat64) -
                        reference.values[index])
                           .abs()
                           .max()
                           .item<double>());
    ++index;
  }
  return maximum;
}

enum class ParameterDeltaPartition {
  all_trainable,
  served,
  predictor,
  mae_decoder,
  vicreg_head,
  target_ema,
};

[[nodiscard]] double
parameter_partition_max_abs_diff(const mtf::MtfJepaMaeVicreg &model,
                                 const ParameterSnapshot &reference,
                                 ParameterDeltaPartition partition) {
  const auto parameters = model->named_parameters(/*recurse=*/true);
  if (parameters.size() != reference.values.size()) {
    throw std::runtime_error("JMCD parameter count mismatch");
  }
  double maximum = 0.0;
  std::size_t included = 0;
  std::size_t index = 0;
  torch::NoGradGuard no_grad;
  for (const auto &item : parameters) {
    if (item.key() != reference.names[index] ||
        item.value().sizes() != reference.values[index].sizes()) {
      throw std::runtime_error("JMCD parameter layout mismatch");
    }
    const auto &name = item.key();
    const bool target = name.rfind("target_tokenizer.", 0) == 0 ||
                        name.rfind("target_encoder.", 0) == 0;
    const bool served =
        name.rfind("tokenizer.", 0) == 0 || name.rfind("encoder.", 0) == 0;
    const bool include =
        (partition == ParameterDeltaPartition::all_trainable &&
         item.value().requires_grad()) ||
        (partition == ParameterDeltaPartition::served && served) ||
        (partition == ParameterDeltaPartition::predictor &&
         name.rfind("predictor.", 0) == 0) ||
        (partition == ParameterDeltaPartition::mae_decoder &&
         name.rfind("mae_decoder.", 0) == 0) ||
        (partition == ParameterDeltaPartition::vicreg_head &&
         name.rfind("vicreg_stability_head.", 0) == 0) ||
        (partition == ParameterDeltaPartition::target_ema && target);
    if (include) {
      maximum = std::max(
          maximum, (item.value().detach().to(torch::kCPU, torch::kFloat64) -
                    reference.values[index])
                       .abs()
                       .max()
                       .item<double>());
      ++included;
    }
    ++index;
  }
  if (included == 0) {
    throw std::runtime_error("JMCD parameter partition is empty");
  }
  return maximum;
}

[[nodiscard]] uint64_t hash_boolean_tensor(const torch::Tensor &input) {
  const auto bytes = input.to(torch::kCPU, torch::kUInt8).contiguous().view(-1);
  const auto *data = bytes.data_ptr<uint8_t>();
  uint64_t hash = 0xcbf29ce484222325ULL;
  hash ^= static_cast<uint64_t>(input.dim());
  hash *= 0x100000001b3ULL;
  for (const auto size : input.sizes()) {
    hash ^= static_cast<uint64_t>(size);
    hash *= 0x100000001b3ULL;
  }
  for (int64_t index = 0; index < bytes.numel(); ++index) {
    hash ^= static_cast<uint64_t>(data[index]);
    hash *= 0x100000001b3ULL;
  }
  return hash;
}

void mix_hash_value(uint64_t &hash, uint64_t value) {
  hash ^= value;
  hash *= 0x100000001b3ULL;
}

[[nodiscard]] uint64_t hash_tensor_stable_bytes(const torch::Tensor &input) {
  if (!input.defined()) {
    throw std::runtime_error("attribution weak-view debug tensor is undefined");
  }
  const auto contiguous = input.detach().to(torch::kCPU).contiguous();
  uint64_t hash = 0xcbf29ce484222325ULL;
  mix_hash_value(hash, static_cast<uint64_t>(contiguous.scalar_type()));
  mix_hash_value(hash, static_cast<uint64_t>(contiguous.dim()));
  for (const auto size : contiguous.sizes()) {
    mix_hash_value(hash, static_cast<uint64_t>(size));
  }
  const auto byte_count = static_cast<std::size_t>(contiguous.numel()) *
                          static_cast<std::size_t>(contiguous.element_size());
  const auto *bytes = static_cast<const uint8_t *>(contiguous.data_ptr());
  for (std::size_t index = 0; index < byte_count; ++index) {
    mix_hash_value(hash, static_cast<uint64_t>(bytes[index]));
  }
  return hash;
}

[[nodiscard]] GeneratorStateDigest
current_generator_state_digest(const torch::Device &device) {
  GeneratorStateDigest digest{};
  digest.cpu = hash_tensor_stable_bytes(
      at::detail::getDefaultCPUGenerator().get_state());
  if (device.is_cuda()) {
    digest.cuda = hash_tensor_stable_bytes(
        at::cuda::detail::getDefaultCUDAGenerator(device.index()).get_state());
  }
  return digest;
}

[[nodiscard]] GeneratorStateSnapshot
current_generator_state_snapshot(const torch::Device &device) {
  GeneratorStateSnapshot snapshot{};
  snapshot.cpu_state = at::detail::getDefaultCPUGenerator().get_state().clone();
  snapshot.digest.cpu = hash_tensor_stable_bytes(snapshot.cpu_state);
  if (device.is_cuda()) {
    snapshot.cuda_state =
        at::cuda::detail::getDefaultCUDAGenerator(device.index())
            .get_state()
            .clone();
    snapshot.digest.cuda = hash_tensor_stable_bytes(snapshot.cuda_state);
  }
  return snapshot;
}

[[nodiscard]] bool
generator_state_snapshot_equal(const GeneratorStateSnapshot &left,
                               const GeneratorStateSnapshot &right) {
  const auto tensor_equal = [](const torch::Tensor &lhs,
                               const torch::Tensor &rhs) {
    if (lhs.defined() != rhs.defined()) {
      return false;
    }
    return !lhs.defined() || torch::equal(lhs, rhs);
  };
  return tensor_equal(left.cpu_state, right.cpu_state) &&
         tensor_equal(left.cuda_state, right.cuda_state);
}

class OuterAugmentationGeneratorGuard {
public:
  explicit OuterAugmentationGeneratorGuard(const torch::Device &device)
      : device_(device),
        cpu_state_(at::detail::getDefaultCPUGenerator().get_state().clone()) {
    if (device_.is_cuda()) {
      cuda_state_ = at::cuda::detail::getDefaultCUDAGenerator(device_.index())
                        .get_state()
                        .clone();
    }
  }

  OuterAugmentationGeneratorGuard(const OuterAugmentationGeneratorGuard &) =
      delete;
  OuterAugmentationGeneratorGuard &
  operator=(const OuterAugmentationGeneratorGuard &) = delete;

  ~OuterAugmentationGeneratorGuard() noexcept {
    if (!restored_) {
      try {
        restore();
      } catch (...) {
      }
    }
  }

  void seed_cpu(int64_t seed) {
    auto cpu_generator = at::detail::getDefaultCPUGenerator();
    cpu_generator.set_current_seed(static_cast<uint64_t>(seed));
  }

  [[nodiscard]] const torch::Tensor &original_cpu_state() const {
    return cpu_state_;
  }

  [[nodiscard]] const torch::Tensor &original_cuda_state() const {
    return cuda_state_;
  }

  void restore() {
    auto cpu_generator = at::detail::getDefaultCPUGenerator();
    cpu_generator.set_state(cpu_state_);
    bool exact = torch::equal(cpu_generator.get_state(), cpu_state_);
    if (device_.is_cuda()) {
      auto cuda_generator =
          at::cuda::detail::getDefaultCUDAGenerator(device_.index());
      cuda_generator.set_state(cuda_state_);
      exact = exact && torch::equal(cuda_generator.get_state(), cuda_state_);
    }
    if (!exact) {
      throw std::runtime_error(
          "outer augmentation generator-state restoration failed");
    }
    restored_ = true;
  }

private:
  torch::Device device_;
  torch::Tensor cpu_state_{};
  torch::Tensor cuda_state_{};
  bool restored_{false};
};

[[nodiscard]] bool
jepa_masks_exact(const mtf::jepa_context_target_mask_t &left,
                 const mtf::jepa_context_target_mask_t &right) {
  return torch::equal(left.target_mask, right.target_mask) &&
         torch::equal(left.context_mask, right.context_mask) &&
         torch::equal(left.valid_mask, right.valid_mask) &&
         torch::equal(left.mask_ratio_actual, right.mask_ratio_actual) &&
         left.num_target_tokens == right.num_target_tokens &&
         left.num_context_tokens == right.num_context_tokens &&
         left.hard_forbidden_count == right.hard_forbidden_count &&
         left.soft_forbidden_count == right.soft_forbidden_count &&
         left.relaxed_soft_forbidden_count ==
             right.relaxed_soft_forbidden_count &&
         left.reduced_targets_for_context_count ==
             right.reduced_targets_for_context_count &&
         left.context_starved_sample_count ==
             right.context_starved_sample_count &&
         left.min_context_satisfied_count == right.min_context_satisfied_count;
}

[[nodiscard]] mtf::jepa_context_target_mask_t
clone_mask_plan_cpu(const mtf::jepa_context_target_mask_t &source) {
  mtf::jepa_context_target_mask_t clone = source;
  clone.context_mask = source.context_mask.to(torch::kCPU).clone();
  clone.target_mask = source.target_mask.to(torch::kCPU).clone();
  clone.valid_mask = source.valid_mask.to(torch::kCPU).clone();
  clone.mask_ratio_actual = source.mask_ratio_actual.to(torch::kCPU).clone();
  return clone;
}

struct MaskPreviewWithState {
  mtf::jepa_context_target_mask_t masks{};
  GeneratorStateSnapshot post_state{};
};

[[nodiscard]] MaskPreviewWithState
preview_masks_with_state(mtf::MtfJepaMaeVicreg &model,
                         const torch::Tensor &data,
                         const torch::Tensor &feature_mask, int64_t seed,
                         const torch::Device &device) {
  set_paired_rng(seed, device);
  torch::NoGradGuard no_grad;
  const auto tokens = model->tokenize(data, feature_mask);
  MaskPreviewWithState result{};
  result.masks = model->create_masks(tokens);
  result.post_state = current_generator_state_snapshot(device);
  return result;
}

[[nodiscard]] OuterAugmentationRetention
outer_augmentation_retention(const torch::Tensor &clean_mask_input,
                             const torch::Tensor &augmented_mask_input) {
  const auto clean =
      clean_mask_input.to(torch::kCPU, torch::kBool).contiguous();
  const auto augmented =
      augmented_mask_input.to(torch::kCPU, torch::kBool).contiguous();
  if (clean.sizes() != augmented.sizes() || clean.dim() != 4 ||
      clean.size(1) != kChannels || clean.size(2) != kHistory) {
    throw std::runtime_error("outer augmentation retention shape failed");
  }
  OuterAugmentationRetention result{};
  result.clean_valid = clean.sum().item<int64_t>();
  result.augmented_valid = augmented.sum().item<int64_t>();
  result.preserved = clean.logical_and(augmented).sum().item<int64_t>();
  result.added =
      clean.logical_not().logical_and(augmented).sum().item<int64_t>();
  result.removed =
      clean.logical_and(augmented.logical_not()).sum().item<int64_t>();
  if (result.clean_valid <= 0) {
    throw std::runtime_error(
        "outer augmentation overall retention denominator failed");
  }
  result.overall = static_cast<double>(result.augmented_valid) /
                   static_cast<double>(result.clean_valid);
  int64_t terminal_clean_total = 0;
  int64_t terminal_preserved_total = 0;
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    const auto index = static_cast<std::size_t>(channel);
    const auto clean_channel = clean.select(1, channel);
    const auto augmented_channel = augmented.select(1, channel);
    result.clean_valid_channel[index] = clean_channel.sum().item<int64_t>();
    result.augmented_valid_channel[index] =
        augmented_channel.sum().item<int64_t>();
    if (result.clean_valid_channel[index] <= 0) {
      throw std::runtime_error(
          "outer augmentation channel retention denominator failed");
    }
    result.channel[index] =
        static_cast<double>(result.augmented_valid_channel[index]) /
        static_cast<double>(result.clean_valid_channel[index]);
    const auto clean_terminal = clean_channel.select(1, kHistory - 1);
    const auto augmented_terminal = augmented_channel.select(1, kHistory - 1);
    result.clean_valid_terminal_channel[index] =
        clean_terminal.sum().item<int64_t>();
    result.augmented_valid_terminal_channel[index] =
        augmented_terminal.sum().item<int64_t>();
    result.preserved_valid_terminal_channel[index] =
        clean_terminal.logical_and(augmented_terminal).sum().item<int64_t>();
    if (result.clean_valid_terminal_channel[index] <= 0) {
      throw std::runtime_error(
          "outer augmentation terminal retention denominator failed");
    }
    result.terminal_channel[index] =
        static_cast<double>(result.preserved_valid_terminal_channel[index]) /
        static_cast<double>(result.clean_valid_terminal_channel[index]);
    terminal_clean_total += result.clean_valid_terminal_channel[index];
    terminal_preserved_total += result.preserved_valid_terminal_channel[index];
  }
  if (terminal_clean_total <= 0) {
    throw std::runtime_error(
        "outer augmentation overall terminal denominator failed");
  }
  result.terminal = static_cast<double>(terminal_preserved_total) /
                    static_cast<double>(terminal_clean_total);
  result.every_sample_channel_nonempty =
      augmented.flatten(/*start_dim=*/2).any(/*dim=*/2).all().item<bool>();
  return result;
}

struct ReplayedOuterAugmentation {
  mtf::mtf_input_t served{};
  OuterAugmentationUpdate diagnostic{};
};

[[nodiscard]] ReplayedOuterAugmentation apply_outer_augmentation_replayed(
    const torch::Tensor &clean_data, const torch::Tensor &clean_mask,
    const mtf::mtf_jepa_mae_vicreg_config_t &preprocessing_config,
    int64_t augmentation_seed_value, const torch::Device &model_device) {
  if (!clean_data.device().is_cpu() || !clean_mask.device().is_cpu()) {
    throw std::runtime_error("outer augmentation input is not CPU");
  }
  ReplayedOuterAugmentation result{};
  auto &diagnostic = result.diagnostic;
  diagnostic.seed = augmentation_seed_value;
  diagnostic.clean_data_hash = hash_tensor_stable_bytes(clean_data);
  diagnostic.clean_mask_hash = hash_tensor_stable_bytes(clean_mask);
  OuterAugmentationGeneratorGuard guard(model_device);
  diagnostic.augmentation_original =
      current_generator_state_digest(model_device);

  guard.seed_cpu(augmentation_seed_value);
  result.served = launcher_augmentation::apply_mtf_training_augmentations(
      mtf::mtf_input_t{clean_data, clean_mask}, preprocessing_config);
  const auto served_cpu_state =
      at::detail::getDefaultCPUGenerator().get_state().clone();
  torch::Tensor served_cuda_state{};
  if (model_device.is_cuda()) {
    served_cuda_state =
        at::cuda::detail::getDefaultCUDAGenerator(model_device.index())
            .get_state()
            .clone();
  }
  diagnostic.augmentation_served_consumed =
      current_generator_state_digest(model_device);

  guard.seed_cpu(augmentation_seed_value);
  const auto replay = launcher_augmentation::apply_mtf_training_augmentations(
      mtf::mtf_input_t{clean_data, clean_mask}, preprocessing_config);
  const auto replay_cpu_state =
      at::detail::getDefaultCPUGenerator().get_state().clone();
  torch::Tensor replay_cuda_state{};
  if (model_device.is_cuda()) {
    replay_cuda_state =
        at::cuda::detail::getDefaultCUDAGenerator(model_device.index())
            .get_state()
            .clone();
  }
  diagnostic.augmentation_replay_consumed =
      current_generator_state_digest(model_device);
  diagnostic.served_data_hash = hash_tensor_stable_bytes(result.served.data);
  diagnostic.served_mask_hash =
      hash_tensor_stable_bytes(result.served.feature_mask);
  diagnostic.replay_data_hash = hash_tensor_stable_bytes(replay.data);
  diagnostic.replay_mask_hash = hash_tensor_stable_bytes(replay.feature_mask);
  diagnostic.augmentation_replay_exact =
      torch::equal(result.served.data, replay.data) &&
      torch::equal(result.served.feature_mask, replay.feature_mask);
  diagnostic.augmentation_consumed_state_exact =
      torch::equal(served_cpu_state, replay_cpu_state) &&
      (!model_device.is_cuda() ||
       torch::equal(served_cuda_state, replay_cuda_state));
  diagnostic.augmentation_cuda_unchanged =
      !model_device.is_cuda() ||
      (torch::equal(served_cuda_state, guard.original_cuda_state()) &&
       torch::equal(replay_cuda_state, guard.original_cuda_state()));
  guard.restore();
  diagnostic.augmentation_restored =
      current_generator_state_digest(model_device);
  diagnostic.augmentation_state_restored = generator_state_digest_equal(
      diagnostic.augmentation_original, diagnostic.augmentation_restored);
  diagnostic.retention =
      outer_augmentation_retention(clean_mask, result.served.feature_mask);
  const auto invalid_values = result.served.data.masked_select(
      result.served.feature_mask.logical_not());
  diagnostic.masked_values_zero =
      invalid_values.numel() == 0 || invalid_values.eq(0).all().item<bool>();
  diagnostic.neutral_identity_exact =
      torch::equal(result.served.data, clean_data) &&
      torch::equal(result.served.feature_mask, clean_mask);
  diagnostic.qualified_mask_exact =
      torch::equal(result.served.feature_mask, clean_mask);
  const auto valid_difference = (result.served.data - clean_data)
                                    .abs()
                                    .masked_select(clean_mask.to(torch::kBool));
  diagnostic.qualified_data_changed =
      valid_difference.numel() > 0 && valid_difference.ne(0).any().item<bool>();
  if (!diagnostic.augmentation_replay_exact ||
      !diagnostic.augmentation_consumed_state_exact ||
      !diagnostic.augmentation_cuda_unchanged ||
      !diagnostic.augmentation_state_restored ||
      !diagnostic.masked_values_zero ||
      !diagnostic.retention.every_sample_channel_nonempty ||
      !torch::isfinite(result.served.data).all().item<bool>()) {
    throw std::runtime_error("outer augmentation replay/retention failed");
  }
  return result;
}

[[nodiscard]] uint64_t
optimizer_state_digest(const torch::optim::Adam &optimizer) {
  std::vector<std::pair<std::uintptr_t, uint64_t>> state_digests;
  state_digests.reserve(optimizer.state().size());
  for (const auto &entry : optimizer.state()) {
    const auto *state =
        dynamic_cast<const torch::optim::AdamParamState *>(entry.second.get());
    if (state == nullptr) {
      throw std::runtime_error("attribution optimizer contains non-Adam state");
    }
    uint64_t digest = 0xcbf29ce484222325ULL;
    mix_hash_value(digest, static_cast<uint64_t>(state->step()));
    for (const auto &tensor :
         {state->exp_avg(), state->exp_avg_sq(), state->max_exp_avg_sq()}) {
      mix_hash_value(digest, tensor.defined() ? hash_tensor_stable_bytes(tensor)
                                              : 0x756e646566696e65ULL);
    }
    state_digests.emplace_back(reinterpret_cast<std::uintptr_t>(entry.first),
                               digest);
  }
  std::sort(state_digests.begin(), state_digests.end());
  uint64_t digest = 0xcbf29ce484222325ULL;
  mix_hash_value(digest, static_cast<uint64_t>(state_digests.size()));
  for (const auto &[parameter_address, state_digest] : state_digests) {
    mix_hash_value(digest, static_cast<uint64_t>(parameter_address));
    mix_hash_value(digest, state_digest);
  }
  return digest;
}

[[nodiscard]] uint64_t hash_batch_rows(const std::vector<int64_t> &rows) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  mix_hash_value(hash, static_cast<uint64_t>(rows.size()));
  for (const auto row : rows) {
    mix_hash_value(hash, static_cast<uint64_t>(row));
  }
  return hash;
}

[[nodiscard]] WeakViewDigest
weak_view_digest(const mtf::mtf_jepa_mae_vicreg_output_t &output) {
  return {
      .view_a_data = hash_tensor_stable_bytes(output.vicreg_view_a_data),
      .view_a_feature_mask =
          hash_tensor_stable_bytes(output.vicreg_view_a_feature_mask),
      .view_b_data = hash_tensor_stable_bytes(output.vicreg_view_b_data),
      .view_b_feature_mask =
          hash_tensor_stable_bytes(output.vicreg_view_b_feature_mask),
  };
}

[[nodiscard]] bool weak_view_digests_equal(const WeakViewDigest &left,
                                           const WeakViewDigest &right) {
  return left.view_a_data == right.view_a_data &&
         left.view_a_feature_mask == right.view_a_feature_mask &&
         left.view_b_data == right.view_b_data &&
         left.view_b_feature_mask == right.view_b_feature_mask;
}

void mix_mask_digest(MaskDigest &digest,
                     const mtf::jepa_context_target_mask_t &masks,
                     int64_t step) {
  digest.target =
      splitmix64(digest.target ^ hash_boolean_tensor(masks.target_mask) ^
                 static_cast<uint64_t>(step));
  digest.context =
      splitmix64(digest.context ^ hash_boolean_tensor(masks.context_mask) ^
                 static_cast<uint64_t>(step));
  digest.target_tokens += masks.num_target_tokens;
  digest.context_tokens += masks.num_context_tokens;
  digest.hard_forbidden += masks.hard_forbidden_count;
  digest.soft_forbidden += masks.soft_forbidden_count;
  digest.relaxed_soft_forbidden += masks.relaxed_soft_forbidden_count;
}

[[nodiscard]] std::vector<int64_t>
training_rows(const Dataset &ssl, int64_t model_seed, int64_t step) {
  const int64_t batches_per_epoch = std::max<int64_t>(
      1, (ssl.data.size(0) + kModelRowBatchSize - 1) / kModelRowBatchSize);
  const int64_t epoch = step / batches_per_epoch;
  const auto order = epoch_permutation(ssl.data.size(0), model_seed, epoch);
  const int64_t start = (step % batches_per_epoch) * kModelRowBatchSize;
  std::vector<int64_t> rows;
  rows.reserve(kModelRowBatchSize);
  for (int64_t index = 0; index < kModelRowBatchSize; ++index) {
    rows.push_back(
        order[static_cast<std::size_t>((start + index) % ssl.data.size(0))]);
  }
  return rows;
}

enum class GradientPartition {
  all_trainable,
  served,
  tokenizer,
  encoder,
  predictor,
  mae_decoder,
  vicreg_head
};

[[nodiscard]] torch::Tensor
served_parameter_vector(const mtf::MtfJepaMaeVicreg &model) {
  std::vector<torch::Tensor> pieces;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    const auto &name = item.key();
    const bool served =
        name.rfind("tokenizer.", 0) == 0 || name.rfind("encoder.", 0) == 0;
    if (!item.value().requires_grad() || !served) {
      continue;
    }
    pieces.push_back(
        item.value().detach().to(torch::kCPU, torch::kFloat64).reshape(-1));
  }
  if (pieces.empty()) {
    throw std::runtime_error("attribution served parameter set is empty");
  }
  return torch::cat(pieces).contiguous();
}

[[nodiscard]] torch::Tensor gradient_vector(const mtf::MtfJepaMaeVicreg &model,
                                            GradientPartition partition) {
  std::vector<torch::Tensor> pieces;
  for (const auto &item : model->named_parameters(/*recurse=*/true)) {
    const auto &name = item.key();
    const bool tokenizer_parameter = name.rfind("tokenizer.", 0) == 0;
    const bool encoder_parameter = name.rfind("encoder.", 0) == 0;
    const bool predictor_parameter = name.rfind("predictor.", 0) == 0;
    const bool mae_decoder_parameter = name.rfind("mae_decoder.", 0) == 0;
    const bool vicreg_head_parameter =
        name.rfind("vicreg_stability_head.", 0) == 0;
    const bool include =
        partition == GradientPartition::all_trainable ||
        (partition == GradientPartition::served &&
         (tokenizer_parameter || encoder_parameter)) ||
        (partition == GradientPartition::tokenizer && tokenizer_parameter) ||
        (partition == GradientPartition::encoder && encoder_parameter) ||
        (partition == GradientPartition::predictor && predictor_parameter) ||
        (partition == GradientPartition::mae_decoder &&
         mae_decoder_parameter) ||
        (partition == GradientPartition::vicreg_head && vicreg_head_parameter);
    if (!item.value().requires_grad() || !include) {
      continue;
    }
    const auto &parameter = item.value();
    if (parameter.grad().defined()) {
      pieces.push_back(parameter.grad()
                           .detach()
                           .to(torch::kCPU, torch::kFloat64)
                           .reshape(-1));
    } else {
      pieces.push_back(torch::zeros(
          {parameter.numel()}, torch::TensorOptions().dtype(torch::kFloat64)));
    }
  }
  if (pieces.empty()) {
    throw std::runtime_error("attribution gradient vector is empty");
  }
  const auto result = torch::cat(pieces).contiguous();
  if (!torch::isfinite(result).all().item<bool>()) {
    throw std::runtime_error(
        "attribution representation gradient is non-finite");
  }
  return result;
}

[[nodiscard]] torch::Tensor
branch_tensor(const mtf::mtf_jepa_mae_vicreg_output_t &output,
              std::size_t branch) {
  switch (branch) {
  case 0:
    return output.loss_jepa;
  case 1:
    return output.loss_mae;
  case 2:
    return output.loss_tf_align;
  case 3:
    return output.loss_vicreg;
  default:
    throw std::runtime_error("unknown attribution branch");
  }
}

[[nodiscard]] torch::Tensor
vicreg_component_tensor(const mtf::mtf_jepa_mae_vicreg_output_t &output,
                        const AttributionArm &arm, std::size_t component) {
  constexpr std::array<const char *, 3> suffixes{"sim_term", "var_term",
                                                 "cov_term"};
  if (component >= suffixes.size()) {
    throw std::runtime_error("unknown VICReg component");
  }
  const std::string surface = arm.projected_channel_stratified_vicreg
                                  ? "vicreg_channel_"
                                  : "vicreg_global_";
  return output.diagnostics.at(surface + suffixes[component]);
}

void populate_vicreg_variance_floor_diagnostic(
    mtf::MtfJepaMaeVicreg &model,
    const mtf::mtf_jepa_mae_vicreg_output_t &output,
    GradientDiagnostic &diagnostic) {
  const auto &config = model->config();
  if (config.vicreg_variance_epsilon != 1.0e-4 ||
      config.vicreg_variance_floor != 1.0) {
    throw std::runtime_error(
        "VICReg variance-floor diagnostic scalar contract failed");
  }
  torch::NoGradGuard no_grad;
  const auto projected_a =
      model->project_vicreg(output.vicreg_view_a_pooled_by_channel);
  const auto projected_b =
      model->project_vicreg(output.vicreg_view_b_pooled_by_channel);
  const auto &joint_mask = output.vicreg_channel_joint_mask;
  if (projected_a.dim() != 3 || projected_b.sizes() != projected_a.sizes() ||
      projected_a.size(1) != kChannels || projected_a.size(2) != 64 ||
      joint_mask.sizes() !=
          torch::IntArrayRef({projected_a.size(0), kChannels}) ||
      joint_mask.scalar_type() != torch::kBool) {
    throw std::runtime_error(
        "VICReg variance-floor diagnostic tensor contract failed");
  }
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    const auto rows =
        joint_mask.select(/*dim=*/1, channel).nonzero().reshape({-1});
    const int64_t valid_rows = rows.size(0);
    if (valid_rows < 2) {
      throw std::runtime_error(
          "VICReg variance-floor diagnostic requires two rows per channel");
    }
    diagnostic
        .vicreg_variance_floor_valid_rows[static_cast<std::size_t>(channel)] =
        valid_rows;
    const auto fraction_below_floor = [&](const torch::Tensor &projected) {
      const auto values =
          projected.select(/*dim=*/1, channel).index_select(/*dim=*/0, rows);
      const auto standard_deviation =
          torch::sqrt(values.var(/*dim=*/0, /*unbiased=*/false) + 1.0e-4);
      return standard_deviation.lt(1.0)
          .to(torch::kFloat64)
          .mean()
          .item<double>();
    };
    diagnostic.vicreg_view_a_variance_floor_fraction[static_cast<std::size_t>(
        channel)] = fraction_below_floor(projected_a);
    diagnostic.vicreg_view_b_variance_floor_fraction[static_cast<std::size_t>(
        channel)] = fraction_below_floor(projected_b);
  }
}

[[nodiscard]] std::array<double, 4>
attribution_arm_weights(const AttributionArm &arm, int64_t zero_based_update) {
  double tf_coefficient = arm.lambda_tf_align;
  if (arm.tf_gradient_matched_warmup) {
    const auto ramp_update = std::clamp<int64_t>(zero_based_update, 0,
                                                 kGradientMatchedTfRampUpdates);
    const double progress = static_cast<double>(ramp_update) /
                            static_cast<double>(kGradientMatchedTfRampUpdates);
    tf_coefficient =
        kGradientMatchedTfInitialWeight +
        (kGradientMatchedTfFinalWeight - kGradientMatchedTfInitialWeight) *
            progress;
  }
  return {arm.lambda_jepa, arm.lambda_mae, tf_coefficient, arm.lambda_vicreg};
}

void validate_tf_schedule_contract() {
  constexpr std::size_t kGradientMatchedTfWarmupIndex = 6;
  const auto &arm = kAttributionArms[kGradientMatchedTfWarmupIndex];
  const double expected_step_15 =
      kGradientMatchedTfInitialWeight +
      (kGradientMatchedTfFinalWeight - kGradientMatchedTfInitialWeight) * 15.0 /
          16.0;
  const std::array<std::pair<int64_t, double>, 4> checkpoints{{
      {0, kGradientMatchedTfInitialWeight},
      {15, expected_step_15},
      {16, kGradientMatchedTfFinalWeight},
      {32, kGradientMatchedTfFinalWeight},
  }};
  for (const auto &[step, expected] : checkpoints) {
    const double observed = attribution_arm_weights(arm, step)[2];
    if (std::abs(observed - expected) > 1.0e-15) {
      throw std::runtime_error(
          "gradient-matched TF schedule checkpoint failed");
    }
  }
  double previous = attribution_arm_weights(arm, 0)[2];
  for (int64_t step = 1; step <= kAttributionSteps; ++step) {
    const double current = attribution_arm_weights(arm, step)[2];
    if (current < previous || current > kGradientMatchedTfFinalWeight) {
      throw std::runtime_error(
          "gradient-matched TF schedule monotonicity failed");
    }
    previous = current;
  }
}

[[nodiscard]] torch::Tensor
attribution_arm_loss(const mtf::mtf_jepa_mae_vicreg_output_t &output,
                     const AttributionArm &arm,
                     const std::array<double, 4> &weights) {
  if (!arm.tf_gradient_matched_warmup) {
    return output.loss;
  }
  return weights[0] * output.loss_jepa + weights[1] * output.loss_mae +
         weights[2] * output.loss_tf_align + weights[3] * output.loss_vicreg;
}

void validate_stratified_vicreg_forward(
    const mtf::mtf_jepa_mae_vicreg_output_t &output, const AttributionArm &arm,
    int64_t expected_batch_rows) {
  if (!arm.projected_channel_stratified_vicreg) {
    return;
  }
  const auto active_groups =
      output.diagnostics.at("vicreg_channel_active_groups").item<int64_t>();
  const auto valid_rows =
      output.diagnostics.at("vicreg_channel_valid_rows").item<int64_t>();
  const auto expected_valid_rows = expected_batch_rows * kChannels;
  const auto &pooled_a = output.vicreg_view_a_pooled_by_channel;
  const auto &pooled_b = output.vicreg_view_b_pooled_by_channel;
  const auto &joint_mask = output.vicreg_channel_joint_mask;
  const bool pooled_contract =
      pooled_a.defined() && pooled_b.defined() && pooled_a.dim() == 3 &&
      pooled_b.sizes() == pooled_a.sizes() &&
      pooled_a.size(0) == expected_batch_rows &&
      pooled_a.size(1) == kChannels && pooled_a.size(2) == kLatentDim;
  const bool mask_contract =
      joint_mask.defined() && joint_mask.dim() == 2 &&
      joint_mask.size(0) == expected_batch_rows &&
      joint_mask.size(1) == kChannels &&
      joint_mask.scalar_type() == torch::kBool &&
      joint_mask.sum().item<int64_t>() == expected_valid_rows;
  if (active_groups != kChannels || valid_rows != expected_valid_rows ||
      !pooled_contract || !mask_contract) {
    throw std::runtime_error(
        "projected channel-stratified VICReg forward contract failed");
  }
}

void validate_weak_view_debug_tensors(
    const mtf::mtf_jepa_mae_vicreg_output_t &output,
    const torch::Tensor &input_data, const torch::Tensor &input_feature_mask) {
  for (const auto &view :
       {output.vicreg_view_a_data, output.vicreg_view_b_data}) {
    if (!view.defined() || view.sizes() != input_data.sizes() ||
        view.scalar_type() != input_data.scalar_type()) {
      throw std::runtime_error(
          "attribution weak-view data debug tensor contract failed");
    }
  }
  for (const auto &mask :
       {output.vicreg_view_a_feature_mask, output.vicreg_view_b_feature_mask}) {
    if (!mask.defined() || mask.sizes() != input_feature_mask.sizes() ||
        mask.scalar_type() != input_feature_mask.scalar_type()) {
      throw std::runtime_error(
          "attribution weak-view mask debug tensor contract failed");
    }
  }
}

[[nodiscard]] double relative_gradient_error(const torch::Tensor &observed,
                                             const torch::Tensor &expected) {
  return (observed - expected).norm().item<double>() /
         std::max(1.0e-30, observed.norm().item<double>());
}

[[nodiscard]] double gradient_cosine(const torch::Tensor &left,
                                     const torch::Tensor &right) {
  const double denominator =
      left.norm().item<double>() * right.norm().item<double>();
  if (!(denominator > 1.0e-30)) {
    return std::numeric_limits<double>::quiet_NaN();
  }
  return left.dot(right).item<double>() / denominator;
}

[[nodiscard]] GradientDiagnostic checkpoint_gradient_diagnostic(
    mtf::MtfJepaMaeVicreg &model, const Dataset &ssl,
    const torch::Device &device, const AttributionArm &arm, int64_t model_seed,
    int64_t checkpoint_step, const torch::optim::Adam *optimizer = nullptr) {
  DefaultGeneratorStateGuard generator_state_guard(device);
  constexpr std::array<double, 4> nominal_weights{1.0, 0.25, 0.10, 0.05};
  const auto arm_weights = attribution_arm_weights(arm, checkpoint_step);
  const auto rows = training_rows(ssl, model_seed, 0);
  const auto row_index = torch::tensor(rows, torch::kInt64);
  const auto data = ssl.data.index_select(0, row_index).to(device);
  const auto mask = ssl.mask.index_select(0, row_index).to(device);
  std::array<torch::Tensor, 4> all_trainable{};
  std::array<torch::Tensor, 4> served{};
  std::array<torch::Tensor, 4> tokenizer{};
  std::array<torch::Tensor, 4> encoder{};
  std::array<torch::Tensor, 4> predictor{};
  std::array<torch::Tensor, 4> mae_decoder{};
  std::array<torch::Tensor, 4> vicreg_head{};
  GradientDiagnostic result{};
  const bool was_training = model->is_training();
  const auto parameter_reference = snapshot_parameters(model);
  const uint64_t optimizer_digest_before =
      optimizer != nullptr ? optimizer_state_digest(*optimizer) : 0;
  result.optimizer_state_checked = optimizer != nullptr;
  WeakViewDigest diagnostic_view_reference{};
  bool have_diagnostic_view_reference = false;
  const auto validate_diagnostic_views =
      [&](const mtf::mtf_jepa_mae_vicreg_output_t &output) {
        validate_weak_view_debug_tensors(output, data, mask);
        const auto digest = weak_view_digest(output);
        if (!have_diagnostic_view_reference) {
          diagnostic_view_reference = digest;
          have_diagnostic_view_reference = true;
        } else if (!weak_view_digests_equal(diagnostic_view_reference,
                                            digest)) {
          throw std::runtime_error(
              "attribution diagnostic weak-view replay is not exact");
        }
      };
  model->train();
  for (std::size_t branch = 0; branch < nominal_weights.size(); ++branch) {
    model->zero_grad();
    set_paired_rng(paired_diagnostic_seed(model_seed), device);
    const auto output = model->forward(data, mask);
    validate_diagnostic_views(output);
    validate_stratified_vicreg_forward(output, arm, kModelRowBatchSize);
    const auto raw = branch_tensor(output, branch);
    result.raw_loss[branch] = raw.item<double>();
    (nominal_weights[branch] * raw).backward();
    all_trainable[branch] =
        gradient_vector(model, GradientPartition::all_trainable);
    served[branch] = gradient_vector(model, GradientPartition::served);
    tokenizer[branch] = gradient_vector(model, GradientPartition::tokenizer);
    encoder[branch] = gradient_vector(model, GradientPartition::encoder);
    predictor[branch] = gradient_vector(model, GradientPartition::predictor);
    mae_decoder[branch] =
        gradient_vector(model, GradientPartition::mae_decoder);
    vicreg_head[branch] =
        gradient_vector(model, GradientPartition::vicreg_head);
    result.served_norm[branch] = served[branch].norm().item<double>();
    result.tokenizer_norm[branch] = tokenizer[branch].norm().item<double>();
    result.encoder_norm[branch] = encoder[branch].norm().item<double>();
    result.predictor_norm[branch] = predictor[branch].norm().item<double>();
    result.mae_decoder_norm[branch] = mae_decoder[branch].norm().item<double>();
    result.vicreg_head_norm[branch] = vicreg_head[branch].norm().item<double>();
  }

  const auto &config = model->config();
  result.vicreg_component_surface_is_channel =
      arm.projected_channel_stratified_vicreg;
  result.vicreg_inner_multiplier = arm.projected_channel_stratified_vicreg
                                       ? config.lambda_channel_vicreg
                                       : config.lambda_global_vicreg;
  const std::array<double, 3> component_objective_weights{
      config.vicreg_sim_weight, config.vicreg_var_weight,
      config.vicreg_cov_weight};
  const std::array<double, 3> expected_component_objective_weights{
      25.0, arm.vicreg_var_weight, 1.0};
  if (result.vicreg_inner_multiplier != 0.25 ||
      component_objective_weights != expected_component_objective_weights) {
    throw std::runtime_error(
        "arm-specific VICReg component weighting contract failed");
  }
  std::array<torch::Tensor, 3> component_served{};
  std::array<torch::Tensor, 3> component_head{};
  auto effective_component_served = torch::zeros_like(served[3]);
  auto effective_component_head = torch::zeros_like(vicreg_head[3]);
  for (std::size_t component = 0;
       component < component_objective_weights.size(); ++component) {
    model->zero_grad();
    set_paired_rng(paired_diagnostic_seed(model_seed), device);
    const auto output = model->forward(data, mask);
    validate_diagnostic_views(output);
    validate_stratified_vicreg_forward(output, arm, kModelRowBatchSize);
    const auto raw_component = vicreg_component_tensor(output, arm, component);
    result.vicreg_component_raw_loss[component] = raw_component.item<double>();
    raw_component.backward();
    component_served[component] =
        gradient_vector(model, GradientPartition::served);
    component_head[component] =
        gradient_vector(model, GradientPartition::vicreg_head);
    result.vicreg_component_raw_trunk_norm[component] =
        component_served[component].norm().item<double>();
    result.vicreg_component_raw_head_norm[component] =
        component_head[component].norm().item<double>();
    const double effective_weight = nominal_weights[3] *
                                    result.vicreg_inner_multiplier *
                                    component_objective_weights[component];
    result.vicreg_component_effective_weight[component] = effective_weight;
    const auto weighted_served = effective_weight * component_served[component];
    const auto weighted_head = effective_weight * component_head[component];
    result.vicreg_component_effective_trunk_norm[component] =
        weighted_served.norm().item<double>();
    result.vicreg_component_effective_head_norm[component] =
        weighted_head.norm().item<double>();
    effective_component_served = effective_component_served + weighted_served;
    effective_component_head = effective_component_head + weighted_head;
  }
  result.vicreg_component_trunk_relative_decomposition_error =
      relative_gradient_error(served[3], effective_component_served);
  result.vicreg_component_head_relative_decomposition_error =
      relative_gradient_error(vicreg_head[3], effective_component_head);
  result.vicreg_component_separate_sum_trunk_relative_error =
      result.vicreg_component_trunk_relative_decomposition_error;
  result.vicreg_component_separate_sum_head_relative_error =
      result.vicreg_component_head_relative_decomposition_error;

  model->zero_grad();
  set_paired_rng(paired_diagnostic_seed(model_seed), device);
  const auto reconstructed_output = model->forward(data, mask);
  validate_diagnostic_views(reconstructed_output);
  validate_stratified_vicreg_forward(reconstructed_output, arm,
                                     kModelRowBatchSize);
  auto reconstructed_vicreg =
      torch::zeros_like(reconstructed_output.loss_vicreg);
  for (std::size_t component = 0;
       component < component_objective_weights.size(); ++component) {
    reconstructed_vicreg =
        reconstructed_vicreg +
        component_objective_weights[component] *
            vicreg_component_tensor(reconstructed_output, arm, component);
  }
  reconstructed_vicreg = nominal_weights[3] * result.vicreg_inner_multiplier *
                         reconstructed_vicreg;
  reconstructed_vicreg.backward();
  const auto reconstructed_component_served =
      gradient_vector(model, GradientPartition::served);
  const auto reconstructed_component_head =
      gradient_vector(model, GradientPartition::vicreg_head);
  result.vicreg_component_single_graph_trunk_relative_decomposition_error =
      relative_gradient_error(served[3], reconstructed_component_served);
  result.vicreg_component_single_graph_head_relative_decomposition_error =
      relative_gradient_error(vicreg_head[3], reconstructed_component_head);

  model->zero_grad();
  set_paired_rng(paired_diagnostic_seed(model_seed), device);
  const auto actual_output = model->forward(data, mask);
  validate_diagnostic_views(actual_output);
  validate_stratified_vicreg_forward(actual_output, arm, kModelRowBatchSize);
  result.vicreg_channel_active_groups =
      actual_output.diagnostics.at("vicreg_channel_active_groups")
          .item<int64_t>();
  result.vicreg_channel_valid_rows =
      actual_output.diagnostics.at("vicreg_channel_valid_rows").item<int64_t>();
  populate_vicreg_variance_floor_diagnostic(model, actual_output, result);
  attribution_arm_loss(actual_output, arm, arm_weights).backward();
  const auto actual_all =
      gradient_vector(model, GradientPartition::all_trainable);
  const auto actual_served = gradient_vector(model, GradientPartition::served);
  result.actual_arm_all_trainable_norm = actual_all.norm().item<double>();
  result.actual_arm_served_norm = actual_served.norm().item<double>();

  auto expected_all = torch::zeros_like(all_trainable[0]);
  auto expected_served = torch::zeros_like(served[0]);
  double actual_arm_weighted_branch_norm_sum = 0.0;
  for (std::size_t branch = 0; branch < nominal_weights.size(); ++branch) {
    const double scale = arm_weights[branch] / nominal_weights[branch];
    expected_all = expected_all + scale * all_trainable[branch];
    expected_served = expected_served + scale * served[branch];
    actual_arm_weighted_branch_norm_sum +=
        (scale * served[branch]).norm().item<double>();
  }
  result.all_trainable_relative_decomposition_error =
      relative_gradient_error(actual_all, expected_all);
  result.served_relative_decomposition_error =
      relative_gradient_error(actual_served, expected_served);
  result.optimizer_tf_coefficient = arm_weights[2];
  result.actual_arm_weighted_cancellation_ratio =
      result.actual_arm_served_norm /
      std::max(1.0e-30, actual_arm_weighted_branch_norm_sum);

  std::size_t cosine_index = 0;
  for (std::size_t left = 0; left < served.size(); ++left) {
    for (std::size_t right = left + 1; right < served.size(); ++right) {
      result.served_cosine[cosine_index++] =
          gradient_cosine(served[left], served[right]);
    }
  }
  const auto jepa_mae = served[0] + served[1];
  const auto weighted_tf = (arm_weights[2] / nominal_weights[2]) * served[2];
  const auto tf_vicreg = served[2] + served[3];
  const auto canonical_total = jepa_mae + tf_vicreg;
  result.jepa_mae_norm = jepa_mae.norm().item<double>();
  result.tf_weighted_served_norm_ratio =
      weighted_tf.norm().item<double>() /
      std::max(1.0e-30, result.jepa_mae_norm);
  result.tf_vicreg_norm = tf_vicreg.norm().item<double>();
  result.canonical_total_norm = canonical_total.norm().item<double>();
  double branch_norm_sum = 0.0;
  for (const auto norm : result.served_norm) {
    branch_norm_sum += norm;
  }
  result.cancellation_ratio =
      result.canonical_total_norm / std::max(1.0e-30, branch_norm_sum);

  model->zero_grad();
  model->train(was_training);
  result.repeated_weak_views_exact = have_diagnostic_view_reference;
  result.parameters_and_ema_exact =
      parameter_max_abs_diff(model, parameter_reference) == 0.0;
  result.training_state_exact = model->is_training() == was_training;
  result.optimizer_state_exact =
      optimizer == nullptr ||
      optimizer_state_digest(*optimizer) == optimizer_digest_before;
  if (result.all_trainable_relative_decomposition_error > 1.0e-5 ||
      result.served_relative_decomposition_error > 1.0e-5 ||
      result.vicreg_component_single_graph_trunk_relative_decomposition_error >
          1.0e-5 ||
      result.vicreg_component_single_graph_head_relative_decomposition_error >
          1.0e-5 ||
      !result.repeated_weak_views_exact || !result.parameters_and_ema_exact ||
      !result.training_state_exact || !result.optimizer_state_exact) {
    throw std::runtime_error(
        "attribution gradient decomposition/neutrality contract failed for " +
        std::string(arm.name) + " at checkpoint " +
        std::to_string(checkpoint_step) + ": all=" +
        std::to_string(result.all_trainable_relative_decomposition_error) +
        ", served=" +
        std::to_string(result.served_relative_decomposition_error) +
        ", vicreg_single_graph_trunk=" +
        std::to_string(
            result
                .vicreg_component_single_graph_trunk_relative_decomposition_error) +
        ", vicreg_single_graph_head=" +
        std::to_string(
            result
                .vicreg_component_single_graph_head_relative_decomposition_error) +
        ", views=" + std::to_string(result.repeated_weak_views_exact) +
        ", parameters=" + std::to_string(result.parameters_and_ema_exact) +
        ", training=" + std::to_string(result.training_state_exact) +
        ", optimizer=" + std::to_string(result.optimizer_state_exact));
  }
  generator_state_guard.restore();
  validate_gradient_diagnostic_finite(result,
                                      "attribution checkpoint diagnostic");
  return result;
}

struct VarianceAblationMechanicalResult {
  bool only_variance_weight_changed{false};
  bool initial_parameters_exact{false};
  bool weak_views_exact{false};
  bool raw_component_losses_exact{false};
  bool raw_component_all_gradients_exact{false};
  bool raw_component_served_gradients_exact{false};
  bool raw_component_head_gradients_exact{false};
  bool common_branch_loss_and_gradients_exact{false};
  bool raw_variance_finite_nonzero{false};
  bool no_variance_effective_weight_zero{false};
  double inner_loss_difference_relative_error{0.0};
  double total_loss_difference_relative_error{0.0};
  double total_all_gradient_difference_relative_error{0.0};
  double total_served_gradient_difference_relative_error{0.0};
  double total_head_gradient_difference_relative_error{0.0};
  double direct_float32_total_all_gradient_difference_relative_error{0.0};
  double direct_float32_total_served_gradient_difference_relative_error{0.0};
  double direct_float32_total_head_gradient_difference_relative_error{0.0};
  bool pass{false};
};

[[nodiscard]] VarianceAblationMechanicalResult
validate_variance_ablation_mechanics(const Dataset &ssl,
                                     const torch::Device &device,
                                     int64_t model_seed) {
  DefaultGeneratorStateGuard generator_state_guard(device);
  constexpr std::size_t kFullIndex = 1;
  constexpr std::size_t kNoVarianceIndex = 2;
  const auto &full_arm = kVarianceNecessityArms[kFullIndex];
  const auto &no_variance_arm = kVarianceNecessityArms[kNoVarianceIndex];
  VarianceAblationMechanicalResult result{};
  result.only_variance_weight_changed =
      std::string(full_arm.name) != std::string(no_variance_arm.name) &&
      full_arm.lambda_jepa == no_variance_arm.lambda_jepa &&
      full_arm.lambda_mae == no_variance_arm.lambda_mae &&
      full_arm.lambda_tf_align == no_variance_arm.lambda_tf_align &&
      full_arm.lambda_vicreg == no_variance_arm.lambda_vicreg &&
      full_arm.max_context_target_time_overlap ==
          no_variance_arm.max_context_target_time_overlap &&
      full_arm.tf_gradient_matched_warmup ==
          no_variance_arm.tf_gradient_matched_warmup &&
      full_arm.projected_channel_stratified_vicreg &&
      no_variance_arm.projected_channel_stratified_vicreg &&
      full_arm.vicreg_var_weight == 25.0 &&
      no_variance_arm.vicreg_var_weight == 0.0;

  set_paired_rng(model_seed, device);
  auto full_model = mtf::MtfJepaMaeVicreg(attribution_config(device, full_arm));
  set_paired_rng(model_seed, device);
  auto no_variance_model =
      mtf::MtfJepaMaeVicreg(attribution_config(device, no_variance_arm));
  result.initial_parameters_exact =
      parameter_max_abs_diff(no_variance_model,
                             snapshot_parameters(full_model)) == 0.0;
  full_model->train();
  no_variance_model->train();

  const auto rows = training_rows(ssl, model_seed, 0);
  const auto row_index = torch::tensor(rows, torch::kInt64);
  const auto data = ssl.data.index_select(0, row_index).to(device);
  const auto mask = ssl.mask.index_select(0, row_index).to(device);

  struct GradientSurface {
    double loss{0.0};
    double inner_vicreg_loss{0.0};
    double raw_variance_loss{0.0};
    torch::Tensor all{};
    torch::Tensor served{};
    torch::Tensor head{};
    WeakViewDigest views{};
  };
  const auto evaluate = [&](mtf::MtfJepaMaeVicreg &model,
                            const AttributionArm &arm,
                            int64_t component) -> GradientSurface {
    model->zero_grad();
    set_paired_rng(paired_diagnostic_seed(model_seed), device);
    const auto output = model->forward(data, mask);
    validate_weak_view_debug_tensors(output, data, mask);
    validate_stratified_vicreg_forward(output, arm, kModelRowBatchSize);
    torch::Tensor loss;
    if (component >= 0) {
      loss = vicreg_component_tensor(output, arm,
                                     static_cast<std::size_t>(component));
    } else if (component == -2) {
      loss = arm.lambda_vicreg * output.loss_vicreg;
    } else if (component == -3) {
      loss = arm.lambda_jepa * output.loss_jepa +
             arm.lambda_mae * output.loss_mae +
             arm.lambda_tf_align * output.loss_tf_align;
    } else {
      loss = output.loss;
    }
    loss.backward();
    return {
        .loss = loss.item<double>(),
        .inner_vicreg_loss = output.loss_vicreg.item<double>(),
        .raw_variance_loss =
            vicreg_component_tensor(output, arm, 1).item<double>(),
        .all = gradient_vector(model, GradientPartition::all_trainable),
        .served = gradient_vector(model, GradientPartition::served),
        .head = gradient_vector(model, GradientPartition::vicreg_head),
        .views = weak_view_digest(output),
    };
  };

  const auto full_total = evaluate(full_model, full_arm, -1);
  const auto no_variance_total =
      evaluate(no_variance_model, no_variance_arm, -1);
  const auto full_vicreg_branch = evaluate(full_model, full_arm, -2);
  const auto no_variance_vicreg_branch =
      evaluate(no_variance_model, no_variance_arm, -2);
  const auto full_common_branch = evaluate(full_model, full_arm, -3);
  const auto no_variance_common_branch =
      evaluate(no_variance_model, no_variance_arm, -3);
  const auto &view_reference = full_total.views;
  const auto views_match_reference = [&](const GradientSurface &surface) {
    return weak_view_digests_equal(view_reference, surface.views);
  };
  result.weak_views_exact = views_match_reference(no_variance_total) &&
                            views_match_reference(full_vicreg_branch) &&
                            views_match_reference(no_variance_vicreg_branch) &&
                            views_match_reference(full_common_branch) &&
                            views_match_reference(no_variance_common_branch);
  result.common_branch_loss_and_gradients_exact =
      full_common_branch.loss == no_variance_common_branch.loss &&
      torch::equal(full_common_branch.all, no_variance_common_branch.all) &&
      torch::equal(full_common_branch.served,
                   no_variance_common_branch.served) &&
      torch::equal(full_common_branch.head, no_variance_common_branch.head);

  std::array<GradientSurface, 3> full_components{};
  std::array<GradientSurface, 3> no_variance_components{};
  result.raw_component_losses_exact = true;
  result.raw_component_all_gradients_exact = true;
  result.raw_component_served_gradients_exact = true;
  result.raw_component_head_gradients_exact = true;
  for (std::size_t component = 0; component < full_components.size();
       ++component) {
    full_components[component] =
        evaluate(full_model, full_arm, static_cast<int64_t>(component));
    no_variance_components[component] = evaluate(
        no_variance_model, no_variance_arm, static_cast<int64_t>(component));
    result.weak_views_exact =
        result.weak_views_exact &&
        views_match_reference(full_components[component]) &&
        views_match_reference(no_variance_components[component]);
    result.raw_component_losses_exact =
        result.raw_component_losses_exact &&
        full_components[component].loss ==
            no_variance_components[component].loss;
    result.raw_component_all_gradients_exact =
        result.raw_component_all_gradients_exact &&
        torch::equal(full_components[component].all,
                     no_variance_components[component].all);
    result.raw_component_served_gradients_exact =
        result.raw_component_served_gradients_exact &&
        torch::equal(full_components[component].served,
                     no_variance_components[component].served);
    result.raw_component_head_gradients_exact =
        result.raw_component_head_gradients_exact &&
        torch::equal(full_components[component].head,
                     no_variance_components[component].head);
  }

  const auto &raw_variance = full_components[1];
  result.raw_variance_finite_nonzero =
      std::isfinite(raw_variance.loss) && raw_variance.loss > 0.0 &&
      torch::isfinite(raw_variance.all).all().item<bool>() &&
      raw_variance.all.norm().item<double>() > 0.0;
  constexpr double kInnerVarianceWeight = 0.25 * 25.0;
  constexpr double kEffectiveVarianceWeight = 0.05 * kInnerVarianceWeight;
  result.no_variance_effective_weight_zero =
      no_variance_arm.vicreg_var_weight == 0.0;
  const double observed_inner_loss_difference =
      full_total.inner_vicreg_loss - no_variance_total.inner_vicreg_loss;
  const double expected_inner_loss_difference =
      kInnerVarianceWeight * raw_variance.loss;
  result.inner_loss_difference_relative_error =
      std::abs(observed_inner_loss_difference -
               expected_inner_loss_difference) /
      std::max(1.0e-30, std::abs(observed_inner_loss_difference));
  const double observed_total_loss_difference =
      full_total.loss - no_variance_total.loss;
  const double expected_total_loss_difference =
      kEffectiveVarianceWeight * raw_variance.loss;
  result.total_loss_difference_relative_error =
      std::abs(observed_total_loss_difference -
               expected_total_loss_difference) /
      std::max(1.0e-30, std::abs(observed_total_loss_difference));
  result.direct_float32_total_all_gradient_difference_relative_error =
      relative_gradient_error(full_total.all - no_variance_total.all,
                              kEffectiveVarianceWeight * raw_variance.all);
  result.direct_float32_total_served_gradient_difference_relative_error =
      relative_gradient_error(full_total.served - no_variance_total.served,
                              kEffectiveVarianceWeight * raw_variance.served);
  result.direct_float32_total_head_gradient_difference_relative_error =
      relative_gradient_error(full_total.head - no_variance_total.head,
                              kEffectiveVarianceWeight * raw_variance.head);
  result.total_all_gradient_difference_relative_error = relative_gradient_error(
      full_vicreg_branch.all - no_variance_vicreg_branch.all,
      kEffectiveVarianceWeight * raw_variance.all);
  result.total_served_gradient_difference_relative_error =
      relative_gradient_error(full_vicreg_branch.served -
                                  no_variance_vicreg_branch.served,
                              kEffectiveVarianceWeight * raw_variance.served);
  result.total_head_gradient_difference_relative_error =
      relative_gradient_error(full_vicreg_branch.head -
                                  no_variance_vicreg_branch.head,
                              kEffectiveVarianceWeight * raw_variance.head);
  result.pass =
      result.only_variance_weight_changed && result.initial_parameters_exact &&
      result.weak_views_exact && result.raw_component_losses_exact &&
      result.raw_component_all_gradients_exact &&
      result.raw_component_served_gradients_exact &&
      result.raw_component_head_gradients_exact &&
      result.common_branch_loss_and_gradients_exact &&
      result.raw_variance_finite_nonzero &&
      result.no_variance_effective_weight_zero &&
      result.inner_loss_difference_relative_error <= 1.0e-5 &&
      result.total_loss_difference_relative_error <= 1.0e-5 &&
      result.total_all_gradient_difference_relative_error <= 1.0e-5 &&
      result.total_served_gradient_difference_relative_error <= 1.0e-5 &&
      result.total_head_gradient_difference_relative_error <= 1.0e-5;
  full_model->zero_grad();
  no_variance_model->zero_grad();
  generator_state_guard.restore();
  return result;
}

[[nodiscard]] AttributionCheckpoint evaluate_attribution_checkpoint(
    mtf::MtfJepaMaeVicreg &model, const Dataset &probe_train,
    const Dataset &probe_validation, const Dataset &test,
    const torch::Device &device, int64_t step) {
  const auto train_embeddings = extract_embeddings(model, probe_train, device);
  const auto validation_embeddings =
      extract_embeddings(model, probe_validation, device);
  const auto test_embeddings = extract_embeddings(model, test, device);
  const auto vicreg_surfaces =
      extract_vicreg_geometry_surfaces(model, test, device);
  std::array<Geometry, kChannels> projected_channel_geometry{};
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    projected_channel_geometry[static_cast<std::size_t>(channel)] =
        geometry_for_channel(
            vicreg_surfaces.projected_by_channel.select(/*dim=*/1, channel));
  }
  return {
      .step = step,
      .probe =
          probe_curve(train_embeddings.flat, validation_embeddings.flat,
                      test_embeddings.flat, probe_train.target,
                      probe_validation.target, test.target, {32, 64, 128, 256}),
      .geometry = geometry(test_embeddings),
      .vicreg_clean_global_preprojector_geometry =
          geometry_for_channel(vicreg_surfaces.global_preprojector),
      .vicreg_clean_projected_channel_geometry = projected_channel_geometry,
      .test_embeddings = test_embeddings.flat.clone(),
  };
}

[[nodiscard]] AttributionTraining train_attribution_arm(
    mtf::MtfJepaMaeVicreg &model, const Dataset &ssl,
    const Dataset &probe_train, const Dataset &probe_validation,
    const Dataset &test, const torch::Device &device, const AttributionArm &arm,
    int64_t model_seed, std::vector<AttributionCheckpoint> &checkpoints,
    const mtf::mtf_jepa_mae_vicreg_config_t *outer_preprocessing = nullptr,
    std::size_t outer_arm_index = std::numeric_limits<std::size_t>::max(),
    bool capture_jmcd_mechanics = false) {
  auto parameters = model->parameters();
  torch::optim::Adam optimizer(parameters, torch::optim::AdamOptions(1.0e-3));
  AttributionTraining result{};
  result.total_losses.reserve(kAttributionSteps);
  result.pre_clip_gradient_norms.reserve(kAttributionSteps);
  result.clip_factors.reserve(kAttributionSteps);
  result.optimizer_tf_coefficients.reserve(kAttributionSteps);
  result.target_masks.reserve(kAttributionSteps);
  result.context_masks.reserve(kAttributionSteps);
  result.batch_rows.reserve(kAttributionSteps);
  result.batch_row_hashes.reserve(kAttributionSteps);
  result.weak_view_digests.reserve(kAttributionSteps);
  result.served_update_norms.reserve(kAttributionSteps);
  if (capture_jmcd_mechanics) {
    result.clean_data_hashes.reserve(kAttributionSteps);
    result.clean_mask_hashes.reserve(kAttributionSteps);
    result.module_forward_pre_states.reserve(kAttributionSteps);
    result.module_forward_post_states.reserve(kAttributionSteps);
  }
  if (outer_preprocessing != nullptr) {
    if (outer_arm_index >= kOuterAugmentationArms.size()) {
      throw std::runtime_error("outer augmentation training arm is invalid");
    }
    result.outer_augmentation_updates.reserve(kAttributionSteps);
    result.outer_view_a_feature_masks.reserve(kAttributionSteps);
    result.outer_view_b_feature_masks.reserve(kAttributionSteps);
    result.model_config_fingerprint =
        fnv1a64(canonical_config_manifest(attribution_config(device, arm)));
    result.preprocessing_config_fingerprint =
        fnv1a64(canonical_preprocessing_manifest(*outer_preprocessing));
  }
  model->train();
  for (int64_t step = 0; step < kAttributionSteps; ++step) {
    const auto rows = training_rows(ssl, model_seed, step);
    const auto row_index = torch::tensor(rows, torch::kInt64);
    const auto clean_data_cpu = ssl.data.index_select(0, row_index);
    const auto clean_mask_cpu = ssl.mask.index_select(0, row_index);
    torch::Tensor data{};
    torch::Tensor feature_mask{};
    OuterAugmentationUpdate outer_update{};
    result.batch_rows.push_back(rows);
    result.batch_row_hashes.push_back(hash_batch_rows(rows));
    if (capture_jmcd_mechanics) {
      result.clean_data_hashes.push_back(
          hash_tensor_stable_bytes(clean_data_cpu));
      result.clean_mask_hashes.push_back(
          hash_tensor_stable_bytes(clean_mask_cpu));
    }

    mtf::jepa_context_target_mask_t preview_masks{};
    if (outer_preprocessing == nullptr) {
      data = clean_data_cpu.to(device);
      feature_mask = clean_mask_cpu.to(device);
      set_paired_rng(paired_step_seed(model_seed, step), device);
      {
        torch::NoGradGuard no_grad;
        const auto tokens = model->tokenize(data, feature_mask);
        preview_masks = model->create_masks(tokens);
      }
    } else {
      auto augmented = apply_outer_augmentation_replayed(
          clean_data_cpu, clean_mask_cpu, *outer_preprocessing,
          outer_augmentation_seed(model_seed, step), device);
      outer_update = std::move(augmented.diagnostic);
      outer_update.clean_data_cpu = clean_data_cpu;
      outer_update.clean_mask_cpu = clean_mask_cpu;
      data = augmented.served.data.to(device);
      feature_mask = augmented.served.feature_mask.to(device);
      const auto clean_data = clean_data_cpu.to(device);
      const auto clean_mask = clean_mask_cpu.to(device);
      const auto support_only_data =
          torch::where(feature_mask, clean_data, torch::zeros_like(clean_data));
      const auto forward_seed = paired_step_seed(model_seed, step);
      const auto clean_preview = preview_masks_with_state(
          model, clean_data, clean_mask, forward_seed, device);
      const auto augmented_preview = preview_masks_with_state(
          model, data, feature_mask, forward_seed, device);
      const auto support_only_preview = preview_masks_with_state(
          model, support_only_data, feature_mask, forward_seed, device);
      const auto augmented_preview_replay = preview_masks_with_state(
          model, data, feature_mask, forward_seed, device);
      preview_masks = augmented_preview.masks;
      outer_update.clean_mask_plan = clone_mask_plan_cpu(clean_preview.masks);
      outer_update.augmented_preview_post = augmented_preview.post_state.digest;
      outer_update.augmented_preview_replay_post =
          augmented_preview_replay.post_state.digest;
      outer_update.preview_replay_exact =
          jepa_masks_exact(augmented_preview.masks,
                           augmented_preview_replay.masks) &&
          generator_state_snapshot_equal(augmented_preview.post_state,
                                         augmented_preview_replay.post_state);
      outer_update.support_counterfactual_exact =
          jepa_masks_exact(augmented_preview.masks, support_only_preview.masks);
      outer_update.actual_forward_data_hash = hash_tensor_stable_bytes(data);
      outer_update.actual_forward_mask_hash =
          hash_tensor_stable_bytes(feature_mask);
      outer_update.actual_input_matches_served =
          outer_update.actual_forward_data_hash ==
              outer_update.served_data_hash &&
          outer_update.actual_forward_mask_hash ==
              outer_update.served_mask_hash;
      if ((outer_arm_index == kOuterNeutralIndex &&
           !outer_update.neutral_identity_exact) ||
          (outer_arm_index == kOuterQualifiedIndex &&
           (!outer_update.qualified_data_changed ||
            !outer_update.qualified_mask_exact)) ||
          !outer_update.preview_replay_exact ||
          !outer_update.support_counterfactual_exact ||
          !outer_update.actual_input_matches_served) {
        throw std::runtime_error(
            "outer augmentation arm/input preview contract failed");
      }
    }
    // Rewind both generators so the diagnostic preview cannot perturb the
    // training draw schedule. Every arm receives this same per-step seed.
    set_paired_rng(paired_step_seed(model_seed, step), device);
    if (capture_jmcd_mechanics) {
      result.module_forward_pre_states.push_back(
          current_generator_state_snapshot(device));
    }
    if (outer_preprocessing != nullptr) {
      outer_update.module_forward_pre_snapshot =
          current_generator_state_snapshot(device);
      outer_update.module_forward_pre =
          outer_update.module_forward_pre_snapshot.digest;
    }
    optimizer.zero_grad();
    const auto output = model->forward(data, feature_mask);
    if (capture_jmcd_mechanics) {
      result.module_forward_post_states.push_back(
          current_generator_state_snapshot(device));
    }
    if (outer_preprocessing != nullptr) {
      outer_update.module_forward_post_snapshot =
          current_generator_state_snapshot(device);
      outer_update.module_forward_post =
          outer_update.module_forward_post_snapshot.digest;
    }
    validate_weak_view_debug_tensors(output, data, feature_mask);
    validate_stratified_vicreg_forward(output, arm, kModelRowBatchSize);
    if (!output.jepa_target_mask.defined() ||
        !output.jepa_context_mask.defined() ||
        output.jepa_target_mask.scalar_type() != torch::kBool ||
        output.jepa_context_mask.scalar_type() != torch::kBool ||
        !torch::equal(output.jepa_target_mask, preview_masks.target_mask) ||
        !torch::equal(output.jepa_context_mask, preview_masks.context_mask)) {
      throw std::runtime_error(
          "attribution preview/actual forward masks are not exactly equal");
    }
    if (outer_preprocessing != nullptr) {
      outer_update.actual_masks_match_preview = true;
      result.outer_augmentation_updates.push_back(std::move(outer_update));
    }
    auto actual_masks = preview_masks;
    actual_masks.target_mask = output.jepa_target_mask;
    actual_masks.context_mask = output.jepa_context_mask;
    mix_mask_digest(result.masks, actual_masks, step);
    result.target_masks.push_back(
        output.jepa_target_mask.to(torch::kCPU).clone());
    result.context_masks.push_back(
        output.jepa_context_mask.to(torch::kCPU).clone());
    result.weak_view_digests.push_back(weak_view_digest(output));
    if (outer_preprocessing != nullptr) {
      result.outer_view_a_feature_masks.push_back(
          output.vicreg_view_a_feature_mask.to(torch::kCPU).clone());
      result.outer_view_b_feature_masks.push_back(
          output.vicreg_view_b_feature_mask.to(torch::kCPU).clone());
    }
    const auto arm_weights = attribution_arm_weights(arm, step);
    const auto optimizer_loss = attribution_arm_loss(output, arm, arm_weights);
    const double loss = optimizer_loss.item<double>();
    if (!std::isfinite(loss)) {
      throw std::runtime_error("attribution training loss is non-finite");
    }
    result.total_losses.push_back(loss);
    result.optimizer_tf_coefficients.push_back(arm_weights[2]);
    result.component_loss_sums[0] += output.loss_jepa.item<double>();
    result.component_loss_sums[1] += output.loss_mae.item<double>();
    result.component_loss_sums[2] += output.loss_tf_align.item<double>();
    result.component_loss_sums[3] += output.loss_vicreg.item<double>();
    const auto output_target_tokens =
        output.diagnostics.at("num_target_tokens").item<int64_t>();
    const auto output_context_tokens =
        output.diagnostics.at("num_context_tokens").item<int64_t>();
    if (output_target_tokens != preview_masks.num_target_tokens ||
        output_context_tokens != preview_masks.num_context_tokens) {
      throw std::runtime_error(
          "attribution mask preview/forward count mismatch");
    }
    optimizer_loss.backward();

    double gradient_square_sum = 0.0;
    for (const auto &parameter : parameters) {
      if (!parameter.grad().defined()) {
        continue;
      }
      if (!torch::isfinite(parameter.grad()).all().item<bool>()) {
        throw std::runtime_error("attribution gradient is non-finite");
      }
      gradient_square_sum +=
          parameter.grad().detach().pow(2).sum().item<double>();
    }
    const double gradient_norm = std::sqrt(gradient_square_sum);
    const double clip_factor =
        gradient_norm > 5.0 ? 5.0 / std::max(gradient_norm, 1.0e-30) : 1.0;
    result.pre_clip_gradient_norms.push_back(gradient_norm);
    result.clip_factors.push_back(clip_factor);
    if (clip_factor < 1.0) {
      throw std::runtime_error(
          "attribution no-clipping contract failed before optimizer step");
    }
    const auto served_before_adam = served_parameter_vector(model);
    optimizer.step();
    const auto served_after_adam = served_parameter_vector(model);
    const double served_update_norm =
        (served_after_adam - served_before_adam).norm().item<double>();
    if (!std::isfinite(served_update_norm)) {
      throw std::runtime_error(
          "attribution served-parameter update norm is non-finite");
    }
    result.served_update_norms.push_back(served_update_norm);
    model->update_target_network();

    const int64_t completed = step + 1;
    if (completed == kAttributionMidpoint || completed == kAttributionSteps) {
      auto checkpoint = evaluate_attribution_checkpoint(
          model, probe_train, probe_validation, test, device, completed);
      checkpoint.gradients = checkpoint_gradient_diagnostic(
          model, ssl, device, arm, model_seed, completed, &optimizer);
      validate_attribution_checkpoint_finite(checkpoint,
                                             "attribution checkpoint step " +
                                                 std::to_string(completed));
      checkpoints.push_back(std::move(checkpoint));
    }
  }
  if (static_cast<int64_t>(result.total_losses.size()) != kAttributionSteps ||
      static_cast<int64_t>(result.optimizer_tf_coefficients.size()) !=
          kAttributionSteps ||
      static_cast<int64_t>(result.batch_rows.size()) != kAttributionSteps ||
      static_cast<int64_t>(result.batch_row_hashes.size()) !=
          kAttributionSteps ||
      static_cast<int64_t>(result.weak_view_digests.size()) !=
          kAttributionSteps ||
      static_cast<int64_t>(result.served_update_norms.size()) !=
          kAttributionSteps ||
      (outer_preprocessing != nullptr &&
       static_cast<int64_t>(result.outer_augmentation_updates.size()) !=
           kAttributionSteps) ||
      (outer_preprocessing != nullptr &&
       (static_cast<int64_t>(result.outer_view_a_feature_masks.size()) !=
            kAttributionSteps ||
        static_cast<int64_t>(result.outer_view_b_feature_masks.size()) !=
            kAttributionSteps)) ||
      (outer_preprocessing == nullptr &&
       !result.outer_augmentation_updates.empty()) ||
      (capture_jmcd_mechanics &&
       (static_cast<int64_t>(result.clean_data_hashes.size()) !=
            kAttributionSteps ||
        static_cast<int64_t>(result.clean_mask_hashes.size()) !=
            kAttributionSteps ||
        static_cast<int64_t>(result.module_forward_pre_states.size()) !=
            kAttributionSteps ||
        static_cast<int64_t>(result.module_forward_post_states.size()) !=
            kAttributionSteps)) ||
      (!capture_jmcd_mechanics &&
       (!result.clean_data_hashes.empty() ||
        !result.clean_mask_hashes.empty() ||
        !result.module_forward_pre_states.empty() ||
        !result.module_forward_post_states.empty())) ||
      checkpoints.size() != 3) {
    throw std::runtime_error("attribution checkpoint contract failed");
  }
  validate_attribution_training_finite(result, "attribution training");
  return result;
}

[[nodiscard]] double mean_loss_window(const std::vector<double> &losses,
                                      bool first) {
  constexpr std::size_t kWindow = 8;
  if (losses.size() < kWindow) {
    throw std::runtime_error("attribution loss window is incomplete");
  }
  const auto begin = first ? losses.begin() : losses.end() - kWindow;
  return std::accumulate(begin, begin + kWindow, 0.0) /
         static_cast<double>(kWindow);
}

struct OuterSeedPairingResult {
  bool batch_rows_exact{true};
  bool batch_row_hashes_exact{true};
  bool clean_cpu_inputs_exact{true};
  bool clean_input_hashes_exact{true};
  bool clean_base_masks_exact{true};
  bool neutral_qualified_actual_masks_exact{true};
  bool neutral_qualified_weak_feature_masks_exact{true};
  bool module_forward_pre_state_all_arms_exact{true};
  bool neutral_qualified_post_state_exact{true};
  bool model_config_fingerprints_exact{true};
  bool per_update_mechanics{true};
  bool pass{true};
};

[[nodiscard]] OuterSeedPairingResult
validate_outer_seed_pairing(const std::vector<AttributionArmResult> &results,
                            std::size_t seed_begin) {
  if (seed_begin + kOuterAugmentationArms.size() > results.size()) {
    throw std::runtime_error("outer augmentation seed result range failed");
  }
  OuterSeedPairingResult pairing{};
  const auto &neutral = results[seed_begin].training;
  const auto &full = results[seed_begin + kOuterFullActiveIndex].training;
  const auto &qualified = results[seed_begin + kOuterQualifiedIndex].training;
  const auto complete = [](const AttributionTraining &training) {
    return training.outer_augmentation_updates.size() == kAttributionSteps &&
           training.batch_rows.size() == kAttributionSteps &&
           training.batch_row_hashes.size() == kAttributionSteps &&
           training.target_masks.size() == kAttributionSteps &&
           training.context_masks.size() == kAttributionSteps &&
           training.outer_view_a_feature_masks.size() == kAttributionSteps &&
           training.outer_view_b_feature_masks.size() == kAttributionSteps;
  };
  if (!complete(neutral) || !complete(full) || !complete(qualified)) {
    throw std::runtime_error("outer augmentation update surface is incomplete");
  }
  pairing.model_config_fingerprints_exact =
      neutral.model_config_fingerprint == full.model_config_fingerprint &&
      neutral.model_config_fingerprint == qualified.model_config_fingerprint;
  for (int64_t step = 0; step < kAttributionSteps; ++step) {
    const auto index = static_cast<std::size_t>(step);
    pairing.batch_rows_exact =
        pairing.batch_rows_exact &&
        neutral.batch_rows[index] == full.batch_rows[index] &&
        neutral.batch_rows[index] == qualified.batch_rows[index];
    pairing.batch_row_hashes_exact =
        pairing.batch_row_hashes_exact &&
        neutral.batch_row_hashes[index] == full.batch_row_hashes[index] &&
        neutral.batch_row_hashes[index] == qualified.batch_row_hashes[index];
    const auto &neutral_update = neutral.outer_augmentation_updates[index];
    const auto &full_update = full.outer_augmentation_updates[index];
    const auto &qualified_update = qualified.outer_augmentation_updates[index];
    pairing.clean_cpu_inputs_exact =
        pairing.clean_cpu_inputs_exact &&
        torch::equal(neutral_update.clean_data_cpu,
                     full_update.clean_data_cpu) &&
        torch::equal(neutral_update.clean_data_cpu,
                     qualified_update.clean_data_cpu) &&
        torch::equal(neutral_update.clean_mask_cpu,
                     full_update.clean_mask_cpu) &&
        torch::equal(neutral_update.clean_mask_cpu,
                     qualified_update.clean_mask_cpu);
    pairing.clean_input_hashes_exact =
        pairing.clean_input_hashes_exact &&
        neutral_update.clean_data_hash == full_update.clean_data_hash &&
        neutral_update.clean_data_hash == qualified_update.clean_data_hash &&
        neutral_update.clean_mask_hash == full_update.clean_mask_hash &&
        neutral_update.clean_mask_hash == qualified_update.clean_mask_hash;
    pairing.clean_base_masks_exact =
        pairing.clean_base_masks_exact &&
        jepa_masks_exact(neutral_update.clean_mask_plan,
                         full_update.clean_mask_plan) &&
        jepa_masks_exact(neutral_update.clean_mask_plan,
                         qualified_update.clean_mask_plan);
    pairing.neutral_qualified_actual_masks_exact =
        pairing.neutral_qualified_actual_masks_exact &&
        torch::equal(neutral.target_masks[index],
                     qualified.target_masks[index]) &&
        torch::equal(neutral.context_masks[index],
                     qualified.context_masks[index]);
    pairing.neutral_qualified_weak_feature_masks_exact =
        pairing.neutral_qualified_weak_feature_masks_exact &&
        torch::equal(neutral.outer_view_a_feature_masks[index],
                     qualified.outer_view_a_feature_masks[index]) &&
        torch::equal(neutral.outer_view_b_feature_masks[index],
                     qualified.outer_view_b_feature_masks[index]);
    pairing.module_forward_pre_state_all_arms_exact =
        pairing.module_forward_pre_state_all_arms_exact &&
        generator_state_snapshot_equal(
            neutral_update.module_forward_pre_snapshot,
            full_update.module_forward_pre_snapshot) &&
        generator_state_snapshot_equal(
            neutral_update.module_forward_pre_snapshot,
            qualified_update.module_forward_pre_snapshot);
    pairing.neutral_qualified_post_state_exact =
        pairing.neutral_qualified_post_state_exact &&
        generator_state_snapshot_equal(
            neutral_update.module_forward_post_snapshot,
            qualified_update.module_forward_post_snapshot);
    const auto common_update_pass = [](const OuterAugmentationUpdate &update) {
      return update.augmentation_replay_exact &&
             update.augmentation_consumed_state_exact &&
             update.augmentation_cuda_unchanged &&
             update.augmentation_state_restored && update.masked_values_zero &&
             update.preview_replay_exact &&
             update.support_counterfactual_exact &&
             update.actual_masks_match_preview &&
             update.actual_input_matches_served &&
             update.retention.every_sample_channel_nonempty;
    };
    pairing.per_update_mechanics = pairing.per_update_mechanics &&
                                   common_update_pass(neutral_update) &&
                                   neutral_update.neutral_identity_exact &&
                                   common_update_pass(full_update) &&
                                   common_update_pass(qualified_update) &&
                                   qualified_update.qualified_data_changed &&
                                   qualified_update.qualified_mask_exact;
  }
  pairing.pass =
      pairing.batch_rows_exact && pairing.batch_row_hashes_exact &&
      pairing.clean_cpu_inputs_exact && pairing.clean_input_hashes_exact &&
      pairing.clean_base_masks_exact &&
      pairing.neutral_qualified_actual_masks_exact &&
      pairing.neutral_qualified_weak_feature_masks_exact &&
      pairing.module_forward_pre_state_all_arms_exact &&
      pairing.neutral_qualified_post_state_exact &&
      pairing.model_config_fingerprints_exact && pairing.per_update_mechanics;
  return pairing;
}

void emit_gradient_diagnostic(const std::string &prefix,
                              const GradientDiagnostic &diagnostic) {
  std::cout << prefix << ".optimizer_lambda_tf_align="
            << diagnostic.optimizer_tf_coefficient << '\n';
  for (std::size_t branch = 0; branch < kAttributionBranchNames.size();
       ++branch) {
    const std::string branch_prefix =
        prefix + ".branch_" + kAttributionBranchNames[branch];
    if (branch == 3) {
      std::cout << branch_prefix
                << ".inner_weighted_loss=" << diagnostic.raw_loss[branch]
                << '\n';
    } else {
      std::cout << branch_prefix << ".raw_loss=" << diagnostic.raw_loss[branch]
                << '\n';
    }
    std::cout << branch_prefix
              << ".served_norm=" << diagnostic.served_norm[branch] << '\n';
    std::cout << branch_prefix
              << ".tokenizer_norm=" << diagnostic.tokenizer_norm[branch]
              << '\n';
    std::cout << branch_prefix
              << ".encoder_norm=" << diagnostic.encoder_norm[branch] << '\n';
    std::cout << branch_prefix
              << ".vicreg_head_norm=" << diagnostic.vicreg_head_norm[branch]
              << '\n';
  }
  std::cout << prefix << ".vicreg_component_surface="
            << (diagnostic.vicreg_component_surface_is_channel ? "channel"
                                                               : "global")
            << '\n';
  std::cout << prefix
            << ".vicreg_inner_multiplier=" << diagnostic.vicreg_inner_multiplier
            << '\n';
  for (std::size_t component = 0; component < kVicregComponentNames.size();
       ++component) {
    const std::string component_prefix =
        prefix + ".vicreg_component_" + kVicregComponentNames[component];
    std::cout << component_prefix
              << ".raw_loss=" << diagnostic.vicreg_component_raw_loss[component]
              << '\n';
    std::cout << component_prefix << ".raw_trunk_gradient_norm="
              << diagnostic.vicreg_component_raw_trunk_norm[component] << '\n';
    std::cout << component_prefix << ".raw_head_gradient_norm="
              << diagnostic.vicreg_component_raw_head_norm[component] << '\n';
    std::cout << component_prefix << ".canonical_effective_weight="
              << diagnostic.vicreg_component_effective_weight[component]
              << '\n';
    std::cout << component_prefix << ".canonical_effective_trunk_norm="
              << diagnostic.vicreg_component_effective_trunk_norm[component]
              << '\n';
    std::cout << component_prefix << ".canonical_effective_head_norm="
              << diagnostic.vicreg_component_effective_head_norm[component]
              << '\n';
  }
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    const auto index = static_cast<std::size_t>(channel);
    const std::string floor_prefix =
        prefix + ".vicreg_variance_floor.channel_" + std::to_string(channel);
    std::cout << floor_prefix << ".joint_valid_rows="
              << diagnostic.vicreg_variance_floor_valid_rows[index] << '\n';
    std::cout << floor_prefix << ".view_a_fraction_below_floor="
              << diagnostic.vicreg_view_a_variance_floor_fraction[index]
              << '\n';
    std::cout << floor_prefix << ".view_b_fraction_below_floor="
              << diagnostic.vicreg_view_b_variance_floor_fraction[index]
              << '\n';
  }
  std::size_t cosine_index = 0;
  for (std::size_t left = 0; left < kAttributionBranchNames.size(); ++left) {
    for (std::size_t right = left + 1; right < kAttributionBranchNames.size();
         ++right) {
      const double cosine = diagnostic.served_cosine[cosine_index++];
      const std::string cosine_prefix = prefix + ".cosine_" +
                                        kAttributionBranchNames[left] + "__" +
                                        kAttributionBranchNames[right];
      std::cout << cosine_prefix << ".valid=" << std::isfinite(cosine) << '\n';
      std::cout << cosine_prefix
                << ".value=" << (std::isfinite(cosine) ? cosine : 0.0) << '\n';
    }
  }
  std::cout << prefix << ".jepa_mae_norm=" << diagnostic.jepa_mae_norm << '\n';
  std::cout << prefix << ".tf_vicreg_norm=" << diagnostic.tf_vicreg_norm
            << '\n';
  std::cout << prefix
            << ".canonical_total_norm=" << diagnostic.canonical_total_norm
            << '\n';
  std::cout << prefix << ".cancellation_ratio=" << diagnostic.cancellation_ratio
            << '\n';
  std::cout << prefix << ".tf_weighted_served_norm_ratio="
            << diagnostic.tf_weighted_served_norm_ratio << '\n';
  std::cout << prefix << ".actual_arm_weighted_cancellation_ratio="
            << diagnostic.actual_arm_weighted_cancellation_ratio << '\n';
  std::cout << prefix << ".actual_arm_all_trainable_norm="
            << diagnostic.actual_arm_all_trainable_norm << '\n';
  std::cout << prefix
            << ".actual_arm_served_norm=" << diagnostic.actual_arm_served_norm
            << '\n';
  std::cout << prefix << ".all_trainable_relative_decomposition_error="
            << diagnostic.all_trainable_relative_decomposition_error << '\n';
  std::cout << prefix << ".served_relative_decomposition_error="
            << diagnostic.served_relative_decomposition_error << '\n';
  std::cout << prefix << ".vicreg_component_trunk_relative_decomposition_error="
            << diagnostic.vicreg_component_trunk_relative_decomposition_error
            << '\n';
  std::cout << prefix << ".vicreg_component_head_relative_decomposition_error="
            << diagnostic.vicreg_component_head_relative_decomposition_error
            << '\n';
  std::cout
      << prefix
      << ".vicreg_component_single_graph_trunk_relative_decomposition_error="
      << diagnostic
             .vicreg_component_single_graph_trunk_relative_decomposition_error
      << '\n';
  std::cout
      << prefix
      << ".vicreg_component_single_graph_head_relative_decomposition_error="
      << diagnostic
             .vicreg_component_single_graph_head_relative_decomposition_error
      << '\n';
  std::cout << prefix << ".vicreg_component_separate_sum_trunk_relative_error="
            << diagnostic.vicreg_component_separate_sum_trunk_relative_error
            << '\n';
  std::cout << prefix << ".vicreg_component_separate_sum_head_relative_error="
            << diagnostic.vicreg_component_separate_sum_head_relative_error
            << '\n';
  std::cout << prefix << ".vicreg_channel_active_groups="
            << diagnostic.vicreg_channel_active_groups << '\n';
  std::cout << prefix << ".vicreg_channel_valid_rows="
            << diagnostic.vicreg_channel_valid_rows << '\n';
  std::cout << prefix << ".neutrality.repeated_weak_views_exact="
            << diagnostic.repeated_weak_views_exact << '\n';
  std::cout << prefix << ".neutrality.parameters_and_ema_exact="
            << diagnostic.parameters_and_ema_exact << '\n';
  std::cout << prefix << ".neutrality.training_state_exact="
            << diagnostic.training_state_exact << '\n';
  std::cout << prefix << ".neutrality.optimizer_state_checked="
            << diagnostic.optimizer_state_checked << '\n';
  std::cout << prefix << ".neutrality.optimizer_state_exact="
            << diagnostic.optimizer_state_exact << '\n';
}

void emit_attribution_checkpoint(const std::string &arm_prefix,
                                 const AttributionCheckpoint &checkpoint) {
  const std::string prefix =
      arm_prefix + ".step_" + std::to_string(checkpoint.step);
  std::cout << prefix << ".probe.area=" << checkpoint.probe.area << '\n';
  const auto &final = checkpoint.probe.points.back().score;
  std::cout << prefix << ".probe.final_macro_r2=" << final.macro << '\n';
  for (int64_t family = 0; family < kFamilies; ++family) {
    std::cout << prefix << ".probe.family_"
              << kFamilyNames[static_cast<std::size_t>(family)]
              << "_r2=" << final.family[static_cast<std::size_t>(family)]
              << '\n';
  }
  emit_geometry(prefix + ".geometry", checkpoint.geometry);
  std::cout << prefix
            << ".vicreg_geometry.clean_global_preprojector.feature_dim=32\n";
  emit_geometry_summary(prefix + ".vicreg_geometry.clean_global_preprojector",
                        checkpoint.vicreg_clean_global_preprojector_geometry,
                        /*emit_passed=*/false);
  std::cout << prefix
            << ".vicreg_geometry.clean_projected_per_channel.feature_dim=64\n";
  emit_geometry(prefix + ".vicreg_geometry.clean_projected_per_channel",
                checkpoint.vicreg_clean_projected_channel_geometry,
                /*emit_passed=*/false);
  emit_gradient_diagnostic(prefix + ".gradient", checkpoint.gradients);
}

void emit_mask_hash(const std::string &key_name, uint64_t value) {
  const auto flags = std::cout.flags();
  std::cout << key_name << "=0x" << std::hex << value << '\n';
  std::cout.flags(flags);
}

void emit_fingerprint(const std::string &key_name, uint64_t value) {
  const auto flags = std::cout.flags();
  const auto fill = std::cout.fill();
  std::cout << key_name << '=' << std::hex << std::nouppercase
            << std::setfill('0') << std::setw(16) << value << '\n';
  std::cout.flags(flags);
  std::cout.fill(fill);
}

void emit_prefixed_manifest(const std::string &prefix,
                            const std::string &manifest) {
  std::istringstream input(manifest);
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line.find('=') == std::string::npos) {
      throw std::runtime_error("outer augmentation manifest line failed");
    }
    std::cout << prefix << '.' << line << '\n';
  }
}

void emit_generator_state_digest(const std::string &prefix,
                                 const GeneratorStateDigest &digest) {
  emit_mask_hash(prefix + ".cpu_hash", digest.cpu);
  emit_mask_hash(prefix + ".cuda0_hash", digest.cuda);
}

void emit_outer_augmentation_arm_diagnostics(
    const AttributionArmResult &result,
    const mtf::mtf_jepa_mae_vicreg_config_t &preprocessing_config) {
  const std::string prefix = "outer_augmentation.seed_" +
                             std::to_string(result.seed) + ".arm." +
                             result.arm.name;
  emit_fingerprint(prefix + ".model_config_fingerprint",
                   result.training.model_config_fingerprint);
  emit_fingerprint(prefix + ".preprocessing_config_fingerprint",
                   result.training.preprocessing_config_fingerprint);
  emit_prefixed_manifest(
      prefix + ".preprocessing_manifest",
      canonical_preprocessing_manifest(preprocessing_config));
  std::cout << prefix << ".augmentation_calls_per_update=2\n";
  std::cout << prefix << ".served_augmentation_calls_per_update=1\n";
  std::cout << prefix << ".discarded_replay_calls_per_update=1\n";
  std::cout << prefix << ".clean_checkpoint_augmentation_calls=0\n";
  for (int64_t step = 0; step < kAttributionSteps; ++step) {
    const auto &update =
        result.training
            .outer_augmentation_updates[static_cast<std::size_t>(step)];
    const std::string update_prefix =
        prefix + ".zero_based_update_" + std::to_string(step);
    std::cout << update_prefix << ".augmentation_seed=" << update.seed << '\n';
    emit_mask_hash(update_prefix + ".clean_data_hash", update.clean_data_hash);
    emit_mask_hash(update_prefix + ".clean_mask_hash", update.clean_mask_hash);
    emit_mask_hash(update_prefix + ".served_data_hash",
                   update.served_data_hash);
    emit_mask_hash(update_prefix + ".served_mask_hash",
                   update.served_mask_hash);
    emit_mask_hash(update_prefix + ".replay_data_hash",
                   update.replay_data_hash);
    emit_mask_hash(update_prefix + ".replay_mask_hash",
                   update.replay_mask_hash);
    emit_mask_hash(update_prefix + ".actual_forward_data_hash",
                   update.actual_forward_data_hash);
    emit_mask_hash(update_prefix + ".actual_forward_mask_hash",
                   update.actual_forward_mask_hash);
    emit_generator_state_digest(update_prefix + ".augmentation_original_state",
                                update.augmentation_original);
    emit_generator_state_digest(update_prefix +
                                    ".augmentation_served_consumed_state",
                                update.augmentation_served_consumed);
    emit_generator_state_digest(update_prefix +
                                    ".augmentation_replay_consumed_state",
                                update.augmentation_replay_consumed);
    emit_generator_state_digest(update_prefix + ".augmentation_restored_state",
                                update.augmentation_restored);
    emit_generator_state_digest(update_prefix + ".augmented_preview_post_state",
                                update.augmented_preview_post);
    emit_generator_state_digest(update_prefix +
                                    ".augmented_preview_replay_post_state",
                                update.augmented_preview_replay_post);
    emit_generator_state_digest(update_prefix + ".module_forward_pre_state",
                                update.module_forward_pre);
    emit_generator_state_digest(update_prefix + ".module_forward_post_state",
                                update.module_forward_post);
    std::cout << update_prefix << ".augmentation_replay_exact="
              << update.augmentation_replay_exact << '\n';
    std::cout << update_prefix << ".augmentation_consumed_state_exact="
              << update.augmentation_consumed_state_exact << '\n';
    std::cout << update_prefix << ".augmentation_cuda0_unchanged="
              << update.augmentation_cuda_unchanged << '\n';
    std::cout << update_prefix << ".augmentation_state_restored="
              << update.augmentation_state_restored << '\n';
    std::cout << update_prefix
              << ".neutral_identity_exact=" << update.neutral_identity_exact
              << '\n';
    std::cout << update_prefix
              << ".qualified_data_changed=" << update.qualified_data_changed
              << '\n';
    std::cout << update_prefix
              << ".qualified_mask_exact=" << update.qualified_mask_exact
              << '\n';
    std::cout << update_prefix
              << ".masked_values_zero=" << update.masked_values_zero << '\n';
    std::cout << update_prefix
              << ".preview_replay_exact=" << update.preview_replay_exact
              << '\n';
    std::cout << update_prefix << ".support_counterfactual_exact="
              << update.support_counterfactual_exact << '\n';
    std::cout << update_prefix << ".actual_masks_match_preview="
              << update.actual_masks_match_preview << '\n';
    std::cout << update_prefix << ".actual_input_matches_served="
              << update.actual_input_matches_served << '\n';
    const auto &retention = update.retention;
    std::cout << update_prefix << ".retention.overall=" << retention.overall
              << '\n';
    std::cout << update_prefix << ".retention.terminal=" << retention.terminal
              << '\n';
    std::cout << update_prefix
              << ".retention.clean_valid=" << retention.clean_valid << '\n';
    std::cout << update_prefix
              << ".retention.augmented_valid=" << retention.augmented_valid
              << '\n';
    std::cout << update_prefix
              << ".retention.preserved_cells=" << retention.preserved << '\n';
    std::cout << update_prefix << ".retention.added_cells=" << retention.added
              << '\n';
    std::cout << update_prefix
              << ".retention.removed_cells=" << retention.removed << '\n';
    std::cout << update_prefix << ".retention.every_sample_channel_nonempty="
              << retention.every_sample_channel_nonempty << '\n';
    for (int64_t channel = 0; channel < kChannels; ++channel) {
      const auto channel_index = static_cast<std::size_t>(channel);
      const std::string channel_prefix =
          update_prefix + ".retention.channel_" + std::to_string(channel);
      std::cout << channel_prefix
                << ".overall=" << retention.channel[channel_index] << '\n';
      std::cout << channel_prefix
                << ".terminal=" << retention.terminal_channel[channel_index]
                << '\n';
      std::cout << channel_prefix << ".clean_valid="
                << retention.clean_valid_channel[channel_index] << '\n';
      std::cout << channel_prefix << ".augmented_valid="
                << retention.augmented_valid_channel[channel_index] << '\n';
      std::cout << channel_prefix << ".clean_valid_terminal="
                << retention.clean_valid_terminal_channel[channel_index]
                << '\n';
      std::cout << channel_prefix << ".augmented_valid_terminal="
                << retention.augmented_valid_terminal_channel[channel_index]
                << '\n';
      std::cout << channel_prefix << ".preserved_clean_valid_terminal="
                << retention.preserved_valid_terminal_channel[channel_index]
                << '\n';
    }
  }
}

void emit_outer_seed_pairing(int64_t seed,
                             const OuterSeedPairingResult &pairing,
                             const std::vector<AttributionArmResult> &results,
                             std::size_t seed_begin) {
  const std::string prefix =
      "outer_augmentation.summary.seed_" + std::to_string(seed) + ".pairing";
  std::cout << prefix << ".batch_rows_exact=" << pairing.batch_rows_exact
            << '\n';
  std::cout << prefix
            << ".batch_row_hashes_exact=" << pairing.batch_row_hashes_exact
            << '\n';
  std::cout << prefix
            << ".clean_cpu_inputs_exact=" << pairing.clean_cpu_inputs_exact
            << '\n';
  std::cout << prefix
            << ".clean_input_hashes_exact=" << pairing.clean_input_hashes_exact
            << '\n';
  std::cout << prefix
            << ".clean_base_masks_exact=" << pairing.clean_base_masks_exact
            << '\n';
  std::cout << prefix << ".neutral_qualified_actual_masks_exact="
            << pairing.neutral_qualified_actual_masks_exact << '\n';
  std::cout << prefix << ".neutral_qualified_weak_feature_masks_exact="
            << pairing.neutral_qualified_weak_feature_masks_exact << '\n';
  std::cout << prefix << ".module_forward_pre_state_all_arms_exact="
            << pairing.module_forward_pre_state_all_arms_exact << '\n';
  std::cout << prefix << ".neutral_qualified_post_state_exact="
            << pairing.neutral_qualified_post_state_exact << '\n';
  std::cout << prefix << ".model_config_fingerprints_exact="
            << pairing.model_config_fingerprints_exact << '\n';
  std::cout << prefix
            << ".per_update_mechanics=" << pairing.per_update_mechanics << '\n';
  std::cout << prefix << ".pass=" << pairing.pass << '\n';
  const auto &neutral = results[seed_begin].training;
  const auto &full = results[seed_begin + kOuterFullActiveIndex].training;
  for (int64_t step = 0; step < kAttributionSteps; ++step) {
    const auto index = static_cast<std::size_t>(step);
    const auto target_xor = neutral.target_masks[index]
                                .logical_xor(full.target_masks[index])
                                .sum()
                                .item<int64_t>();
    const auto context_xor = neutral.context_masks[index]
                                 .logical_xor(full.context_masks[index])
                                 .sum()
                                 .item<int64_t>();
    const std::string update_prefix =
        prefix + ".zero_based_update_" + std::to_string(step);
    std::cout << update_prefix
              << ".full_active_target_mask_xor_count=" << target_xor << '\n';
    std::cout << update_prefix
              << ".full_active_context_mask_xor_count=" << context_xor << '\n';
    emit_generator_state_digest(
        update_prefix + ".full_active_post_state_descriptive",
        full.outer_augmentation_updates[index].module_forward_post);
  }
}

[[nodiscard]] double
probe_curve_prediction_max_abs_diff(const ProbeCurve &left,
                                    const ProbeCurve &right) {
  if (left.points.size() != right.points.size() || left.points.empty()) {
    throw std::runtime_error("attribution step-zero probe ladder mismatch");
  }
  double maximum = 0.0;
  for (std::size_t index = 0; index < left.points.size(); ++index) {
    if (left.points[index].samples != right.points[index].samples ||
        left.points[index].prediction.sizes() !=
            right.points[index].prediction.sizes()) {
      throw std::runtime_error("attribution step-zero probe shape mismatch");
    }
    maximum = std::max(maximum, (left.points[index].prediction -
                                 right.points[index].prediction)
                                    .abs()
                                    .max()
                                    .item<double>());
  }
  return maximum;
}

[[nodiscard]] bool probe_curve_selected_alphas_equal(const ProbeCurve &left,
                                                     const ProbeCurve &right) {
  if (left.points.size() != right.points.size()) {
    return false;
  }
  for (std::size_t point = 0; point < left.points.size(); ++point) {
    if (left.points[point].selected_alpha !=
        right.points[point].selected_alpha) {
      return false;
    }
  }
  return true;
}

struct AttributionBootstrap {
  Interval interval{};
  int64_t positive_seed_count{0};
};

[[nodiscard]] AttributionBootstrap
paired_arm_bootstrap(const std::vector<ProbeCurve> &candidate_curves,
                     const std::vector<ProbeCurve> &reference_curves,
                     const torch::Tensor &test_target, int64_t replicates,
                     uint64_t bootstrap_seed) {
  if (candidate_curves.size() != kAttributionSeeds.size() ||
      reference_curves.size() != kAttributionSeeds.size() || replicates <= 0 ||
      test_target.dim() != 2 || test_target.size(1) != kTargets) {
    throw std::runtime_error("attribution paired-bootstrap contract failed");
  }
  AttributionBootstrap result{};
  for (std::size_t seed = 0; seed < candidate_curves.size(); ++seed) {
    result.positive_seed_count +=
        candidate_curves[seed].area - reference_curves[seed].area > 0.0 ? 1 : 0;
  }
  const int64_t groups = test_target.size(0);
  const std::size_t ladder_points = reference_curves.front().points.size();
  std::vector<double> contrasts;
  contrasts.reserve(static_cast<std::size_t>(replicates));
  for (int64_t replicate = 0; replicate < replicates; ++replicate) {
    uint64_t state = splitmix64(bootstrap_seed ^
                                splitmix64(static_cast<uint64_t>(replicate)));
    std::vector<int64_t> sampled_rows;
    sampled_rows.reserve(static_cast<std::size_t>(groups));
    for (int64_t draw = 0; draw < groups; ++draw) {
      state = splitmix64(state);
      sampled_rows.push_back(
          static_cast<int64_t>(state % static_cast<uint64_t>(groups)));
    }
    const auto row_index = torch::tensor(sampled_rows, torch::kInt64);
    const auto resampled_target = test_target.index_select(0, row_index);
    double contrast = 0.0;
    for (std::size_t seed = 0; seed < candidate_curves.size(); ++seed) {
      const double candidate =
          resampled_curve_area(candidate_curves[seed], resampled_target,
                               row_index, groups, ladder_points);
      const double reference =
          resampled_curve_area(reference_curves[seed], resampled_target,
                               row_index, groups, ladder_points);
      contrast += candidate - reference;
    }
    contrasts.push_back(contrast /
                        static_cast<double>(candidate_curves.size()));
  }
  result.interval = percentile_interval(std::move(contrasts));
  return result;
}

struct RepairArmContrast {
  repair_gate::PairedContrast summary{};
  std::array<double, repair_gate::kRepairSeedCount> per_seed{};
};

[[nodiscard]] RepairArmContrast
repair_arm_contrast(const std::vector<AttributionArmResult> &results,
                    std::size_t arm_count, std::size_t candidate_index,
                    std::size_t reference_index,
                    const torch::Tensor &test_target) {
  if (candidate_index >= arm_count || reference_index >= arm_count ||
      results.size() != kAttributionSeeds.size() * arm_count) {
    throw std::runtime_error("repair arm contrast indexing contract failed");
  }
  std::vector<ProbeCurve> candidate_curves;
  std::vector<ProbeCurve> reference_curves;
  candidate_curves.reserve(kAttributionSeeds.size());
  reference_curves.reserve(kAttributionSeeds.size());
  RepairArmContrast result{};
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    const auto &candidate =
        results[seed * arm_count + candidate_index].checkpoints.back().probe;
    const auto &reference =
        results[seed * arm_count + reference_index].checkpoints.back().probe;
    candidate_curves.push_back(candidate);
    reference_curves.push_back(reference);
    result.per_seed[seed] = candidate.area - reference.area;
    result.summary.point += result.per_seed[seed];
  }
  result.summary.point /= static_cast<double>(kAttributionSeeds.size());
  const auto bootstrap = paired_arm_bootstrap(
      candidate_curves, reference_curves, test_target,
      kAttributionBootstrapReplicates, kRepairPrimaryBootstrapSeed);
  result.summary.low = bootstrap.interval.low;
  result.summary.high = bootstrap.interval.high;
  result.summary.positive_seed_count = bootstrap.positive_seed_count;
  return result;
}

struct JmcdFactorialContrast {
  jmcd_gate::PairedContrast summary{};
  std::array<double, jmcd_gate::kSeedCount> per_seed{};
};

[[nodiscard]] JmcdFactorialContrast
jmcd_factorial_contrast(const std::vector<AttributionArmResult> &results,
                        const torch::Tensor &test_target) {
  if (results.size() != kAttributionSeeds.size() * kJmcdArms.size() ||
      test_target.dim() != 2 || test_target.size(1) != kTargets) {
    throw std::runtime_error("JMCD factorial contrast contract failed");
  }
  JmcdFactorialContrast result{};
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    const auto area = [&](std::size_t arm) {
      return results[seed * kJmcdArms.size() + arm]
          .checkpoints.back()
          .probe.area;
    };
    result.per_seed[seed] = area(kJmcdJepaIndex) + area(kJmcdMaeIndex) -
                            area(kJmcdNullIndex) - area(kJmcdCombinedIndex);
    result.summary.point += result.per_seed[seed];
    result.summary.positive_seed_count += result.per_seed[seed] > 0.0 ? 1 : 0;
  }
  result.summary.point /= static_cast<double>(kAttributionSeeds.size());

  const int64_t groups = test_target.size(0);
  const std::size_t ladder_points =
      results.front().checkpoints.back().probe.points.size();
  std::vector<double> contrasts;
  contrasts.reserve(kAttributionBootstrapReplicates);
  for (int64_t replicate = 0; replicate < kAttributionBootstrapReplicates;
       ++replicate) {
    uint64_t state = splitmix64(kRepairPrimaryBootstrapSeed ^
                                splitmix64(static_cast<uint64_t>(replicate)));
    std::vector<int64_t> sampled_rows;
    sampled_rows.reserve(static_cast<std::size_t>(groups));
    for (int64_t draw = 0; draw < groups; ++draw) {
      state = splitmix64(state);
      sampled_rows.push_back(
          static_cast<int64_t>(state % static_cast<uint64_t>(groups)));
    }
    const auto row_index = torch::tensor(sampled_rows, torch::kInt64);
    const auto resampled_target = test_target.index_select(0, row_index);
    double fixed_seed_mean = 0.0;
    for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
      const auto resampled_area = [&](std::size_t arm) {
        return resampled_curve_area(
            results[seed * kJmcdArms.size() + arm].checkpoints.back().probe,
            resampled_target, row_index, groups, ladder_points);
      };
      fixed_seed_mean +=
          resampled_area(kJmcdJepaIndex) + resampled_area(kJmcdMaeIndex) -
          resampled_area(kJmcdNullIndex) - resampled_area(kJmcdCombinedIndex);
    }
    contrasts.push_back(fixed_seed_mean /
                        static_cast<double>(kAttributionSeeds.size()));
  }
  const auto interval = percentile_interval(std::move(contrasts));
  result.summary.low = interval.low;
  result.summary.high = interval.high;
  return result;
}

[[nodiscard]] repair_gate::GeometryBySeed
repair_geometry_by_seed(const std::vector<AttributionArmResult> &results,
                        std::size_t arm_count, std::size_t arm_index) {
  repair_gate::GeometryBySeed geometry{};
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    const auto &checkpoint =
        results[seed * arm_count + arm_index].checkpoints.back();
    auto &mapped = geometry[seed];
    mapped.top = 0.0;
    mapped.active = 1.0;
    for (const auto &channel : checkpoint.geometry) {
      mapped.effective += channel.effective_rank_ratio;
      mapped.participation += channel.participation_rank_ratio;
      mapped.top = std::max(mapped.top, channel.top_eigenvalue_share);
      mapped.active =
          std::min(mapped.active, channel.active_dimension_fraction);
    }
    mapped.effective /= static_cast<double>(kChannels);
    mapped.participation /= static_cast<double>(kChannels);
  }
  return geometry;
}

[[nodiscard]] repair_gate::FamilyDeltas
repair_family_deltas(const std::vector<AttributionArmResult> &results,
                     std::size_t arm_count, std::size_t candidate_index,
                     std::size_t reference_index) {
  repair_gate::FamilyDeltas deltas{};
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    const auto &candidate = results[seed * arm_count + candidate_index]
                                .checkpoints.back()
                                .probe.points.back()
                                .score.family;
    const auto &reference = results[seed * arm_count + reference_index]
                                .checkpoints.back()
                                .probe.points.back()
                                .score.family;
    for (std::size_t family = 0; family < deltas.size(); ++family) {
      deltas[family] += candidate[family] - reference[family];
    }
  }
  for (auto &delta : deltas) {
    delta /= static_cast<double>(kAttributionSeeds.size());
  }
  return deltas;
}

[[nodiscard]] jmcd_gate::PairedContrast
jmcd_contrast(const RepairArmContrast &contrast) {
  return {.point = contrast.summary.point,
          .low = contrast.summary.low,
          .high = contrast.summary.high,
          .positive_seed_count = contrast.summary.positive_seed_count};
}

[[nodiscard]] jmcd_gate::GeometryBySeed
jmcd_geometry_by_seed(const std::vector<AttributionArmResult> &results,
                      std::size_t arm_index) {
  const auto source =
      repair_geometry_by_seed(results, kJmcdArms.size(), arm_index);
  jmcd_gate::GeometryBySeed mapped{};
  for (std::size_t seed = 0; seed < mapped.size(); ++seed) {
    mapped[seed] = {.effective = source[seed].effective,
                    .participation = source[seed].participation,
                    .top = source[seed].top,
                    .active = source[seed].active};
  }
  return mapped;
}

[[nodiscard]] jmcd_gate::FamilyDeltas
jmcd_family_deltas(const std::vector<AttributionArmResult> &results,
                   std::size_t candidate_index, std::size_t reference_index) {
  const auto source = repair_family_deltas(results, kJmcdArms.size(),
                                           candidate_index, reference_index);
  jmcd_gate::FamilyDeltas mapped{};
  std::copy(source.begin(), source.end(), mapped.begin());
  return mapped;
}

[[nodiscard]] repair_gate::TfRatios
repair_tf_step_zero_ratios(const std::vector<AttributionArmResult> &results,
                           std::size_t arm_count) {
  repair_gate::TfRatios ratios{};
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    ratios[seed] = results[seed * arm_count + kRepairWarmupIndex]
                       .checkpoints.front()
                       .gradients.tf_weighted_served_norm_ratio;
  }
  return ratios;
}

[[nodiscard]] bool
repair_mechanics_complete(const std::vector<AttributionArmResult> &results,
                          std::size_t arm_count) {
  if (results.size() != kAttributionSeeds.size() * arm_count) {
    return false;
  }
  for (const auto &result : results) {
    if (static_cast<int64_t>(result.training.clip_factors.size()) !=
            kAttributionSteps ||
        static_cast<int64_t>(result.training.served_update_norms.size()) !=
            kAttributionSteps ||
        static_cast<int64_t>(result.training.target_masks.size()) !=
            kAttributionSteps ||
        static_cast<int64_t>(result.training.context_masks.size()) !=
            kAttributionSteps ||
        result.checkpoints.size() != 3) {
      return false;
    }
    for (std::size_t update = 0; update < result.training.clip_factors.size();
         ++update) {
      if (result.training.clip_factors[update] != 1.0 ||
          !std::isfinite(result.training.served_update_norms[update])) {
        return false;
      }
    }
    for (const auto &checkpoint : result.checkpoints) {
      const auto &diagnostic = checkpoint.gradients;
      if (!diagnostic.repeated_weak_views_exact ||
          !diagnostic.parameters_and_ema_exact ||
          !diagnostic.training_state_exact ||
          !diagnostic.optimizer_state_exact) {
        return false;
      }
    }
  }
  return true;
}

struct JmcdSeedPairing {
  bool complete{true};
  bool rows_exact{true};
  bool row_hashes_exact{true};
  bool clean_hashes_exact{true};
  bool target_masks_exact{true};
  bool context_masks_exact{true};
  bool weak_views_exact{true};
  bool forward_pre_states_exact{true};
  bool forward_post_states_exact{true};
  bool common_config_exact{true};
  bool objective_configs_distinct{true};
  bool pass{true};
};

[[nodiscard]] JmcdSeedPairing
validate_jmcd_seed_pairing(const std::vector<AttributionArmResult> &results,
                           std::size_t seed_begin,
                           const torch::Device &device) {
  if (seed_begin + kJmcdArms.size() > results.size()) {
    throw std::runtime_error("JMCD seed result range failed");
  }
  JmcdSeedPairing pairing{};
  const auto &reference = results[seed_begin].training;
  const auto complete = [](const AttributionTraining &training) {
    return training.batch_rows.size() == kAttributionSteps &&
           training.batch_row_hashes.size() == kAttributionSteps &&
           training.clean_data_hashes.size() == kAttributionSteps &&
           training.clean_mask_hashes.size() == kAttributionSteps &&
           training.target_masks.size() == kAttributionSteps &&
           training.context_masks.size() == kAttributionSteps &&
           training.weak_view_digests.size() == kAttributionSteps &&
           training.module_forward_pre_states.size() == kAttributionSteps &&
           training.module_forward_post_states.size() == kAttributionSteps;
  };
  std::string common_manifest;
  std::set<std::string> full_manifests;
  for (std::size_t arm = 0; arm < kJmcdArms.size(); ++arm) {
    const auto &training = results[seed_begin + arm].training;
    pairing.complete = pairing.complete && complete(training);
    const auto config =
        attribution_config(device, results[seed_begin + arm].arm);
    const auto full = canonical_config_manifest(config);
    const auto common = jmcd_common_config_manifest(config);
    full_manifests.insert(full);
    if (arm == 0) {
      common_manifest = common;
    } else {
      pairing.common_config_exact =
          pairing.common_config_exact && common == common_manifest;
    }
  }
  pairing.objective_configs_distinct =
      full_manifests.size() == kJmcdArms.size();
  if (!pairing.complete) {
    pairing.pass = false;
    return pairing;
  }
  for (int64_t step = 0; step < kAttributionSteps; ++step) {
    const auto index = static_cast<std::size_t>(step);
    for (std::size_t arm = 1; arm < kJmcdArms.size(); ++arm) {
      const auto &candidate = results[seed_begin + arm].training;
      pairing.rows_exact =
          pairing.rows_exact &&
          candidate.batch_rows[index] == reference.batch_rows[index];
      pairing.row_hashes_exact =
          pairing.row_hashes_exact && candidate.batch_row_hashes[index] ==
                                          reference.batch_row_hashes[index];
      pairing.clean_hashes_exact = pairing.clean_hashes_exact &&
                                   candidate.clean_data_hashes[index] ==
                                       reference.clean_data_hashes[index] &&
                                   candidate.clean_mask_hashes[index] ==
                                       reference.clean_mask_hashes[index];
      pairing.target_masks_exact = pairing.target_masks_exact &&
                                   torch::equal(candidate.target_masks[index],
                                                reference.target_masks[index]);
      pairing.context_masks_exact =
          pairing.context_masks_exact &&
          torch::equal(candidate.context_masks[index],
                       reference.context_masks[index]);
      pairing.weak_views_exact =
          pairing.weak_views_exact &&
          weak_view_digests_equal(candidate.weak_view_digests[index],
                                  reference.weak_view_digests[index]);
      pairing.forward_pre_states_exact =
          pairing.forward_pre_states_exact &&
          generator_state_snapshot_equal(
              candidate.module_forward_pre_states[index],
              reference.module_forward_pre_states[index]);
      pairing.forward_post_states_exact =
          pairing.forward_post_states_exact &&
          generator_state_snapshot_equal(
              candidate.module_forward_post_states[index],
              reference.module_forward_post_states[index]);
    }
  }
  pairing.pass =
      pairing.complete && pairing.rows_exact && pairing.row_hashes_exact &&
      pairing.clean_hashes_exact && pairing.target_masks_exact &&
      pairing.context_masks_exact && pairing.weak_views_exact &&
      pairing.forward_pre_states_exact && pairing.forward_post_states_exact &&
      pairing.common_config_exact && pairing.objective_configs_distinct;
  return pairing;
}

[[nodiscard]] bool jmcd_null_identity(const AttributionArmResult &result) {
  if (std::string(result.arm.name) != "core_objective_null" ||
      result.checkpoints.size() != 3 ||
      result.final_all_trainable_max_abs_diff != 0.0 ||
      result.final_served_max_abs_diff != 0.0 ||
      result.final_predictor_max_abs_diff != 0.0 ||
      result.final_mae_decoder_max_abs_diff != 0.0 ||
      result.final_vicreg_head_max_abs_diff != 0.0 ||
      !std::all_of(result.training.served_update_norms.begin(),
                   result.training.served_update_norms.end(),
                   [](double value) { return value == 0.0; })) {
    return false;
  }
  const auto &initial = result.checkpoints.front();
  const auto &terminal = result.checkpoints.back();
  return torch::equal(initial.test_embeddings, terminal.test_embeddings) &&
         initial.probe.area == terminal.probe.area &&
         probe_curve_prediction_max_abs_diff(initial.probe, terminal.probe) ==
             0.0 &&
         probe_curve_selected_alphas_equal(initial.probe, terminal.probe) &&
         geometry_exact(initial.geometry, terminal.geometry) &&
         geometry_exact(initial.vicreg_clean_global_preprojector_geometry,
                        terminal.vicreg_clean_global_preprojector_geometry) &&
         geometry_exact(initial.vicreg_clean_projected_channel_geometry,
                        terminal.vicreg_clean_projected_channel_geometry);
}

[[nodiscard]] bool jmcd_parameter_and_gradient_mechanics(
    const std::vector<AttributionArmResult> &results) {
  if (results.size() != kAttributionSeeds.size() * kJmcdArms.size()) {
    return false;
  }
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    const auto &combined =
        results[seed * kJmcdArms.size() + kJmcdCombinedIndex];
    const auto &jepa = results[seed * kJmcdArms.size() + kJmcdJepaIndex];
    const auto &mae = results[seed * kJmcdArms.size() + kJmcdMaeIndex];
    const auto &null_arm = results[seed * kJmcdArms.size() + kJmcdNullIndex];
    const bool parameter_contract =
        combined.final_served_max_abs_diff > 0.0 &&
        combined.final_predictor_max_abs_diff > 0.0 &&
        combined.final_mae_decoder_max_abs_diff > 0.0 &&
        combined.final_vicreg_head_max_abs_diff == 0.0 &&
        jepa.final_served_max_abs_diff > 0.0 &&
        jepa.final_predictor_max_abs_diff > 0.0 &&
        jepa.final_mae_decoder_max_abs_diff == 0.0 &&
        jepa.final_vicreg_head_max_abs_diff == 0.0 &&
        mae.final_served_max_abs_diff > 0.0 &&
        mae.final_predictor_max_abs_diff == 0.0 &&
        mae.final_mae_decoder_max_abs_diff > 0.0 &&
        mae.final_vicreg_head_max_abs_diff == 0.0 &&
        jmcd_null_identity(null_arm);
    if (!parameter_contract) {
      return false;
    }
    for (std::size_t arm = 0; arm < kJmcdArms.size(); ++arm) {
      const auto &result = results[seed * kJmcdArms.size() + arm];
      for (const auto &checkpoint : result.checkpoints) {
        const auto &gradient = checkpoint.gradients;
        const bool raw_contract =
            gradient.raw_loss[0] > 0.0 && gradient.raw_loss[1] > 0.0 &&
            gradient.served_norm[0] > 0.0 && gradient.served_norm[1] > 0.0 &&
            gradient.predictor_norm[0] > 0.0 &&
            gradient.mae_decoder_norm[1] > 0.0;
        const bool decomposition_contract =
            gradient.all_trainable_relative_decomposition_error <= 1.0e-5 &&
            gradient.served_relative_decomposition_error <= 1.0e-5;
        const bool active_contract =
            arm == kJmcdNullIndex
                ? gradient.actual_arm_all_trainable_norm == 0.0 &&
                      gradient.actual_arm_served_norm == 0.0
                : gradient.actual_arm_all_trainable_norm > 0.0 &&
                      gradient.actual_arm_served_norm > 0.0;
        if (!raw_contract || !decomposition_contract || !active_contract) {
          return false;
        }
      }
    }
  }
  return true;
}

[[nodiscard]] bool
jmcd_mechanics_complete(const std::vector<AttributionArmResult> &results,
                        const torch::Device &device) {
  if (!repair_mechanics_complete(results, kJmcdArms.size()) ||
      !jmcd_parameter_and_gradient_mechanics(results)) {
    return false;
  }
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    if (!validate_jmcd_seed_pairing(results, seed * kJmcdArms.size(), device)
             .pass) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] variance_gate::GeometryBySeed
variance_geometry_by_seed(const std::vector<AttributionArmResult> &results,
                          std::size_t arm_count, std::size_t arm_index) {
  variance_gate::GeometryBySeed geometry{};
  if (arm_index >= arm_count ||
      results.size() != kAttributionSeeds.size() * arm_count) {
    throw std::runtime_error("variance geometry indexing contract failed");
  }
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    const auto &checkpoint =
        results[seed * arm_count + arm_index].checkpoints.back();
    auto &mapped = geometry[seed];
    mapped.top = 0.0;
    mapped.active = 1.0;
    for (const auto &channel : checkpoint.geometry) {
      mapped.effective += channel.effective_rank_ratio;
      mapped.participation += channel.participation_rank_ratio;
      mapped.top = std::max(mapped.top, channel.top_eigenvalue_share);
      mapped.active =
          std::min(mapped.active, channel.active_dimension_fraction);
    }
    mapped.effective /= static_cast<double>(kChannels);
    mapped.participation /= static_cast<double>(kChannels);
  }
  return geometry;
}

[[nodiscard]] variance_gate::FamilyDeltas
variance_family_deltas(const std::vector<AttributionArmResult> &results,
                       std::size_t arm_count, std::size_t candidate_index,
                       std::size_t reference_index) {
  variance_gate::FamilyDeltas deltas{};
  if (candidate_index >= arm_count || reference_index >= arm_count ||
      results.size() != kAttributionSeeds.size() * arm_count) {
    throw std::runtime_error("variance family indexing contract failed");
  }
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    const auto &candidate = results[seed * arm_count + candidate_index]
                                .checkpoints.back()
                                .probe.points.back()
                                .score.family;
    const auto &reference = results[seed * arm_count + reference_index]
                                .checkpoints.back()
                                .probe.points.back()
                                .score.family;
    for (std::size_t family = 0; family < deltas.size(); ++family) {
      deltas[family] += candidate[family] - reference[family];
    }
  }
  for (auto &delta : deltas) {
    delta /= static_cast<double>(kAttributionSeeds.size());
  }
  return deltas;
}

[[nodiscard]] bool variance_checkpoint_mechanics_complete(
    const std::vector<AttributionArmResult> &results, std::size_t arm_count,
    const std::array<VarianceAblationMechanicalResult, kAttributionSeeds.size()>
        &ablation) {
  constexpr std::size_t kFullIndex = 1;
  constexpr std::size_t kNoVarianceIndex = 2;
  if (arm_count != kVarianceNecessityArms.size() ||
      !repair_mechanics_complete(results, arm_count)) {
    return false;
  }
  for (const auto &mechanic : ablation) {
    if (!mechanic.pass) {
      return false;
    }
  }
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    const auto &full = results[seed * arm_count + kFullIndex];
    const auto &no_variance = results[seed * arm_count + kNoVarianceIndex];
    if (full.arm.vicreg_var_weight != 25.0 ||
        no_variance.arm.vicreg_var_weight != 0.0) {
      return false;
    }
    for (std::size_t checkpoint = 0; checkpoint < full.checkpoints.size();
         ++checkpoint) {
      const auto &full_gradient = full.checkpoints[checkpoint].gradients;
      const auto &candidate_gradient =
          no_variance.checkpoints[checkpoint].gradients;
      if (full_gradient.vicreg_component_effective_weight !=
              std::array<double, 3>{0.3125, 0.3125, 0.0125} ||
          candidate_gradient.vicreg_component_effective_weight !=
              std::array<double, 3>{0.3125, 0.0, 0.0125} ||
          candidate_gradient.vicreg_component_effective_trunk_norm[1] != 0.0 ||
          candidate_gradient.vicreg_component_effective_head_norm[1] != 0.0 ||
          !std::isfinite(candidate_gradient.vicreg_component_raw_loss[1]) ||
          !std::isfinite(
              candidate_gradient.vicreg_component_raw_trunk_norm[1]) ||
          !std::isfinite(
              candidate_gradient.vicreg_component_raw_head_norm[1])) {
        return false;
      }
    }
  }
  return true;
}

void emit_variance_ablation_mechanics(
    int64_t seed, const VarianceAblationMechanicalResult &result) {
  const std::string prefix =
      "variance_necessity.seed_" + std::to_string(seed) + ".ablation";
  std::cout << prefix << ".only_variance_weight_changed="
            << result.only_variance_weight_changed << '\n';
  std::cout << prefix
            << ".initial_parameters_exact=" << result.initial_parameters_exact
            << '\n';
  std::cout << prefix << ".weak_views_exact=" << result.weak_views_exact
            << '\n';
  std::cout << prefix << ".raw_component_losses_exact="
            << result.raw_component_losses_exact << '\n';
  std::cout << prefix << ".raw_component_all_gradients_exact="
            << result.raw_component_all_gradients_exact << '\n';
  std::cout << prefix << ".raw_component_served_gradients_exact="
            << result.raw_component_served_gradients_exact << '\n';
  std::cout << prefix << ".raw_component_head_gradients_exact="
            << result.raw_component_head_gradients_exact << '\n';
  std::cout << prefix << ".common_branch_loss_and_gradients_exact="
            << result.common_branch_loss_and_gradients_exact << '\n';
  std::cout << prefix << ".raw_variance_finite_nonzero="
            << result.raw_variance_finite_nonzero << '\n';
  std::cout << prefix << ".no_variance_effective_weight_zero="
            << result.no_variance_effective_weight_zero << '\n';
  std::cout << prefix << ".inner_loss_difference_relative_error="
            << result.inner_loss_difference_relative_error << '\n';
  std::cout << prefix << ".total_loss_difference_relative_error="
            << result.total_loss_difference_relative_error << '\n';
  std::cout << prefix << ".total_all_gradient_difference_relative_error="
            << result.total_all_gradient_difference_relative_error << '\n';
  std::cout << prefix << ".total_served_gradient_difference_relative_error="
            << result.total_served_gradient_difference_relative_error << '\n';
  std::cout << prefix << ".total_head_gradient_difference_relative_error="
            << result.total_head_gradient_difference_relative_error << '\n';
  std::cout
      << prefix
      << ".direct_float32_total_all_gradient_difference_relative_error="
      << result.direct_float32_total_all_gradient_difference_relative_error
      << '\n';
  std::cout
      << prefix
      << ".direct_float32_total_served_gradient_difference_relative_error="
      << result.direct_float32_total_served_gradient_difference_relative_error
      << '\n';
  std::cout
      << prefix
      << ".direct_float32_total_head_gradient_difference_relative_error="
      << result.direct_float32_total_head_gradient_difference_relative_error
      << '\n';
  std::cout << prefix << ".pass=" << result.pass << '\n';
}

void emit_variance_arm_summaries(
    const std::vector<AttributionArmResult> &results,
    const std::vector<AttributionArm> &arms, std::size_t first_arm = 0,
    std::size_t arm_count = std::numeric_limits<std::size_t>::max(),
    const std::string &prefix_root = "summary.arm") {
  constexpr std::array<int64_t, 3> kSummarySteps{0, 16, 32};
  if (results.size() != kAttributionSeeds.size() * arms.size()) {
    throw std::runtime_error("variance summary indexing contract failed");
  }
  const auto last_arm = arm_count == std::numeric_limits<std::size_t>::max()
                            ? arms.size()
                            : first_arm + arm_count;
  if (first_arm > arms.size() || last_arm > arms.size() ||
      last_arm < first_arm) {
    throw std::runtime_error("variance summary arm range failed");
  }
  for (std::size_t arm_index = first_arm; arm_index < last_arm; ++arm_index) {
    for (std::size_t checkpoint_index = 0;
         checkpoint_index < kSummarySteps.size(); ++checkpoint_index) {
      double area_mean = 0.0;
      std::array<double, kFamilies> family_mean{};
      double channel_mean_effective_rank = 0.0;
      double channel_mean_participation_rank = 0.0;
      double channel_max_top_eigen_share = 0.0;
      double channel_min_active_fraction = 0.0;
      for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
        const auto &checkpoint = results[seed * arms.size() + arm_index]
                                     .checkpoints[checkpoint_index];
        area_mean += checkpoint.probe.area;
        const auto &score = checkpoint.probe.points.back().score;
        for (std::size_t family = 0; family < family_mean.size(); ++family) {
          family_mean[family] += score.family[family];
        }
        double seed_effective = 0.0;
        double seed_participation = 0.0;
        double seed_top = 0.0;
        double seed_active = 1.0;
        for (const auto &channel : checkpoint.geometry) {
          seed_effective += channel.effective_rank_ratio;
          seed_participation += channel.participation_rank_ratio;
          seed_top = std::max(seed_top, channel.top_eigenvalue_share);
          seed_active =
              std::min(seed_active, channel.active_dimension_fraction);
        }
        channel_mean_effective_rank +=
            seed_effective / static_cast<double>(kChannels);
        channel_mean_participation_rank +=
            seed_participation / static_cast<double>(kChannels);
        channel_max_top_eigen_share += seed_top;
        channel_min_active_fraction += seed_active;
      }
      const double denominator = static_cast<double>(kAttributionSeeds.size());
      const std::string prefix =
          prefix_root + "." + std::string(arms[arm_index].name) + ".step_" +
          std::to_string(kSummarySteps[checkpoint_index]);
      std::cout << prefix << ".optimizer_lambda_tf_align="
                << attribution_arm_weights(arms[arm_index],
                                           kSummarySteps[checkpoint_index])[2]
                << '\n';
      std::cout << prefix
                << ".probe_area_fixed_seed_mean=" << area_mean / denominator
                << '\n';
      for (std::size_t family = 0; family < family_mean.size(); ++family) {
        std::cout << prefix << ".family_" << kFamilyNames[family]
                  << "_r2_fixed_seed_mean=" << family_mean[family] / denominator
                  << '\n';
      }
      std::cout << prefix
                << ".geometry.channel_mean_effective_rank_ratio_fixed_seed_"
                   "mean="
                << channel_mean_effective_rank / denominator << '\n';
      std::cout << prefix
                << ".geometry.channel_mean_participation_rank_ratio_fixed_"
                   "seed_mean="
                << channel_mean_participation_rank / denominator << '\n';
      std::cout << prefix
                << ".geometry.channel_max_top_eigenvalue_share_fixed_seed_"
                   "mean="
                << channel_max_top_eigen_share / denominator << '\n';
      std::cout << prefix
                << ".geometry.channel_min_active_dimension_fraction_fixed_"
                   "seed_mean="
                << channel_min_active_fraction / denominator << '\n';
    }
  }
}

[[nodiscard]] outer_gate::GeometryBySeed
outer_geometry_by_seed(const std::vector<AttributionArmResult> &results,
                       std::size_t arm_index) {
  const auto source = repair_geometry_by_seed(
      results, kOuterAugmentationArms.size(), arm_index);
  outer_gate::GeometryBySeed mapped{};
  for (std::size_t seed = 0; seed < mapped.size(); ++seed) {
    mapped[seed] = {
        .effective = source[seed].effective,
        .participation = source[seed].participation,
        .top = source[seed].top,
        .active = source[seed].active,
    };
  }
  return mapped;
}

[[nodiscard]] outer_gate::FamilyDeltas
outer_family_deltas(const std::vector<AttributionArmResult> &results,
                    std::size_t candidate_index, std::size_t reference_index) {
  const auto source = repair_family_deltas(
      results, kOuterAugmentationArms.size(), candidate_index, reference_index);
  outer_gate::FamilyDeltas mapped{};
  std::copy(source.begin(), source.end(), mapped.begin());
  return mapped;
}

[[nodiscard]] bool
outer_mechanics_complete(const std::vector<AttributionArmResult> &results) {
  if (results.size() !=
      kAttributionSeeds.size() * kOuterAugmentationArms.size()) {
    return false;
  }
  bool pass = repair_mechanics_complete(results, kOuterAugmentationArms.size());
  std::array<uint64_t, kOuterAugmentationArms.size()>
      preprocessing_fingerprints{};
  uint64_t model_fingerprint = 0;
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    const auto seed_begin = seed * kOuterAugmentationArms.size();
    pass = pass && validate_outer_seed_pairing(results, seed_begin).pass;
    for (std::size_t arm = 0; arm < kOuterAugmentationArms.size(); ++arm) {
      const auto &result = results[seed_begin + arm];
      pass = pass && result.initial_parameter_max_abs_diff == 0.0 &&
             result.initial_embedding_max_abs_diff == 0.0 &&
             result.initial_probe_prediction_max_abs_diff == 0.0 &&
             result.checkpoints.size() == 3 &&
             result.training.outer_augmentation_updates.size() ==
                 static_cast<std::size_t>(kAttributionSteps) &&
             std::all_of(result.training.clip_factors.begin(),
                         result.training.clip_factors.end(),
                         [](double factor) { return factor == 1.0; });
      if (seed == 0) {
        preprocessing_fingerprints[arm] =
            result.training.preprocessing_config_fingerprint;
      } else {
        pass = pass && preprocessing_fingerprints[arm] ==
                           result.training.preprocessing_config_fingerprint;
      }
      if (seed == 0 && arm == 0) {
        model_fingerprint = result.training.model_config_fingerprint;
      } else {
        pass = pass &&
               model_fingerprint == result.training.model_config_fingerprint;
      }
    }
  }
  return pass;
}

void emit_outer_contrast(const std::string &name,
                         const RepairArmContrast &contrast) {
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    std::cout << "outer_augmentation.summary.seed_" << kAttributionSeeds[seed]
              << ".contrast." << name
              << ".step32_probe_area=" << contrast.per_seed[seed] << '\n';
  }
  const std::string prefix =
      "outer_augmentation.summary.contrast." + name + ".step32_probe_area";
  std::cout << prefix << ".fixed_seed_mean=" << contrast.summary.point << '\n';
  std::cout << prefix << ".bootstrap_95_low=" << contrast.summary.low << '\n';
  std::cout << prefix << ".bootstrap_95_high=" << contrast.summary.high << '\n';
  std::cout << prefix
            << ".positive_seed_count=" << contrast.summary.positive_seed_count
            << '\n';
}

void emit_outer_gap(const std::string &name,
                    const outer_gate::ActiveGapRepairResult &gap) {
  const std::string prefix =
      "outer_augmentation.summary.gate.geometry." + name + "_active_gap";
  std::cout << prefix << ".applicable=" << gap.applicable << '\n';
  std::cout << prefix << ".closure_defined=" << gap.closure_defined << '\n';
  std::cout << prefix << ".closure_finite=" << gap.closure_finite << '\n';
  std::cout << prefix
            << ".closure=" << (std::isfinite(gap.closure) ? gap.closure : 0.0)
            << '\n';
  std::cout << prefix << ".closure_pass=" << gap.closure_pass << '\n';
  std::cout << prefix
            << ".repair_direction_count=" << gap.repair_direction_count << '\n';
  std::cout << prefix << ".repair_direction_pass=" << gap.repair_direction_pass
            << '\n';
  std::cout << prefix << ".pass=" << gap.pass << '\n';
}

void emit_outer_gate(
    const outer_gate::OuterAugmentationTrainingGateResult &gate,
    const outer_gate::FamilyDeltas &qualified_minus_full_active_family,
    const outer_gate::FamilyDeltas &qualified_minus_neutral_family) {
  constexpr const char *prefix = "outer_augmentation.summary.gate";
  std::cout << prefix << ".classification="
            << outer_gate::classification_name(gate.classification) << '\n';
  std::cout << prefix
            << ".basic_numeric_inputs_valid=" << gate.basic_numeric_inputs_valid
            << '\n';
  std::cout << prefix << ".numeric_inputs_valid=" << gate.numeric_inputs_valid
            << '\n';
  std::cout << prefix << ".mechanics_pass=" << gate.mechanics_pass << '\n';
  std::cout << prefix << ".replacement.point=" << gate.replacement_point_pass
            << '\n';
  std::cout << prefix
            << ".replacement.lower_bound=" << gate.replacement_lower_bound_pass
            << '\n';
  std::cout << prefix << ".replacement.positive_seed_count="
            << gate.replacement_positive_seed_count_pass << '\n';
  std::cout << prefix
            << ".replacement.contrast=" << gate.replacement_contrast_pass
            << '\n';
  std::cout << prefix
            << ".neutral_noninferiority=" << gate.neutral_noninferiority_pass
            << '\n';
  for (std::size_t family = 0; family < outer_gate::kFamilyCount; ++family) {
    std::cout << prefix << ".family_delta.qualified_minus_full_active."
              << kFamilyNames[family] << '='
              << qualified_minus_full_active_family[family] << '\n';
    std::cout << prefix << ".family_clause.qualified_minus_full_active."
              << kFamilyNames[family] << '='
              << gate.qualified_minus_full_active_family_pass[family] << '\n';
    std::cout << prefix << ".family_delta.qualified_minus_neutral."
              << kFamilyNames[family] << '='
              << qualified_minus_neutral_family[family] << '\n';
    std::cout << prefix << ".family_clause.qualified_minus_neutral."
              << kFamilyNames[family] << '='
              << gate.qualified_minus_neutral_family_pass[family] << '\n';
  }
  std::cout << prefix << ".all_eight_family_deltas_pass="
            << gate.all_eight_family_deltas_pass << '\n';
  const auto &geometry = gate.geometry;
  const auto emit_geometry_mean = [&](const char *name,
                                      const outer_gate::PerSeedGeometry &mean) {
    const std::string mean_prefix =
        std::string(prefix) + ".geometry." + name + "_mean";
    std::cout << mean_prefix << ".effective=" << mean.effective << '\n';
    std::cout << mean_prefix << ".participation=" << mean.participation << '\n';
    std::cout << mean_prefix << ".top=" << mean.top << '\n';
    std::cout << mean_prefix << ".active=" << mean.active << '\n';
  };
  emit_geometry_mean("neutral", geometry.neutral_mean);
  emit_geometry_mean("full_active", geometry.full_active_mean);
  emit_geometry_mean("qualified", geometry.qualified_mean);
  std::cout << prefix << ".geometry.effective_ratio="
            << (std::isfinite(geometry.effective_ratio)
                    ? geometry.effective_ratio
                    : 0.0)
            << '\n';
  std::cout << prefix << ".geometry.participation_ratio="
            << (std::isfinite(geometry.participation_ratio)
                    ? geometry.participation_ratio
                    : 0.0)
            << '\n';
  std::cout << prefix << ".geometry.top_ratio="
            << (std::isfinite(geometry.top_ratio) ? geometry.top_ratio : 0.0)
            << '\n';
  std::cout << prefix << ".geometry.denominators_positive="
            << geometry.all_better_reference_denominators_positive << '\n';
  std::cout << prefix
            << ".geometry.ratios_finite=" << geometry.required_ratios_finite
            << '\n';
  std::cout << prefix << ".geometry.effective_ratio_pass="
            << geometry.effective_ratio_pass << '\n';
  std::cout << prefix << ".geometry.participation_ratio_pass="
            << geometry.participation_ratio_pass << '\n';
  std::cout << prefix << ".geometry.top_ratio_pass=" << geometry.top_ratio_pass
            << '\n';
  emit_outer_gap("effective", geometry.effective_gap);
  emit_outer_gap("participation", geometry.participation_gap);
  emit_outer_gap("top", geometry.top_gap);
  emit_outer_gap("active", geometry.active_gap);
  for (std::size_t seed = 0; seed < outer_gate::kSeedCount; ++seed) {
    std::cout << prefix << ".geometry.candidate_active.seed_"
              << kAttributionSeeds[seed]
              << "_pass=" << geometry.candidate_active_seed_pass[seed] << '\n';
  }
  std::cout << prefix << ".geometry.candidate_min_active="
            << geometry.candidate_min_active << '\n';
  std::cout << prefix << ".geometry.all_candidate_active_pass="
            << geometry.all_candidate_active_pass << '\n';
  std::cout << prefix << ".geometry.pass=" << geometry.pass << '\n';
  std::cout << prefix << ".replacement_pass=" << gate.replacement_pass << '\n';
  std::cout << prefix << ".neutral_improvement.point="
            << gate.neutral_improvement_point_pass << '\n';
  std::cout << prefix << ".neutral_improvement.lower_bound="
            << gate.neutral_improvement_lower_bound_pass << '\n';
  std::cout << prefix << ".neutral_improvement.positive_seed_count="
            << gate.neutral_improvement_positive_seed_count_pass << '\n';
  std::cout << prefix << ".neutral_improvement_contrast_pass="
            << gate.neutral_improvement_contrast_pass << '\n';
  std::cout << prefix << ".representation_improvement_pass="
            << gate.representation_improvement_pass << '\n';
}

void emit_jmcd_contrast(const std::string &name,
                        const RepairArmContrast &contrast) {
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    std::cout << "jmcd.summary.seed_" << kAttributionSeeds[seed] << ".contrast."
              << name << ".step32_probe_area=" << contrast.per_seed[seed]
              << '\n';
  }
  const std::string prefix =
      "jmcd.summary.contrast." + name + ".step32_probe_area";
  std::cout << prefix << ".fixed_seed_mean=" << contrast.summary.point << '\n';
  std::cout << prefix << ".bootstrap_95_low=" << contrast.summary.low << '\n';
  std::cout << prefix << ".bootstrap_95_high=" << contrast.summary.high << '\n';
  std::cout << prefix
            << ".positive_seed_count=" << contrast.summary.positive_seed_count
            << '\n';
}

void emit_jmcd_factorial_contrast(const JmcdFactorialContrast &contrast) {
  constexpr const char *name = "harmful_interaction_j_plus_m_minus_n_minus_jm";
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    std::cout << "jmcd.summary.seed_" << kAttributionSeeds[seed] << ".contrast."
              << name << ".step32_probe_area=" << contrast.per_seed[seed]
              << '\n';
  }
  const std::string prefix =
      std::string("jmcd.summary.contrast.") + name + ".step32_probe_area";
  std::cout << prefix << ".fixed_seed_mean=" << contrast.summary.point << '\n';
  std::cout << prefix << ".bootstrap_95_low=" << contrast.summary.low << '\n';
  std::cout << prefix << ".bootstrap_95_high=" << contrast.summary.high << '\n';
  std::cout << prefix
            << ".positive_seed_count=" << contrast.summary.positive_seed_count
            << '\n';
}

void emit_jmcd_material(const std::string &prefix,
                        const jmcd_gate::MaterialContrastResult &result) {
  std::cout << prefix << ".numeric_valid=" << result.numeric_valid << '\n';
  std::cout << prefix << ".point_pass=" << result.point_pass << '\n';
  std::cout << prefix << ".lower_bound_pass=" << result.lower_bound_pass
            << '\n';
  std::cout << prefix
            << ".positive_seed_count_pass=" << result.positive_seed_count_pass
            << '\n';
  std::cout << prefix << ".pass=" << result.pass << '\n';
}

void emit_jmcd_gap(const std::string &prefix, const jmcd_gate::GapResult &gap) {
  std::cout << prefix << ".applicable=" << gap.applicable << '\n';
  std::cout << prefix << ".closure_defined=" << gap.closure_defined << '\n';
  std::cout << prefix
            << ".closure=" << (std::isfinite(gap.closure) ? gap.closure : 0.0)
            << '\n';
  std::cout << prefix << ".closure_pass=" << gap.closure_pass << '\n';
  std::cout << prefix << ".direction_count=" << gap.direction_count << '\n';
  std::cout << prefix << ".direction_pass=" << gap.direction_pass << '\n';
  std::cout << prefix << ".pass=" << gap.pass << '\n';
}

void emit_jmcd_singleton(const std::string &name,
                         const jmcd_gate::SingletonResult &result,
                         const jmcd_gate::FamilyDeltas &minus_null_family,
                         const jmcd_gate::FamilyDeltas &minus_combined_family) {
  const std::string prefix = "jmcd.summary.gate." + name;
  emit_jmcd_material(prefix + ".standalone_improvement",
                     result.standalone_improvement);
  emit_jmcd_material(prefix + ".removal_rescue", result.removal_rescue);
  std::cout << prefix << ".standalone_nonharm=" << result.standalone_nonharm
            << '\n';
  for (std::size_t family = 0; family < jmcd_gate::kFamilyCount; ++family) {
    std::cout << prefix << ".family_delta.minus_null." << kFamilyNames[family]
              << '=' << minus_null_family[family] << '\n';
    std::cout << prefix << ".family_clause.minus_null." << kFamilyNames[family]
              << '=' << result.safety.minus_null_family_pass[family] << '\n';
    std::cout << prefix << ".family_delta.minus_combined."
              << kFamilyNames[family] << '=' << minus_combined_family[family]
              << '\n';
    std::cout << prefix << ".family_clause.minus_combined."
              << kFamilyNames[family] << '='
              << result.safety.minus_combined_family_pass[family] << '\n';
  }
  std::cout << prefix << ".all_family_deltas_pass="
            << result.safety.all_family_deltas_pass << '\n';
  const auto &geometry = result.safety.geometry;
  std::cout << prefix << ".geometry.effective_ratio="
            << (std::isfinite(geometry.effective_ratio)
                    ? geometry.effective_ratio
                    : 0.0)
            << '\n';
  std::cout << prefix << ".geometry.participation_ratio="
            << (std::isfinite(geometry.participation_ratio)
                    ? geometry.participation_ratio
                    : 0.0)
            << '\n';
  std::cout << prefix << ".geometry.top_ratio="
            << (std::isfinite(geometry.top_ratio) ? geometry.top_ratio : 0.0)
            << '\n';
  std::cout << prefix << ".geometry.effective_ratio_pass="
            << geometry.effective_ratio_pass << '\n';
  std::cout << prefix << ".geometry.participation_ratio_pass="
            << geometry.participation_ratio_pass << '\n';
  std::cout << prefix << ".geometry.top_ratio_pass=" << geometry.top_ratio_pass
            << '\n';
  emit_jmcd_gap(prefix + ".geometry.effective_gap", geometry.effective_gap);
  emit_jmcd_gap(prefix + ".geometry.participation_gap",
                geometry.participation_gap);
  emit_jmcd_gap(prefix + ".geometry.top_gap", geometry.top_gap);
  emit_jmcd_gap(prefix + ".geometry.active_gap", geometry.active_gap);
  for (std::size_t seed = 0; seed < jmcd_gate::kSeedCount; ++seed) {
    std::cout << prefix << ".geometry.active.seed_" << kAttributionSeeds[seed]
              << "_pass=" << geometry.active_seed_pass[seed] << '\n';
  }
  std::cout << prefix << ".geometry.minimum_active=" << geometry.minimum_active
            << '\n';
  std::cout << prefix << ".geometry.pass=" << geometry.pass << '\n';
  std::cout << prefix << ".safety_pass=" << result.safety.pass << '\n';
  std::cout << prefix << ".less_harmful=" << result.less_harmful << '\n';
  std::cout << prefix
            << ".replacement_supported=" << result.replacement_supported
            << '\n';
}

void emit_jmcd_gate(const jmcd_gate::GateResult &gate,
                    const jmcd_gate::FamilyDeltas &jepa_minus_null_family,
                    const jmcd_gate::FamilyDeltas &jepa_minus_combined_family,
                    const jmcd_gate::FamilyDeltas &mae_minus_null_family,
                    const jmcd_gate::FamilyDeltas &mae_minus_combined_family) {
  constexpr const char *prefix = "jmcd.summary.gate";
  std::cout << prefix << ".raw_classification="
            << jmcd_gate::classification_name(gate.classification) << '\n';
  std::cout << prefix << ".numeric_inputs_valid=" << gate.numeric_inputs_valid
            << '\n';
  std::cout << prefix << ".mechanics_pass=" << gate.mechanics_pass << '\n';
  std::cout << prefix << ".null_identity_pass=" << gate.null_identity_pass
            << '\n';
  std::cout << prefix << ".accepted_reference_reproduced_provisional="
            << gate.accepted_reference_reproduced << '\n';
  std::cout << prefix << ".combined_declined_from_null="
            << gate.combined_declined_from_null << '\n';
  emit_jmcd_singleton("jepa", gate.jepa, jepa_minus_null_family,
                      jepa_minus_combined_family);
  emit_jmcd_singleton("mae", gate.mae, mae_minus_null_family,
                      mae_minus_combined_family);
  emit_jmcd_material(std::string(prefix) + ".harmful_interaction_residual",
                     gate.harmful_interaction_residual);
  std::cout << prefix << ".jepa_conditional_harm=" << gate.jepa_conditional_harm
            << '\n';
  std::cout << prefix << ".mae_conditional_harm=" << gate.mae_conditional_harm
            << '\n';
  std::cout << prefix << ".harmful_interaction=" << gate.harmful_interaction
            << '\n';
}

void emit_jmcd_seed_pairing(int64_t seed, const JmcdSeedPairing &pairing) {
  const std::string prefix = "jmcd.seed_" + std::to_string(seed) + ".pairing";
  std::cout << prefix << ".complete=" << pairing.complete << '\n';
  std::cout << prefix << ".rows_exact=" << pairing.rows_exact << '\n';
  std::cout << prefix << ".row_hashes_exact=" << pairing.row_hashes_exact
            << '\n';
  std::cout << prefix << ".clean_hashes_exact=" << pairing.clean_hashes_exact
            << '\n';
  std::cout << prefix << ".target_masks_exact=" << pairing.target_masks_exact
            << '\n';
  std::cout << prefix << ".context_masks_exact=" << pairing.context_masks_exact
            << '\n';
  std::cout << prefix << ".weak_views_exact=" << pairing.weak_views_exact
            << '\n';
  std::cout << prefix
            << ".forward_pre_states_exact=" << pairing.forward_pre_states_exact
            << '\n';
  std::cout << prefix << ".forward_post_states_exact="
            << pairing.forward_post_states_exact << '\n';
  std::cout << prefix << ".common_config_exact=" << pairing.common_config_exact
            << '\n';
  std::cout << prefix << ".objective_configs_distinct="
            << pairing.objective_configs_distinct << '\n';
  std::cout << prefix << ".pass=" << pairing.pass << '\n';
}

[[nodiscard]] const char *
variance_gate_status_name(variance_gate::GateStatus status) {
  switch (status) {
  case variance_gate::GateStatus::passed:
    return "passed";
  case variance_gate::GateStatus::failed:
    return "failed";
  case variance_gate::GateStatus::reference_not_reproduced:
    return "reference_not_reproduced";
  }
  throw std::runtime_error("unknown variance necessity gate status");
}

void emit_variance_necessity_gate(
    const variance_gate::VarianceNecessityGateResult &result,
    const variance_gate::FamilyDeltas &family_deltas) {
  constexpr const char *kPrefix = "variance_necessity_gate";
  std::cout << kPrefix << ".status=" << variance_gate_status_name(result.status)
            << '\n';
  std::cout << kPrefix
            << ".clause.numeric_inputs_valid=" << result.numeric_inputs_valid
            << '\n';
  std::cout << kPrefix << ".clause.mechanics=" << result.mechanics_pass << '\n';
  std::cout << kPrefix << ".clause.harmful_aulc_reference_direction="
            << result.harmful_aulc_reference_direction << '\n';
  std::cout << kPrefix << ".clause.geometry_reference_gaps_valid="
            << result.geometry_reference_gaps_valid << '\n';
  std::cout << kPrefix
            << ".reference_reproduced=" << result.reference_reproduced << '\n';
  std::cout << kPrefix
            << ".reference_not_reproduced=" << result.reference_not_reproduced
            << '\n';
  std::cout << kPrefix << ".clause.rescue_point=" << result.rescue_point_pass
            << '\n';
  std::cout << kPrefix
            << ".clause.rescue_lower_bound=" << result.rescue_lower_bound_pass
            << '\n';
  std::cout << kPrefix << ".clause.rescue_positive_seed_count="
            << result.rescue_positive_seed_count_pass << '\n';
  std::cout << kPrefix
            << ".clause.primary_rescue=" << result.primary_rescue_pass << '\n';
  std::cout << kPrefix
            << ".clause.jm_noninferiority=" << result.jm_noninferiority_pass
            << '\n';
  for (std::size_t family = 0; family < family_deltas.size(); ++family) {
    std::cout << kPrefix << ".family_delta." << kFamilyNames[family] << '='
              << family_deltas[family] << '\n';
    std::cout << kPrefix << ".clause.family_" << kFamilyNames[family]
              << "_delta_floor=" << result.family_delta_pass[family] << '\n';
  }
  std::cout << kPrefix
            << ".clause.all_family_deltas=" << result.all_family_deltas_pass
            << '\n';
  std::cout << kPrefix
            << ".geometry.ratios_defined=" << result.geometry.ratios_defined
            << '\n';
  std::cout << kPrefix << ".geometry.effective_ratio="
            << (std::isfinite(result.geometry.effective_ratio)
                    ? result.geometry.effective_ratio
                    : 0.0)
            << '\n';
  std::cout << kPrefix << ".geometry.participation_ratio="
            << (std::isfinite(result.geometry.participation_ratio)
                    ? result.geometry.participation_ratio
                    : 0.0)
            << '\n';
  std::cout << kPrefix << ".geometry.top_ratio="
            << (std::isfinite(result.geometry.top_ratio)
                    ? result.geometry.top_ratio
                    : 0.0)
            << '\n';
  std::cout << kPrefix << ".clause.geometry_effective_ratio="
            << result.geometry.effective_ratio_pass << '\n';
  std::cout << kPrefix << ".clause.geometry_participation_ratio="
            << result.geometry.participation_ratio_pass << '\n';
  std::cout << kPrefix
            << ".clause.geometry_top_ratio=" << result.geometry.top_ratio_pass
            << '\n';
  std::cout << kPrefix << ".geometry.effective_direction_count="
            << result.geometry.effective_direction_count << '\n';
  std::cout << kPrefix << ".geometry.participation_direction_count="
            << result.geometry.participation_direction_count << '\n';
  std::cout << kPrefix << ".geometry.top_direction_count="
            << result.geometry.top_direction_count << '\n';
  std::cout << kPrefix << ".clause.geometry_effective_direction="
            << result.geometry.effective_direction_pass << '\n';
  std::cout << kPrefix << ".clause.geometry_participation_direction="
            << result.geometry.participation_direction_pass << '\n';
  std::cout << kPrefix << ".clause.geometry_top_direction="
            << result.geometry.top_direction_pass << '\n';
  for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
    std::cout << kPrefix << ".clause.active_dimension.seed_"
              << kAttributionSeeds[seed] << '='
              << result.geometry.candidate_active_seed_pass[seed] << '\n';
  }
  std::cout << kPrefix << ".geometry.candidate_min_active="
            << result.geometry.candidate_min_active << '\n';
  std::cout << kPrefix << ".clause.active_dimension_all_seeds="
            << result.geometry.all_candidate_active_pass << '\n';
  std::cout << kPrefix << ".clause.geometry_all=" << result.geometry.pass
            << '\n';
  std::cout << kPrefix
            << ".partial_amelioration=" << result.partial_amelioration << '\n';
  std::cout << kPrefix << ".pass=" << result.pass << '\n';
}

[[nodiscard]] const char *
repair_gate_status_name(repair_gate::RepairGateStatus status) {
  switch (status) {
  case repair_gate::RepairGateStatus::passed:
    return "passed";
  case repair_gate::RepairGateStatus::failed:
    return "failed";
  case repair_gate::RepairGateStatus::reference_not_reproduced:
    return "reference_not_reproduced";
  }
  throw std::runtime_error("unknown repair gate status");
}

void emit_common_repair_gate(
    const std::string &prefix,
    const repair_gate::CommonRepairGateResult &result) {
  std::cout << prefix << ".status=" << repair_gate_status_name(result.status)
            << '\n';
  std::cout << prefix
            << ".clause.numeric_inputs_valid=" << result.numeric_inputs_valid
            << '\n';
  std::cout << prefix << ".clause.harmful_reference_direction="
            << result.harmful_reference_direction << '\n';
  std::cout << prefix
            << ".reference_not_reproduced=" << result.reference_not_reproduced
            << '\n';
  std::cout << prefix << ".clause.repair_point=" << result.repair_point_pass
            << '\n';
  std::cout << prefix
            << ".clause.repair_lower_bound=" << result.repair_lower_bound_pass
            << '\n';
  std::cout << prefix << ".clause.repair_positive_seed_count="
            << result.repair_positive_seed_count_pass << '\n';
  std::cout << prefix
            << ".clause.jm_noninferiority=" << result.jm_noninferiority_pass
            << '\n';
  for (std::size_t family = 0; family < result.family_delta_pass.size();
       ++family) {
    std::cout << prefix << ".clause.family_" << kFamilyNames[family]
              << "_delta_floor=" << result.family_delta_pass[family] << '\n';
  }
  std::cout << prefix
            << ".clause.all_family_deltas=" << result.all_family_deltas_pass
            << '\n';
  std::cout << prefix << ".clause.mechanics=" << result.mechanics_pass << '\n';
  const auto &geometry = result.geometry;
  std::cout << prefix
            << ".geometry.denominators_valid=" << geometry.denominators_valid
            << '\n';
  std::cout << prefix << ".geometry.ratios_defined=" << geometry.ratios_defined
            << '\n';
  std::cout << prefix << ".geometry.effective_ratio="
            << (std::isfinite(geometry.effective_ratio)
                    ? geometry.effective_ratio
                    : 0.0)
            << '\n';
  std::cout << prefix << ".geometry.participation_ratio="
            << (std::isfinite(geometry.participation_ratio)
                    ? geometry.participation_ratio
                    : 0.0)
            << '\n';
  std::cout << prefix << ".geometry.top_ratio="
            << (std::isfinite(geometry.top_ratio) ? geometry.top_ratio : 0.0)
            << '\n';
  std::cout << prefix << ".clause.geometry_effective_ratio="
            << geometry.effective_ratio_pass << '\n';
  std::cout << prefix << ".clause.geometry_participation_ratio="
            << geometry.participation_ratio_pass << '\n';
  std::cout << prefix
            << ".clause.geometry_top_ratio=" << geometry.top_ratio_pass << '\n';
  std::cout << prefix << ".geometry.effective_direction_count="
            << geometry.effective_direction_count << '\n';
  std::cout << prefix << ".geometry.participation_direction_count="
            << geometry.participation_direction_count << '\n';
  std::cout << prefix
            << ".geometry.top_direction_count=" << geometry.top_direction_count
            << '\n';
  std::cout << prefix << ".clause.geometry_effective_direction="
            << geometry.effective_direction_pass << '\n';
  std::cout << prefix << ".clause.geometry_participation_direction="
            << geometry.participation_direction_pass << '\n';
  std::cout << prefix
            << ".clause.geometry_top_direction=" << geometry.top_direction_pass
            << '\n';
  std::cout << prefix << ".geometry.candidate_active_fixed_seed_mean="
            << geometry.candidate_active_mean << '\n';
  std::cout << prefix
            << ".clause.active_dimension_fraction=" << geometry.active_pass
            << '\n';
  std::cout << prefix << ".clause.geometry_all=" << geometry.pass << '\n';
}

void emit_tf_repair_gate(const repair_gate::TfRepairGateResult &result,
                         const repair_gate::TfRatios &ratios) {
  constexpr const char *kPrefix = "repair_gate.tf";
  emit_common_repair_gate(kPrefix, result.common);
  std::cout << kPrefix << ".tf_ratio.fixed_seed_mean=" << result.tf_ratio_mean
            << '\n';
  std::cout << kPrefix << ".clause.tf_ratio_mean=" << result.tf_ratio_mean_pass
            << '\n';
  for (std::size_t seed = 0; seed < ratios.size(); ++seed) {
    std::cout << kPrefix << ".tf_ratio.seed_" << kAttributionSeeds[seed] << "="
              << ratios[seed] << '\n';
    std::cout << kPrefix << ".clause.tf_ratio.seed_" << kAttributionSeeds[seed]
              << '=' << result.tf_ratio_seed_pass[seed] << '\n';
  }
  std::cout << kPrefix
            << ".clause.tf_ratio_all_seeds=" << result.all_tf_ratio_seeds_pass
            << '\n';
  std::cout << kPrefix << ".pass=" << result.pass << '\n';
}

void emit_vicreg_repair_gate(
    const repair_gate::VicregRepairGateResult &result) {
  constexpr const char *kPrefix = "repair_gate.vicreg";
  emit_common_repair_gate(kPrefix, result.common);
  std::cout << kPrefix << ".pass=" << result.pass << '\n';
}

[[nodiscard]] double vector_mean(const std::vector<double> &values) {
  if (values.empty()) {
    throw std::runtime_error("attribution statistic vector is empty");
  }
  return std::accumulate(values.begin(), values.end(), 0.0) /
         static_cast<double>(values.size());
}

int run_objective_mask_attribution(const Options &options) {
  const bool variance_necessity =
      options.experiment == "vicreg-variance-component-necessity";
  const bool outer_augmentation =
      options.experiment == "outer-augmentation-representation-training";
  const bool core_decomposition =
      options.experiment == "jepa-mae-core-decomposition";
  const bool legacy_attribution =
      !variance_necessity && !outer_augmentation && !core_decomposition;
  SemanticQualifierEvidence semantic_qualifier{};
  if ((options.steps > 0 && options.steps != kAttributionSteps) ||
      (options.seeds > 0 &&
       options.seeds != static_cast<int64_t>(kAttributionSeeds.size())) ||
      !options.weak_views) {
    throw std::runtime_error(
        "the module-only objective screen is preregistered for 32 steps, 3 "
        "seeds, and active weak views");
  }
  std::vector<AttributionArm> arms;
  if (outer_augmentation) {
    arms.assign(kOuterAugmentationArms.begin(), kOuterAugmentationArms.end());
  } else if (core_decomposition) {
    arms.assign(kJmcdArms.begin(), kJmcdArms.end());
  } else if (variance_necessity) {
    arms.assign(kVarianceNecessityArms.begin(), kVarianceNecessityArms.end());
  } else {
    arms.assign(kAttributionArms.begin(), kAttributionArms.end());
  }
  torch::Device device(torch::kCPU);
  if (options.device == "cuda") {
    if (!torch::cuda::is_available()) {
      throw std::runtime_error("CUDA requested but unavailable");
    }
    device = torch::Device(torch::kCUDA, 0);
  } else if (options.device != "cpu") {
    throw std::runtime_error("--device must be cpu or cuda");
  }
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);
  validate_attribution_arm_configs(device, arms);
  if (core_decomposition) {
    if (!device.is_cuda() || device.index() != 0) {
      throw std::runtime_error("JMCD-1 requires cuda:0");
    }
    validate_jmcd_arm_table(device);
  }
  if (outer_augmentation) {
    if (!device.is_cuda() || device.index() != 0) {
      throw std::runtime_error(
          "outer augmentation representation training requires cuda:0");
    }
    validate_outer_augmentation_configs(device);
    validate_outer_augmentation_seed_domain();
    if (options.qualifier_log.empty()) {
      throw std::runtime_error(
          "outer augmentation training requires --qualifier-log");
    }
    semantic_qualifier =
        parse_semantic_qualifier_evidence(options.qualifier_log);
    if (!semantic_qualifier.pass) {
      throw std::runtime_error(
          "outer augmentation semantic qualifier evidence failed");
    }
  } else if (legacy_attribution) {
    validate_tf_schedule_contract();
  }

  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout
      << "schema="
      << (outer_augmentation
              ? "wikimyei.mtf_jepa_mae_vicreg.outer_augmentation_training.v1"
              : (core_decomposition
                     ? "wikimyei.mtf_jepa_mae_vicreg.jmcd.v1"
                     : (variance_necessity ? "wikimyei.mtf_jepa_mae_vicreg."
                                             "vicreg_variance_necessity.v1"
                                           : "wikimyei.mtf_jepa_mae_vicreg."
                                             "objective_mask_attribution.v5")))
      << '\n';
  std::cout << "module_only=true\n";
  std::cout << "device=" << options.device << '\n';
  std::cout << "architecture_scope=exact_active_H30_F9_D32\n";
  if (outer_augmentation) {
    std::cout << "outer_augmentation.summary.launcher_augmentation=true\n";
    std::cout << "outer_augmentation.summary.launcher_augmentation_device="
                 "cpu\n";
    std::cout << "outer_augmentation.summary.qualifier.schema_count="
              << semantic_qualifier.schema_count << '\n';
    std::cout
        << "outer_augmentation.summary.qualifier.candidate_qualified_count="
        << semantic_qualifier.candidate_qualified_count << '\n';
    std::cout
        << "outer_augmentation.summary.qualifier.full_active_not_qualified_"
           "count="
        << semantic_qualifier.full_active_not_qualified_count << '\n';
    std::cout << "outer_augmentation.summary.qualifier.global_not_qualified_"
                 "count="
              << semantic_qualifier.global_not_qualified_count << '\n';
    std::cout << "outer_augmentation.summary.qualifier.schema_value_exact="
              << semantic_qualifier.schema_value_exact << '\n';
    std::cout << "outer_augmentation.summary.qualifier.candidate_value_exact="
              << semantic_qualifier.candidate_value_exact << '\n';
    std::cout << "outer_augmentation.summary.qualifier.full_active_value_exact="
              << semantic_qualifier.full_active_value_exact << '\n';
    std::cout << "outer_augmentation.summary.qualifier.global_value_exact="
              << semantic_qualifier.global_value_exact << '\n';
    std::cout << "outer_augmentation.summary.qualifier.pass="
              << semantic_qualifier.pass << '\n';
  }
  std::cout << "internal_weak_view_augmentation=active\n";
  std::cout << "internal_weak_view_gaussian_jitter_std=0.005\n";
  std::cout << "internal_weak_view_resolved_time_dropout_prob=0.01\n";
  std::cout << "all_objective_branches_execute=true\n";
  std::cout << "training_steps=" << kAttributionSteps << '\n';
  std::cout << "checkpoints=0,16,32\n";
  std::cout << "model_seeds=17,31,47\n";
  std::cout << "paired_initialization=exact_named_parameter_values\n";
  std::cout << "paired_batches=true\n";
  std::cout << "paired_cpu_cuda_rng_per_step=true\n";
  std::cout << "gradient_diagnostic_batch=fixed_within_seed\n";
  std::cout << "gradient_diagnostic_rng=fixed_within_seed\n";
  std::cout << "gradient_served_trunk=tokenizer.*,encoder.*\n";
  std::cout << "gradient_vicreg_head=vicreg_stability_head.*\n";
  std::cout << "gradient_canonical_weights=jepa:1,mae:0.25,tf_align:0.10,"
               "vicreg:0.05\n";
  if (legacy_attribution) {
    std::cout << "tf_gradient_matched_warmup.zero_based_update_formula="
                 "0.0165124+(0.10-0.0165124)*min(s,16)/16\n";
    std::cout << "tf_gradient_matched_warmup.initial_weight="
              << kGradientMatchedTfInitialWeight << '\n';
    std::cout << "tf_gradient_matched_warmup.final_weight="
              << kGradientMatchedTfFinalWeight << '\n';
    std::cout << "tf_gradient_matched_warmup.ramp_updates="
              << kGradientMatchedTfRampUpdates << '\n';
    std::cout << "tf_gradient_matched_warmup.step_15_weight="
              << attribution_arm_weights(kAttributionArms[6], 15)[2] << '\n';
    std::cout << "tf_gradient_matched_warmup.step_16_weight="
              << attribution_arm_weights(kAttributionArms[6], 16)[2] << '\n';
    std::cout << "tf_gradient_matched_warmup.step_32_weight="
              << attribution_arm_weights(kAttributionArms[6], 32)[2] << '\n';
    std::cout << "tf_gradient_matched_warmup.schedule_assertions="
                 "exact_0_15_16_32_and_monotone\n";
  }
  std::cout << "projected_channel_stratified_vicreg.global_enabled=false\n";
  std::cout << "projected_channel_stratified_vicreg.channel_enabled=true\n";
  std::cout << "projected_channel_stratified_vicreg.stratify_by_channel=true\n";
  std::cout << "projected_channel_stratified_vicreg.lambda_channel_vicreg="
               "0.25\n";
  std::cout << "vicreg_clean_global_geometry="
               "test_rows_preprojector_D32_centered_across_samples\n";
  std::cout << "vicreg_clean_projected_channel_geometry="
               "test_rows_D64_centered_across_samples_per_channel\n";
  std::cout << "vicreg_variance_floor_fraction_formula="
               "mean(sqrt(var(unbiased=false)+1e-4)<1)\n";
  std::cout << "vicreg_variance_floor_min_rows_per_channel=2\n";
  std::cout << "vicreg_aggregate_loss_semantics=inner_weighted\n";
  std::cout << "vicreg_component_effective_weight_formula="
               "0.05*actual_inner_multiplier*component_weight\n";
  std::cout << "vicreg_component_gradient_gate="
               "single_graph_explicit_weighted_reconstruction\n";
  std::cout << "vicreg_separate_backward_component_sum=descriptive\n";
  std::cout << "vicreg_component_weights.reference=sim:25,var:25,cov:1\n";
  if (variance_necessity) {
    std::cout << "vicreg_component_weights.no_variance=sim:25,var:0,cov:1\n";
    std::cout << "variance_ablation_only_scalar=vicreg_var_weight:25_to_0\n";
    std::cout << "variance_raw_diagnostic_remains_active=true\n";
    std::cout << "variance_necessity.jepa_mae_only.inactive_use_global_vicreg="
                 "true\n";
    std::cout << "variance_necessity.jepa_mae_only.inactive_use_channel_vicreg="
                 "false\n";
    std::cout << "variance_necessity.jepa_mae_only.inactive_lambda_channel_"
                 "vicreg=1\n";
    std::cout << "variance_necessity.jepa_mae_only.frozen_table_lambda_channel_"
                 "vicreg=0.25\n";
    std::cout << "variance_necessity.jepa_mae_only.inactive_manifest_"
                 "difference_behaviorally_inert=true\n";
    std::cout << "variance_gradient_difference_gate="
                 "common_branch_exact_plus_outer_vicreg_difference\n";
    std::cout << "direct_float32_total_gradient_subtraction=descriptive\n";
  }
  std::cout << "repair_primary_bootstrap.common_resampling_table=true\n";
  std::cout << "repair_primary_bootstrap.seed=" << kRepairPrimaryBootstrapSeed
            << '\n';
  std::cout << "batch_pairing=row_indices_actual_forward_masks_and_actual_weak_"
               "view_bytes\n";
  std::cout << "weak_view_hash_contract=dtype_shape_contiguous_value_bytes\n";
  std::cout << "gradient_diagnostic_generator_state=cpu_cuda_restored\n";
  std::cout << "gradient_diagnostic_parameter_ema_state=exact_before_after\n";
  std::cout << "gradient_diagnostic_optimizer_state=exact_before_after_when_"
               "optimizer_exists\n";
  std::cout << "gradient_diagnostic_weak_views=exact_replay_all_forwards\n";
  std::cout << "reported_finite_contract=runtime_asserted\n";
  std::cout << "gradient_clipping_contract=abort_before_optimizer\n";
  std::cout << "update_order=one_adam_then_one_target_ema\n";
  std::cout << "gradient_cancellation_ratio_definition="
               "canonical_sum_norm_over_sum_branch_norms\n";
  std::cout << "probe_data=representation_quality_clean_disjoint_splits\n";
  std::cout << "probe_selection=validation_selected_ridge\n";
  std::cout << "probe_sample_ladder=32,64,128,256\n";
  std::cout << "paired_group_bootstrap_replicates="
            << kAttributionBootstrapReplicates << '\n';
  std::cout << "paired_group_bootstrap_seed_aggregation=fixed_seed_mean\n";
  if (legacy_attribution) {
    std::cout << "full_overlap_allowed.context_overlap_allowed=true\n";
    std::cout << "full_overlap_allowed.target_selection_policy="
                 "unchanged_from_full_soft\n";
    std::cout << "full_overlap_allowed.interpretation="
                 "context_overlap_allowed_not_mask_off\n";
    std::cout << "true_mask_off_tested=false\n";
    std::cout << "mask_off_claim=false\n";
    std::cout << "conditional_split_trigger=jepa_mae_only_rescued_full_v2\n";
  } else if (core_decomposition) {
    std::cout << "screen_scope=clean_core_objective_2x2_factorial\n";
    std::cout << "jmcd.outer_augmentation=false\n";
    std::cout << "jmcd.tf_optimizer_coefficient=0\n";
    std::cout << "jmcd.vicreg_optimizer_coefficient=0\n";
    std::cout << "jmcd.null_arm_is_trained_factorial_cell=true\n";
    std::cout << "jmcd.reference_audit_required=true\n";
    std::cout << "long_run_authorized=false\n";
    std::cout << "production_or_end_to_end_authorized=false\n";
  } else {
    std::cout << "screen_scope="
              << (outer_augmentation
                      ? "jm_neutral_full_active_outer_qualified_outer"
                      : "jm_stratified_full_stratified_no_variance")
              << '\n';
    std::cout << "long_run_authorized=false\n";
    std::cout << "production_or_end_to_end_authorized=false\n";
  }
  std::cout << "quality_failure_exit_policy=exit_zero\n";

  auto ssl = generate_dataset(0, 256);
  auto probe_train = generate_dataset(1000000, 256);
  auto probe_validation = generate_dataset(2000000, 128);
  auto test = generate_dataset(3000000, 256);
  const auto raw_projection = make_raw_equal_width_projection();
  const auto raw_control_train =
      raw_equal_width_features(probe_train, raw_projection);
  const auto raw_control_validation =
      raw_equal_width_features(probe_validation, raw_projection);
  const auto raw_control_test = raw_equal_width_features(test, raw_projection);
  const auto normalization = fit_normalization(ssl);
  for (Dataset *dataset : {&ssl, &probe_train, &probe_validation, &test}) {
    normalize(*dataset, normalization);
    validate_dataset(*dataset);
  }
  if (!probe_train.target.var(0, false).gt(1.0e-8).all().item<bool>()) {
    throw std::runtime_error("attribution probe target lacks variance");
  }
  const auto raw_control_probe =
      probe_curve(raw_control_train, raw_control_validation, raw_control_test,
                  probe_train.target, probe_validation.target, test.target,
                  {32, 64, 128, 256});
  validate_probe_curve_finite(raw_control_probe,
                              "attribution raw equal-width control");
  std::cout << "control.raw_equal_width.area=" << raw_control_probe.area
            << '\n';
  std::cout << "control.raw_equal_width.final_macro_r2="
            << raw_control_probe.points.back().score.macro << '\n';
  for (int64_t family = 0; family < kFamilies; ++family) {
    std::cout << "control.raw_equal_width.family_"
              << kFamilyNames[static_cast<std::size_t>(family)] << "_r2="
              << raw_control_probe.points.back()
                     .score.family[static_cast<std::size_t>(family)]
              << '\n';
  }

  std::array<VarianceAblationMechanicalResult, kAttributionSeeds.size()>
      variance_ablation_mechanics{};
  if (variance_necessity) {
    for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
      variance_ablation_mechanics[seed] = validate_variance_ablation_mechanics(
          ssl, device, kAttributionSeeds[seed]);
      emit_variance_ablation_mechanics(kAttributionSeeds[seed],
                                       variance_ablation_mechanics[seed]);
      if (!variance_ablation_mechanics[seed].pass) {
        throw std::runtime_error(
            "variance ablation mechanical contract failed before training");
      }
    }
  }

  std::vector<AttributionArmResult> results;
  results.reserve(kAttributionSeeds.size() * arms.size());
  for (const int64_t model_seed : kAttributionSeeds) {
    const std::size_t seed_begin = results.size();
    ParameterSnapshot initial_reference{};
    torch::Tensor initial_embedding_reference{};
    ProbeCurve initial_probe_reference{};
    std::array<Geometry, kChannels> initial_served_geometry_reference{};
    Geometry initial_global_preprojector_geometry_reference{};
    std::array<Geometry, kChannels>
        initial_projected_channel_geometry_reference{};
    bool step0_served_geometry_exact = true;
    bool step0_global_preprojector_geometry_exact = true;
    bool step0_projected_channel_geometry_exact = true;
    for (std::size_t arm_index = 0; arm_index < arms.size(); ++arm_index) {
      const auto &arm = arms[arm_index];
      set_paired_rng(model_seed, device);
      auto model = mtf::MtfJepaMaeVicreg(attribution_config(device, arm));
      AttributionArmResult result{};
      result.seed = model_seed;
      result.arm = arm;
      if (arm_index == 0) {
        initial_reference = snapshot_parameters(model);
      }
      result.initial_parameter_max_abs_diff =
          parameter_max_abs_diff(model, initial_reference);
      if (result.initial_parameter_max_abs_diff != 0.0) {
        throw std::runtime_error(
            "attribution initialization is not exactly paired");
      }
      const auto arm_initial_parameters = snapshot_parameters(model);

      auto checkpoint_zero = evaluate_attribution_checkpoint(
          model, probe_train, probe_validation, test, device, 0);
      checkpoint_zero.gradients = checkpoint_gradient_diagnostic(
          model, ssl, device, arm, model_seed, 0);
      validate_attribution_checkpoint_finite(checkpoint_zero,
                                             "attribution checkpoint step 0");
      if (arm_index == 0) {
        initial_embedding_reference = checkpoint_zero.test_embeddings.clone();
        initial_probe_reference = checkpoint_zero.probe;
        initial_served_geometry_reference = checkpoint_zero.geometry;
        initial_global_preprojector_geometry_reference =
            checkpoint_zero.vicreg_clean_global_preprojector_geometry;
        initial_projected_channel_geometry_reference =
            checkpoint_zero.vicreg_clean_projected_channel_geometry;
      } else {
        result.initial_embedding_max_abs_diff =
            (checkpoint_zero.test_embeddings - initial_embedding_reference)
                .abs()
                .max()
                .item<double>();
        result.initial_probe_prediction_max_abs_diff =
            probe_curve_prediction_max_abs_diff(checkpoint_zero.probe,
                                                initial_probe_reference);
        step0_served_geometry_exact =
            step0_served_geometry_exact &&
            geometry_exact(checkpoint_zero.geometry,
                           initial_served_geometry_reference);
        step0_global_preprojector_geometry_exact =
            step0_global_preprojector_geometry_exact &&
            geometry_exact(
                checkpoint_zero.vicreg_clean_global_preprojector_geometry,
                initial_global_preprojector_geometry_reference);
        step0_projected_channel_geometry_exact =
            step0_projected_channel_geometry_exact &&
            geometry_exact(
                checkpoint_zero.vicreg_clean_projected_channel_geometry,
                initial_projected_channel_geometry_reference);
        if (result.initial_embedding_max_abs_diff != 0.0 ||
            result.initial_probe_prediction_max_abs_diff != 0.0 ||
            checkpoint_zero.probe.area != initial_probe_reference.area ||
            !probe_curve_selected_alphas_equal(checkpoint_zero.probe,
                                               initial_probe_reference) ||
            !step0_served_geometry_exact ||
            !step0_global_preprojector_geometry_exact ||
            !step0_projected_channel_geometry_exact) {
          throw std::runtime_error(
              "attribution step-zero representation/probe pairing failed");
        }
      }
      result.checkpoints.push_back(std::move(checkpoint_zero));
      if (outer_augmentation) {
        const auto preprocessing =
            outer_preprocessing_config(device, arm_index);
        result.training = train_attribution_arm(
            model, ssl, probe_train, probe_validation, test, device, arm,
            model_seed, result.checkpoints, &preprocessing, arm_index);
      } else if (core_decomposition) {
        result.training = train_attribution_arm(
            model, ssl, probe_train, probe_validation, test, device, arm,
            model_seed, result.checkpoints, nullptr,
            std::numeric_limits<std::size_t>::max(),
            /*capture_jmcd_mechanics=*/true);
      } else {
        result.training = train_attribution_arm(
            model, ssl, probe_train, probe_validation, test, device, arm,
            model_seed, result.checkpoints);
      }
      if (core_decomposition) {
        result.final_all_trainable_max_abs_diff =
            parameter_partition_max_abs_diff(
                model, arm_initial_parameters,
                ParameterDeltaPartition::all_trainable);
        result.final_served_max_abs_diff = parameter_partition_max_abs_diff(
            model, arm_initial_parameters, ParameterDeltaPartition::served);
        result.final_predictor_max_abs_diff = parameter_partition_max_abs_diff(
            model, arm_initial_parameters, ParameterDeltaPartition::predictor);
        result.final_mae_decoder_max_abs_diff =
            parameter_partition_max_abs_diff(
                model, arm_initial_parameters,
                ParameterDeltaPartition::mae_decoder);
        result.final_vicreg_head_max_abs_diff =
            parameter_partition_max_abs_diff(
                model, arm_initial_parameters,
                ParameterDeltaPartition::vicreg_head);
        result.final_target_ema_max_abs_diff = parameter_partition_max_abs_diff(
            model, arm_initial_parameters, ParameterDeltaPartition::target_ema);
      }
      results.push_back(std::move(result));
    }

    if (outer_augmentation) {
      const auto outer_pairing =
          validate_outer_seed_pairing(results, seed_begin);
      emit_outer_seed_pairing(model_seed, outer_pairing, results, seed_begin);
      continue;
    }

    bool target_masks_exact = true;
    bool base_context_masks_exact = true;
    bool overlap_context_changed = false;
    bool batch_rows_exact = true;
    bool weak_views_exact = true;
    bool view_a_data_exact = true;
    bool view_a_feature_mask_exact = true;
    bool view_b_data_exact = true;
    bool view_b_feature_mask_exact = true;
    for (int64_t step = 0; step < kAttributionSteps; ++step) {
      const auto &reference_target =
          results[seed_begin].training.target_masks[step];
      const auto &reference_context =
          results[seed_begin].training.context_masks[step];
      const auto &reference_rows =
          results[seed_begin].training.batch_rows[step];
      const auto &reference_weak_view =
          results[seed_begin].training.weak_view_digests[step];
      for (std::size_t arm_index = 1; arm_index < arms.size(); ++arm_index) {
        target_masks_exact =
            target_masks_exact &&
            torch::equal(
                reference_target,
                results[seed_begin + arm_index].training.target_masks[step]);
        const auto &candidate_training =
            results[seed_begin + arm_index].training;
        batch_rows_exact =
            batch_rows_exact &&
            candidate_training.batch_rows[step] == reference_rows;
        const auto &candidate_weak_view =
            candidate_training.weak_view_digests[step];
        weak_views_exact =
            weak_views_exact &&
            weak_view_digests_equal(reference_weak_view, candidate_weak_view);
        view_a_data_exact =
            view_a_data_exact &&
            reference_weak_view.view_a_data == candidate_weak_view.view_a_data;
        view_a_feature_mask_exact = view_a_feature_mask_exact &&
                                    reference_weak_view.view_a_feature_mask ==
                                        candidate_weak_view.view_a_feature_mask;
        view_b_data_exact =
            view_b_data_exact &&
            reference_weak_view.view_b_data == candidate_weak_view.view_b_data;
        view_b_feature_mask_exact = view_b_feature_mask_exact &&
                                    reference_weak_view.view_b_feature_mask ==
                                        candidate_weak_view.view_b_feature_mask;
      }
      for (std::size_t arm_index = 1; arm_index < arms.size(); ++arm_index) {
        if (legacy_attribution && arm_index == 3) {
          continue;
        }
        base_context_masks_exact =
            base_context_masks_exact &&
            torch::equal(
                reference_context,
                results[seed_begin + arm_index].training.context_masks[step]);
      }
      if (legacy_attribution) {
        overlap_context_changed =
            overlap_context_changed ||
            !torch::equal(reference_context,
                          results[seed_begin + 3].training.context_masks[step]);
      }
    }
    if (!target_masks_exact || !base_context_masks_exact || !batch_rows_exact ||
        !weak_views_exact || !view_a_data_exact || !view_a_feature_mask_exact ||
        !view_b_data_exact || !view_b_feature_mask_exact) {
      throw std::runtime_error("attribution exact training pairing failed");
    }
    const std::string pairing_prefix =
        (core_decomposition ? "jmcd.seed_" : "seed_") +
        std::to_string(model_seed) + ".pairing";
    std::cout << pairing_prefix
              << ".initial_named_parameter_values_exact=true\n";
    std::cout << pairing_prefix << ".step0_embeddings_exact=true\n";
    std::cout << pairing_prefix << ".step0_probe_predictions_exact=true\n";
    std::cout << pairing_prefix << ".step0_probe_area_exact=true\n";
    std::cout << pairing_prefix << ".step0_probe_selected_alphas_exact=true\n";
    const bool step0_geometry_exact =
        step0_served_geometry_exact &&
        step0_global_preprojector_geometry_exact &&
        step0_projected_channel_geometry_exact;
    std::cout << pairing_prefix
              << ".step0_served_geometry_exact=" << step0_served_geometry_exact
              << '\n';
    std::cout << pairing_prefix << ".step0_global_preprojector_geometry_exact="
              << step0_global_preprojector_geometry_exact << '\n';
    std::cout << pairing_prefix << ".step0_projected_channel_geometry_exact="
              << step0_projected_channel_geometry_exact << '\n';
    std::cout << pairing_prefix
              << ".step0_geometry_exact=" << step0_geometry_exact << '\n';
    std::cout << pairing_prefix
              << ".mask_surface=actual_forward_debug_tensors\n";
    std::cout << pairing_prefix
              << ".target_masks_all_arms_exact=" << target_masks_exact << '\n';
    std::cout << pairing_prefix << ".context_masks_overlap_0_50_arms_exact="
              << base_context_masks_exact << '\n';
    if (legacy_attribution) {
      std::cout << pairing_prefix << ".full_overlap_allowed_context_changed="
                << overlap_context_changed << '\n';
    }
    std::cout << pairing_prefix
              << ".batch_row_indices_all_arms_exact=" << batch_rows_exact
              << '\n';
    std::cout << pairing_prefix
              << ".actual_weak_views_all_arms_exact=" << weak_views_exact
              << '\n';
    std::cout << pairing_prefix
              << ".actual_view_a_data_all_arms_exact=" << view_a_data_exact
              << '\n';
    std::cout << pairing_prefix << ".actual_view_a_feature_mask_all_arms_exact="
              << view_a_feature_mask_exact << '\n';
    std::cout << pairing_prefix
              << ".actual_view_b_data_all_arms_exact=" << view_b_data_exact
              << '\n';
    std::cout << pairing_prefix << ".actual_view_b_feature_mask_all_arms_exact="
              << view_b_feature_mask_exact << '\n';
  }

  for (const auto &result : results) {
    const bool jmcd_new_arm =
        core_decomposition && std::string(result.arm.name) != "jepa_mae_only";
    const std::string prefix = (jmcd_new_arm ? "jmcd.seed_" : "seed_") +
                               std::to_string(result.seed) + ".arm." +
                               result.arm.name;
    std::cout << prefix << ".lambda_jepa=" << result.arm.lambda_jepa << '\n';
    std::cout << prefix << ".lambda_mae=" << result.arm.lambda_mae << '\n';
    std::cout << prefix << ".lambda_tf_align=" << result.arm.lambda_tf_align
              << '\n';
    std::cout << prefix << ".lambda_vicreg=" << result.arm.lambda_vicreg
              << '\n';
    std::cout << prefix << ".tf_gradient_matched_warmup="
              << result.arm.tf_gradient_matched_warmup << '\n';
    std::cout << prefix << ".projected_channel_stratified_vicreg="
              << result.arm.projected_channel_stratified_vicreg << '\n';
    std::cout << prefix << ".use_global_vicreg="
              << !result.arm.projected_channel_stratified_vicreg << '\n';
    std::cout << prefix << ".use_channel_vicreg="
              << result.arm.projected_channel_stratified_vicreg << '\n';
    std::cout << prefix << ".stratify_channel_vicreg_by_channel="
              << result.arm.projected_channel_stratified_vicreg << '\n';
    std::cout << prefix << ".lambda_global_vicreg=0.25\n";
    std::cout << prefix << ".lambda_channel_vicreg="
              << (result.arm.projected_channel_stratified_vicreg ? 0.25 : 1.0)
              << '\n';
    std::cout << prefix << ".vicreg_var_weight=" << result.arm.vicreg_var_weight
              << '\n';
    std::cout << prefix << ".vicreg_variance_effective="
              << (result.arm.lambda_vicreg != 0.0 &&
                  result.arm.vicreg_var_weight != 0.0)
              << '\n';
    std::cout << prefix << ".return_vicreg_debug_tensors=true\n";
    std::cout << prefix << ".max_context_target_time_overlap="
              << result.arm.max_context_target_time_overlap << '\n';
    std::cout << prefix << ".initial_parameter_max_abs_diff="
              << result.initial_parameter_max_abs_diff << '\n';
    std::cout << prefix << ".step0_embedding_max_abs_diff="
              << result.initial_embedding_max_abs_diff << '\n';
    std::cout << prefix << ".step0_probe_prediction_max_abs_diff="
              << result.initial_probe_prediction_max_abs_diff << '\n';
    std::cout << prefix << ".training.first_8_mean_loss="
              << mean_loss_window(result.training.total_losses, true) << '\n';
    std::cout << prefix << ".training.last_8_mean_loss="
              << mean_loss_window(result.training.total_losses, false) << '\n';
    int64_t clipped_steps = 0;
    for (int64_t step = 0; step < kAttributionSteps; ++step) {
      const double clip_factor = result.training.clip_factors[step];
      clipped_steps += clip_factor < 1.0 ? 1 : 0;
      std::cout << prefix << ".training.step_" << step + 1
                << ".pre_clip_gradient_norm="
                << result.training.pre_clip_gradient_norms[step] << '\n';
      std::cout << prefix << ".training.step_" << step + 1
                << ".clip_factor=" << clip_factor << '\n';
      std::cout << prefix << ".training.zero_based_update_" << step
                << ".optimizer_lambda_tf_align="
                << result.training.optimizer_tf_coefficients[step] << '\n';
      const std::string update_prefix =
          prefix + ".training.zero_based_update_" + std::to_string(step);
      emit_mask_hash(update_prefix + ".batch_rows_hash",
                     result.training.batch_row_hashes[step]);
      const auto &views = result.training.weak_view_digests[step];
      emit_mask_hash(update_prefix + ".vicreg_view_a_data_hash",
                     views.view_a_data);
      emit_mask_hash(update_prefix + ".vicreg_view_a_feature_mask_hash",
                     views.view_a_feature_mask);
      emit_mask_hash(update_prefix + ".vicreg_view_b_data_hash",
                     views.view_b_data);
      emit_mask_hash(update_prefix + ".vicreg_view_b_feature_mask_hash",
                     views.view_b_feature_mask);
      if (step == 0 || step == 15 || step == 31) {
        std::cout << prefix << ".training.transition_" << step << "_to_"
                  << step + 1 << ".served_update_norm="
                  << result.training.served_update_norms[step] << '\n';
      }
    }
    std::cout << prefix << ".training.mean_pre_clip_gradient_norm="
              << vector_mean(result.training.pre_clip_gradient_norms) << '\n';
    std::cout << prefix << ".training.max_pre_clip_gradient_norm="
              << *std::max_element(
                     result.training.pre_clip_gradient_norms.begin(),
                     result.training.pre_clip_gradient_norms.end())
              << '\n';
    std::cout << prefix << ".training.mean_clip_factor="
              << vector_mean(result.training.clip_factors) << '\n';
    std::cout << prefix << ".training.min_clip_factor="
              << *std::min_element(result.training.clip_factors.begin(),
                                   result.training.clip_factors.end())
              << '\n';
    std::cout << prefix << ".training.clipped_step_count=" << clipped_steps
              << '\n';
    for (std::size_t branch = 0;
         branch < result.training.component_loss_sums.size(); ++branch) {
      const std::string loss_label =
          branch == 3
              ? "mean_inner_weighted_vicreg_loss"
              : "mean_raw_" + std::string(kAttributionBranchNames[branch]) +
                    "_loss";
      std::cout << prefix << ".training." << loss_label << '='
                << result.training.component_loss_sums[branch] /
                       static_cast<double>(kAttributionSteps)
                << '\n';
    }
    emit_mask_hash(prefix + ".mask.target_hash", result.training.masks.target);
    emit_mask_hash(prefix + ".mask.context_hash",
                   result.training.masks.context);
    std::cout << prefix << ".mask.mean_batch_total_target_tokens="
              << static_cast<double>(result.training.masks.target_tokens) /
                     kAttributionSteps
              << '\n';
    std::cout << prefix << ".mask.mean_batch_total_context_tokens="
              << static_cast<double>(result.training.masks.context_tokens) /
                     kAttributionSteps
              << '\n';
    const double target_tokens_per_sample =
        static_cast<double>(result.training.masks.target_tokens) /
        static_cast<double>(kAttributionSteps * kModelRowBatchSize);
    const double context_tokens_per_sample =
        static_cast<double>(result.training.masks.context_tokens) /
        static_cast<double>(kAttributionSteps * kModelRowBatchSize);
    std::cout << prefix
              << ".mask.target_tokens_per_sample=" << target_tokens_per_sample
              << '\n';
    std::cout << prefix
              << ".mask.context_tokens_per_sample=" << context_tokens_per_sample
              << '\n';
    std::cout << prefix << ".mask.valid_tokens_per_sample=72\n";
    std::cout << prefix << ".mask.target_ratio_per_sample="
              << target_tokens_per_sample / 72.0 << '\n';
    if (target_tokens_per_sample != 6.0 ||
        (std::string(result.arm.name) == "full_overlap_allowed" &&
         context_tokens_per_sample != 66.0)) {
      throw std::runtime_error(
          "attribution active token-count contract failed");
    }
    std::cout << prefix << ".mask.mean_hard_forbidden="
              << static_cast<double>(result.training.masks.hard_forbidden) /
                     kAttributionSteps
              << '\n';
    std::cout << prefix << ".mask.mean_soft_forbidden="
              << static_cast<double>(result.training.masks.soft_forbidden) /
                     kAttributionSteps
              << '\n';
    std::cout << prefix << ".mask.mean_relaxed_soft_forbidden="
              << static_cast<double>(
                     result.training.masks.relaxed_soft_forbidden) /
                     kAttributionSteps
              << '\n';
    for (const auto &checkpoint : result.checkpoints) {
      emit_attribution_checkpoint(prefix, checkpoint);
    }
    const double area_delta = result.checkpoints.back().probe.area -
                              result.checkpoints.front().probe.area;
    std::cout << prefix << ".probe.step32_minus_step0_area=" << area_delta
              << '\n';
  }

  if (core_decomposition) {
    const auto jepa_minus_null = repair_arm_contrast(
        results, kJmcdArms.size(), kJmcdJepaIndex, kJmcdNullIndex, test.target);
    const auto mae_minus_null = repair_arm_contrast(
        results, kJmcdArms.size(), kJmcdMaeIndex, kJmcdNullIndex, test.target);
    const auto jepa_minus_combined =
        repair_arm_contrast(results, kJmcdArms.size(), kJmcdJepaIndex,
                            kJmcdCombinedIndex, test.target);
    const auto mae_minus_combined =
        repair_arm_contrast(results, kJmcdArms.size(), kJmcdMaeIndex,
                            kJmcdCombinedIndex, test.target);
    const auto combined_minus_null =
        repair_arm_contrast(results, kJmcdArms.size(), kJmcdCombinedIndex,
                            kJmcdNullIndex, test.target);
    const auto factorial = jmcd_factorial_contrast(results, test.target);
    const auto null_geometry = jmcd_geometry_by_seed(results, kJmcdNullIndex);
    const auto combined_geometry =
        jmcd_geometry_by_seed(results, kJmcdCombinedIndex);
    const auto jepa_geometry = jmcd_geometry_by_seed(results, kJmcdJepaIndex);
    const auto mae_geometry = jmcd_geometry_by_seed(results, kJmcdMaeIndex);
    const auto jepa_minus_null_family =
        jmcd_family_deltas(results, kJmcdJepaIndex, kJmcdNullIndex);
    const auto jepa_minus_combined_family =
        jmcd_family_deltas(results, kJmcdJepaIndex, kJmcdCombinedIndex);
    const auto mae_minus_null_family =
        jmcd_family_deltas(results, kJmcdMaeIndex, kJmcdNullIndex);
    const auto mae_minus_combined_family =
        jmcd_family_deltas(results, kJmcdMaeIndex, kJmcdCombinedIndex);
    bool null_identity = true;
    for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
      null_identity =
          null_identity &&
          jmcd_null_identity(results[seed * kJmcdArms.size() + kJmcdNullIndex]);
      emit_jmcd_seed_pairing(
          kAttributionSeeds[seed],
          validate_jmcd_seed_pairing(results, seed * kJmcdArms.size(), device));
    }
    const bool mechanics_complete = jmcd_mechanics_complete(results, device);
    const auto gate = jmcd_gate::evaluate({
        .combined_minus_null = jmcd_contrast(combined_minus_null),
        .harmful_interaction_residual = factorial.summary,
        .null_geometry = null_geometry,
        .combined_geometry = combined_geometry,
        .jepa = {.minus_null = jmcd_contrast(jepa_minus_null),
                 .minus_combined = jmcd_contrast(jepa_minus_combined),
                 .geometry = jepa_geometry,
                 .minus_null_family = jepa_minus_null_family,
                 .minus_combined_family = jepa_minus_combined_family},
        .mae = {.minus_null = jmcd_contrast(mae_minus_null),
                .minus_combined = jmcd_contrast(mae_minus_combined),
                .geometry = mae_geometry,
                .minus_null_family = mae_minus_null_family,
                .minus_combined_family = mae_minus_combined_family},
        .null_identity = null_identity,
        .mechanics = mechanics_complete,
        // The exact 2,046-key accepted-reference audit is post-run. This true
        // value permits only a provisional raw classification.
        .accepted_reference_reproduced = true,
    });

    for (const auto &result : results) {
      const std::string prefix = "jmcd.seed_" + std::to_string(result.seed) +
                                 ".arm." + result.arm.name;
      const auto config = attribution_config(device, result.arm);
      emit_fingerprint(prefix + ".model_config_fingerprint",
                       fnv1a64(canonical_config_manifest(config)));
      emit_fingerprint(prefix + ".common_config_fingerprint",
                       fnv1a64(jmcd_common_config_manifest(config)));
      std::cout << prefix << ".parameter_delta.all_trainable_max_abs="
                << result.final_all_trainable_max_abs_diff << '\n';
      std::cout << prefix << ".parameter_delta.served_max_abs="
                << result.final_served_max_abs_diff << '\n';
      std::cout << prefix << ".parameter_delta.predictor_max_abs="
                << result.final_predictor_max_abs_diff << '\n';
      std::cout << prefix << ".parameter_delta.mae_decoder_max_abs="
                << result.final_mae_decoder_max_abs_diff << '\n';
      std::cout << prefix << ".parameter_delta.vicreg_head_max_abs="
                << result.final_vicreg_head_max_abs_diff << '\n';
      std::cout << prefix << ".parameter_delta.target_ema_max_abs="
                << result.final_target_ema_max_abs_diff << '\n';
      if (std::string(result.arm.name) == "core_objective_null") {
        std::cout << prefix
                  << ".identity_to_step0=" << jmcd_null_identity(result)
                  << '\n';
      }
      for (const auto &checkpoint : result.checkpoints) {
        const std::string checkpoint_prefix = prefix + ".step_" +
                                              std::to_string(checkpoint.step) +
                                              ".jmcd_gradient";
        const auto &gradient = checkpoint.gradients;
        std::cout << checkpoint_prefix
                  << ".jepa_predictor_norm=" << gradient.predictor_norm[0]
                  << '\n';
        std::cout << checkpoint_prefix
                  << ".mae_decoder_norm=" << gradient.mae_decoder_norm[1]
                  << '\n';
        std::cout << checkpoint_prefix
                  << ".jepa_mae_served_cosine=" << gradient.served_cosine[0]
                  << '\n';
        std::cout << checkpoint_prefix << ".active_all_trainable_norm="
                  << gradient.actual_arm_all_trainable_norm << '\n';
        std::cout << checkpoint_prefix
                  << ".active_served_norm=" << gradient.actual_arm_served_norm
                  << '\n';
        std::cout << checkpoint_prefix << ".all_trainable_decomposition_error="
                  << gradient.all_trainable_relative_decomposition_error
                  << '\n';
        std::cout << checkpoint_prefix << ".served_decomposition_error="
                  << gradient.served_relative_decomposition_error << '\n';
      }
      for (int64_t step = 0; step < kAttributionSteps; ++step) {
        const auto index = static_cast<std::size_t>(step);
        const std::string update_prefix =
            prefix + ".update_" + std::to_string(step) + ".mechanics";
        emit_mask_hash(update_prefix + ".clean_data_hash",
                       result.training.clean_data_hashes[index]);
        emit_mask_hash(update_prefix + ".clean_mask_hash",
                       result.training.clean_mask_hashes[index]);
        emit_mask_hash(
            update_prefix + ".forward_pre_cpu_state_hash",
            result.training.module_forward_pre_states[index].digest.cpu);
        emit_mask_hash(
            update_prefix + ".forward_pre_cuda_state_hash",
            result.training.module_forward_pre_states[index].digest.cuda);
        emit_mask_hash(
            update_prefix + ".forward_post_cpu_state_hash",
            result.training.module_forward_post_states[index].digest.cpu);
        emit_mask_hash(
            update_prefix + ".forward_post_cuda_state_hash",
            result.training.module_forward_post_states[index].digest.cuda);
      }
    }

    emit_jmcd_contrast("jepa_minus_null", jepa_minus_null);
    emit_jmcd_contrast("mae_minus_null", mae_minus_null);
    emit_jmcd_contrast("jepa_minus_combined", jepa_minus_combined);
    emit_jmcd_contrast("mae_minus_combined", mae_minus_combined);
    emit_jmcd_contrast("combined_minus_null", combined_minus_null);
    emit_jmcd_factorial_contrast(factorial);
    // Keep the accepted combined common summary byte-exact; all new summaries
    // remain outside its frozen selector.
    emit_variance_arm_summaries(results, arms, kJmcdCombinedIndex, 1,
                                "summary.arm");
    emit_variance_arm_summaries(results, arms, kJmcdJepaIndex, 3,
                                "jmcd.summary.arm");
    emit_jmcd_gate(gate, jepa_minus_null_family, jepa_minus_combined_family,
                   mae_minus_null_family, mae_minus_combined_family);
    std::cout << "jmcd.summary.mechanics_complete=" << mechanics_complete
              << '\n';
    std::cout << "jmcd.summary.null_identity=" << null_identity << '\n';
    std::cout << "jmcd.summary.reference_audit_pending=true\n";
    std::cout << "jmcd.summary.final_classification_requires_postrun_audit="
                 "true\n";
    std::cout << "next_experiment_authorized=false\n";
    std::cout << "long_run_authorized=false\n";
    std::cout << "production_or_end_to_end_authorized=false\n";
    std::cout << "execution_status=jmcd_measurements_complete\n";
    std::cout << "full_quality_qualification=false\n";
    return 0;
  }

  if (outer_augmentation) {
    for (std::size_t result_index = 0; result_index < results.size();
         ++result_index) {
      const auto arm_index = result_index % static_cast<std::size_t>(
                                                kOuterAugmentationArms.size());
      emit_outer_augmentation_arm_diagnostics(
          results[result_index], outer_preprocessing_config(device, arm_index));
    }
    const auto qualified_minus_full_active = repair_arm_contrast(
        results, kOuterAugmentationArms.size(), kOuterQualifiedIndex,
        kOuterFullActiveIndex, test.target);
    const auto qualified_minus_neutral = repair_arm_contrast(
        results, kOuterAugmentationArms.size(), kOuterQualifiedIndex,
        kOuterNeutralIndex, test.target);
    const auto full_active_minus_neutral = repair_arm_contrast(
        results, kOuterAugmentationArms.size(), kOuterFullActiveIndex,
        kOuterNeutralIndex, test.target);
    const auto neutral_geometry =
        outer_geometry_by_seed(results, kOuterNeutralIndex);
    const auto full_active_geometry =
        outer_geometry_by_seed(results, kOuterFullActiveIndex);
    const auto qualified_geometry =
        outer_geometry_by_seed(results, kOuterQualifiedIndex);
    const auto qualified_minus_full_active_family = outer_family_deltas(
        results, kOuterQualifiedIndex, kOuterFullActiveIndex);
    const auto qualified_minus_neutral_family =
        outer_family_deltas(results, kOuterQualifiedIndex, kOuterNeutralIndex);
    const bool mechanics_complete =
        semantic_qualifier.pass && outer_mechanics_complete(results);
    const auto map_contrast = [](const repair_gate::PairedContrast &contrast) {
      return outer_gate::PairedContrast{
          .point = contrast.point,
          .low = contrast.low,
          .high = contrast.high,
          .positive_seed_count = contrast.positive_seed_count,
      };
    };
    const auto gate = outer_gate::evaluate_outer_augmentation_training_gate({
        .qualified_minus_full_active =
            map_contrast(qualified_minus_full_active.summary),
        .qualified_minus_neutral =
            map_contrast(qualified_minus_neutral.summary),
        .full_active_minus_neutral =
            map_contrast(full_active_minus_neutral.summary),
        .neutral_geometry = neutral_geometry,
        .full_active_geometry = full_active_geometry,
        .qualified_geometry = qualified_geometry,
        .qualified_minus_full_active_family =
            qualified_minus_full_active_family,
        .qualified_minus_neutral_family = qualified_minus_neutral_family,
        .mechanics = mechanics_complete,
    });
    emit_outer_contrast("qualified_minus_full_active",
                        qualified_minus_full_active);
    emit_outer_contrast("qualified_minus_neutral", qualified_minus_neutral);
    emit_outer_contrast("full_active_minus_neutral", full_active_minus_neutral);
    // Preserve only the accepted neutral common-summary namespace. The two
    // treatment summaries are new aggregate diagnostics and therefore stay
    // under the frozen outer-augmentation summary namespace.
    emit_variance_arm_summaries(results, arms, kOuterNeutralIndex, 1,
                                "summary.arm");
    emit_variance_arm_summaries(results, arms, kOuterFullActiveIndex, 2,
                                "outer_augmentation.summary.arm");
    emit_outer_gate(gate, qualified_minus_full_active_family,
                    qualified_minus_neutral_family);
    std::cout << "outer_augmentation.summary.mechanics_complete="
              << mechanics_complete << '\n';
    std::cout << "outer_augmentation.summary.raw_training_classification="
              << outer_gate::classification_name(gate.classification) << '\n';
    std::cout << "outer_augmentation.summary.neutral_reference_audit_pending="
                 "true\n";
    std::cout << "outer_augmentation.summary.final_classification_requires_"
                 "postrun_audit=true\n";
    std::cout << "next_experiment_authorized=false\n";
    std::cout << "long_run_authorized=false\n";
    std::cout << "production_or_end_to_end_authorized=false\n";
    std::cout << "execution_status=outer_augmentation_representation_training_"
                 "measurements_complete\n";
    std::cout << "full_quality_qualification=false\n";
    return 0;
  }

  if (variance_necessity) {
    constexpr std::size_t kJmIndex = 0;
    constexpr std::size_t kFullStratifiedIndex = 1;
    constexpr std::size_t kNoVarianceIndex = 2;
    const auto stratified_minus_jm = repair_arm_contrast(
        results, arms.size(), kFullStratifiedIndex, kJmIndex, test.target);
    const auto no_variance_minus_stratified =
        repair_arm_contrast(results, arms.size(), kNoVarianceIndex,
                            kFullStratifiedIndex, test.target);
    const auto no_variance_minus_jm = repair_arm_contrast(
        results, arms.size(), kNoVarianceIndex, kJmIndex, test.target);
    const bool mechanics_complete = variance_checkpoint_mechanics_complete(
        results, arms.size(), variance_ablation_mechanics);
    if (!mechanics_complete) {
      throw std::runtime_error(
          "variance necessity post-training mechanics failed");
    }
    const auto jm_geometry =
        variance_geometry_by_seed(results, arms.size(), kJmIndex);
    const auto stratified_geometry =
        variance_geometry_by_seed(results, arms.size(), kFullStratifiedIndex);
    const auto no_variance_geometry =
        variance_geometry_by_seed(results, arms.size(), kNoVarianceIndex);
    const auto family_deltas = variance_family_deltas(
        results, arms.size(), kNoVarianceIndex, kJmIndex);
    const auto map_contrast = [](const repair_gate::PairedContrast &contrast) {
      return variance_gate::PairedContrast{
          .point = contrast.point,
          .low = contrast.low,
          .high = contrast.high,
          .positive_seed_count = contrast.positive_seed_count,
      };
    };
    const auto gate = variance_gate::evaluate_variance_necessity_gate({
        .stratified_minus_jm = map_contrast(stratified_minus_jm.summary),
        .variance_disabled_minus_stratified =
            map_contrast(no_variance_minus_stratified.summary),
        .variance_disabled_minus_jm =
            map_contrast(no_variance_minus_jm.summary),
        .jm_geometry = jm_geometry,
        .stratified_geometry = stratified_geometry,
        .variance_disabled_geometry = no_variance_geometry,
        .variance_disabled_minus_jm_family = family_deltas,
        .mechanics = mechanics_complete,
    });

    const auto emit_contrast = [&](const std::string &name,
                                   const RepairArmContrast &contrast) {
      for (std::size_t seed = 0; seed < kAttributionSeeds.size(); ++seed) {
        std::cout << "seed_" << kAttributionSeeds[seed] << ".contrast." << name
                  << ".step32_probe_area=" << contrast.per_seed[seed] << '\n';
      }
      const std::string prefix = "contrast." + name + ".step32_probe_area";
      std::cout << prefix << ".fixed_seed_mean=" << contrast.summary.point
                << '\n';
      std::cout << prefix << ".bootstrap_95_low=" << contrast.summary.low
                << '\n';
      std::cout << prefix << ".bootstrap_95_high=" << contrast.summary.high
                << '\n';
      std::cout << prefix << ".positive_seed_count="
                << contrast.summary.positive_seed_count << '\n';
    };
    emit_contrast(
        "jepa_mae_plus_projected_channel_stratified_vicreg_minus_jepa_mae_"
        "only",
        stratified_minus_jm);
    emit_contrast(
        "jepa_mae_plus_projected_channel_stratified_vicreg_no_variance_minus_"
        "jepa_mae_plus_projected_channel_stratified_vicreg",
        no_variance_minus_stratified);
    emit_contrast(
        "jepa_mae_plus_projected_channel_stratified_vicreg_no_variance_minus_"
        "jepa_mae_only",
        no_variance_minus_jm);
    emit_variance_arm_summaries(results, arms);
    emit_variance_necessity_gate(gate, family_deltas);
    std::cout << "variance_necessity_gate.mechanics_complete="
              << mechanics_complete << '\n';
    std::string classification;
    if (!gate.numeric_inputs_valid || !gate.mechanics_pass) {
      classification = "invalid";
    } else if (gate.reference_not_reproduced) {
      classification = "reference_not_reproduced";
    } else if (gate.pass) {
      classification = "variance_necessity_supported";
    } else if (gate.partial_amelioration) {
      classification = "partial_amelioration";
    } else {
      classification = "variance_necessity_not_supported";
    }
    std::cout << "variance_necessity_gate.classification=" << classification
              << '\n';
    std::cout << "variance_component_necessity_supported=" << gate.pass << '\n';
    std::cout << "next_experiment_authorized=false\n";
    std::cout << "execution_status=vicreg_variance_component_necessity_"
                 "measurements_complete\n";
    std::cout << "full_quality_qualification=false\n";
    return 0;
  }

  const auto fixed_tf_minus_jm = repair_arm_contrast(
      results, arms.size(), kRepairFixedTfIndex, kRepairJmIndex, test.target);
  const auto warmup_minus_fixed_tf =
      repair_arm_contrast(results, arms.size(), kRepairWarmupIndex,
                          kRepairFixedTfIndex, test.target);
  const auto warmup_minus_jm = repair_arm_contrast(
      results, arms.size(), kRepairWarmupIndex, kRepairJmIndex, test.target);
  const auto global_vicreg_minus_jm =
      repair_arm_contrast(results, arms.size(), kRepairGlobalVicregIndex,
                          kRepairJmIndex, test.target);
  const auto stratified_minus_global_vicreg =
      repair_arm_contrast(results, arms.size(), kRepairStratifiedVicregIndex,
                          kRepairGlobalVicregIndex, test.target);
  const auto stratified_minus_jm =
      repair_arm_contrast(results, arms.size(), kRepairStratifiedVicregIndex,
                          kRepairJmIndex, test.target);
  const bool mechanics_complete =
      repair_mechanics_complete(results, arms.size());
  if (!mechanics_complete) {
    throw std::runtime_error("repair gate mechanics completion failed");
  }
  const auto jm_geometry =
      repair_geometry_by_seed(results, arms.size(), kRepairJmIndex);
  const auto fixed_tf_geometry =
      repair_geometry_by_seed(results, arms.size(), kRepairFixedTfIndex);
  const auto warmup_geometry =
      repair_geometry_by_seed(results, arms.size(), kRepairWarmupIndex);
  const auto global_vicreg_geometry =
      repair_geometry_by_seed(results, arms.size(), kRepairGlobalVicregIndex);
  const auto stratified_geometry = repair_geometry_by_seed(
      results, arms.size(), kRepairStratifiedVicregIndex);
  const auto tf_ratios = repair_tf_step_zero_ratios(results, arms.size());
  const auto tf_family_deltas = repair_family_deltas(
      results, arms.size(), kRepairWarmupIndex, kRepairJmIndex);
  const auto vicreg_family_deltas = repair_family_deltas(
      results, arms.size(), kRepairStratifiedVicregIndex, kRepairJmIndex);
  const auto tf_gate = repair_gate::evaluate_tf_repair_gate({
      .fixed_tf_minus_jm = fixed_tf_minus_jm.summary,
      .candidate_minus_fixed_tf = warmup_minus_fixed_tf.summary,
      .candidate_minus_jm = warmup_minus_jm.summary,
      .jm_geometry = jm_geometry,
      .fixed_tf_geometry = fixed_tf_geometry,
      .candidate_geometry = warmup_geometry,
      .candidate_minus_jm_family = tf_family_deltas,
      .tf_weighted_norm_ratios = tf_ratios,
      .mechanics = mechanics_complete,
  });
  const auto vicreg_gate = repair_gate::evaluate_vicreg_repair_gate({
      .global_vicreg_minus_jm = global_vicreg_minus_jm.summary,
      .candidate_minus_global_vicreg = stratified_minus_global_vicreg.summary,
      .candidate_minus_jm = stratified_minus_jm.summary,
      .jm_geometry = jm_geometry,
      .global_vicreg_geometry = global_vicreg_geometry,
      .candidate_geometry = stratified_geometry,
      .candidate_minus_jm_family = vicreg_family_deltas,
      .mechanics = mechanics_complete,
  });

  for (std::size_t arm_index = 1; arm_index < kAttributionArms.size();
       ++arm_index) {
    std::vector<ProbeCurve> candidate_curves;
    std::vector<ProbeCurve> reference_curves;
    candidate_curves.reserve(kAttributionSeeds.size());
    reference_curves.reserve(kAttributionSeeds.size());
    double point_contrast = 0.0;
    for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
         ++seed_index) {
      const auto &reference = results[seed_index * kAttributionArms.size()]
                                  .checkpoints.back()
                                  .probe;
      const auto &candidate =
          results[seed_index * kAttributionArms.size() + arm_index]
              .checkpoints.back()
              .probe;
      reference_curves.push_back(reference);
      candidate_curves.push_back(candidate);
      const double seed_contrast = candidate.area - reference.area;
      point_contrast += seed_contrast;
      std::cout << "seed_" << kAttributionSeeds[seed_index] << ".contrast."
                << kAttributionArms[arm_index].name
                << "_minus_full_soft.step32_probe_area=" << seed_contrast
                << '\n';
    }
    point_contrast /= static_cast<double>(kAttributionSeeds.size());
    const auto bootstrap = paired_arm_bootstrap(
        candidate_curves, reference_curves, test.target,
        kAttributionBootstrapReplicates,
        0x617474725f626f6fULL ^ static_cast<uint64_t>(arm_index));
    const std::string contrast_prefix =
        "contrast." + std::string(kAttributionArms[arm_index].name) +
        "_minus_full_soft.step32_probe_area";
    std::cout << contrast_prefix << ".fixed_seed_mean=" << point_contrast
              << '\n';
    std::cout << contrast_prefix
              << ".bootstrap_95_low=" << bootstrap.interval.low << '\n';
    std::cout << contrast_prefix
              << ".bootstrap_95_high=" << bootstrap.interval.high << '\n';
    std::cout << contrast_prefix
              << ".positive_seed_count=" << bootstrap.positive_seed_count
              << '\n';
  }
  for (const std::size_t arm_index :
       {std::size_t{4}, std::size_t{5}, std::size_t{6}, std::size_t{7}}) {
    const RepairArmContrast *contrast = nullptr;
    switch (arm_index) {
    case kRepairFixedTfIndex:
      contrast = &fixed_tf_minus_jm;
      break;
    case kRepairGlobalVicregIndex:
      contrast = &global_vicreg_minus_jm;
      break;
    case kRepairWarmupIndex:
      contrast = &warmup_minus_jm;
      break;
    case kRepairStratifiedVicregIndex:
      contrast = &stratified_minus_jm;
      break;
    default:
      throw std::runtime_error("unknown repair arm contrast");
    }
    for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
         ++seed_index) {
      std::cout << "seed_" << kAttributionSeeds[seed_index] << ".contrast."
                << kAttributionArms[arm_index].name
                << "_minus_jepa_mae_only.step32_probe_area="
                << contrast->per_seed[seed_index] << '\n';
    }
    const std::string contrast_prefix =
        "contrast." + std::string(kAttributionArms[arm_index].name) +
        "_minus_jepa_mae_only.step32_probe_area";
    std::cout << contrast_prefix
              << ".fixed_seed_mean=" << contrast->summary.point << '\n';
    std::cout << contrast_prefix
              << ".bootstrap_95_low=" << contrast->summary.low << '\n';
    std::cout << contrast_prefix
              << ".bootstrap_95_high=" << contrast->summary.high << '\n';
    std::cout << contrast_prefix << ".positive_seed_count="
              << contrast->summary.positive_seed_count << '\n';
  }

  {
    for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
         ++seed_index) {
      std::cout << "seed_" << kAttributionSeeds[seed_index]
                << ".contrast.jepa_mae_plus_tf_gradient_matched_warmup_minus_"
                   "jepa_mae_plus_tf.step32_probe_area="
                << warmup_minus_fixed_tf.per_seed[seed_index] << '\n';
    }
    const std::string contrast_prefix =
        "contrast.jepa_mae_plus_tf_gradient_matched_warmup_minus_"
        "jepa_mae_plus_tf.step32_probe_area";
    std::cout << contrast_prefix
              << ".fixed_seed_mean=" << warmup_minus_fixed_tf.summary.point
              << '\n';
    std::cout << contrast_prefix
              << ".bootstrap_95_low=" << warmup_minus_fixed_tf.summary.low
              << '\n';
    std::cout << contrast_prefix
              << ".bootstrap_95_high=" << warmup_minus_fixed_tf.summary.high
              << '\n';
    std::cout << contrast_prefix << ".positive_seed_count="
              << warmup_minus_fixed_tf.summary.positive_seed_count << '\n';
  }

  {
    for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
         ++seed_index) {
      std::cout
          << "seed_" << kAttributionSeeds[seed_index]
          << ".contrast.jepa_mae_plus_projected_channel_stratified_vicreg_"
             "minus_jepa_mae_plus_vicreg.step32_probe_area="
          << stratified_minus_global_vicreg.per_seed[seed_index] << '\n';
    }
    const std::string contrast_prefix =
        "contrast.jepa_mae_plus_projected_channel_stratified_vicreg_minus_"
        "jepa_mae_plus_vicreg.step32_probe_area";
    std::cout << contrast_prefix << ".fixed_seed_mean="
              << stratified_minus_global_vicreg.summary.point << '\n';
    std::cout << contrast_prefix << ".bootstrap_95_low="
              << stratified_minus_global_vicreg.summary.low << '\n';
    std::cout << contrast_prefix << ".bootstrap_95_high="
              << stratified_minus_global_vicreg.summary.high << '\n';
    std::cout << contrast_prefix << ".positive_seed_count="
              << stratified_minus_global_vicreg.summary.positive_seed_count
              << '\n';
  }

  constexpr std::array<int64_t, 3> kSummarySteps{0, 16, 32};
  for (std::size_t arm_index = 0; arm_index < kAttributionArms.size();
       ++arm_index) {
    for (std::size_t checkpoint_index = 0;
         checkpoint_index < kSummarySteps.size(); ++checkpoint_index) {
      double area_mean = 0.0;
      std::array<double, kFamilies> family_mean{};
      double channel_mean_effective_rank = 0.0;
      double channel_mean_participation_rank = 0.0;
      double channel_max_top_eigen_share = 0.0;
      double channel_min_active_fraction = 0.0;
      for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
           ++seed_index) {
        const auto &checkpoint =
            results[seed_index * kAttributionArms.size() + arm_index]
                .checkpoints[checkpoint_index];
        area_mean += checkpoint.probe.area;
        const auto &final_score = checkpoint.probe.points.back().score;
        for (int64_t family = 0; family < kFamilies; ++family) {
          family_mean[static_cast<std::size_t>(family)] +=
              final_score.family[static_cast<std::size_t>(family)];
        }
        double seed_effective_rank = 0.0;
        double seed_participation_rank = 0.0;
        double seed_top_eigen_share = 0.0;
        double seed_active_fraction = 1.0;
        for (const auto &channel : checkpoint.geometry) {
          seed_effective_rank += channel.effective_rank_ratio;
          seed_participation_rank += channel.participation_rank_ratio;
          seed_top_eigen_share =
              std::max(seed_top_eigen_share, channel.top_eigenvalue_share);
          seed_active_fraction =
              std::min(seed_active_fraction, channel.active_dimension_fraction);
        }
        channel_mean_effective_rank +=
            seed_effective_rank / static_cast<double>(kChannels);
        channel_mean_participation_rank +=
            seed_participation_rank / static_cast<double>(kChannels);
        channel_max_top_eigen_share += seed_top_eigen_share;
        channel_min_active_fraction += seed_active_fraction;
      }
      const double seed_denominator =
          static_cast<double>(kAttributionSeeds.size());
      const std::string prefix =
          "summary.arm." + std::string(kAttributionArms[arm_index].name) +
          ".step_" + std::to_string(kSummarySteps[checkpoint_index]);
      std::cout << prefix << ".optimizer_lambda_tf_align="
                << attribution_arm_weights(kAttributionArms[arm_index],
                                           kSummarySteps[checkpoint_index])[2]
                << '\n';
      std::cout << prefix << ".probe_area_fixed_seed_mean="
                << area_mean / seed_denominator << '\n';
      for (int64_t family = 0; family < kFamilies; ++family) {
        std::cout << prefix << ".family_"
                  << kFamilyNames[static_cast<std::size_t>(family)]
                  << "_r2_fixed_seed_mean="
                  << family_mean[static_cast<std::size_t>(family)] /
                         seed_denominator
                  << '\n';
      }
      std::cout << prefix
                << ".geometry.channel_mean_effective_rank_ratio_fixed_seed_"
                   "mean="
                << channel_mean_effective_rank / seed_denominator << '\n';
      std::cout << prefix
                << ".geometry.channel_mean_participation_rank_ratio_fixed_"
                   "seed_mean="
                << channel_mean_participation_rank / seed_denominator << '\n';
      std::cout << prefix
                << ".geometry.channel_max_top_eigenvalue_share_fixed_seed_"
                   "mean="
                << channel_max_top_eigen_share / seed_denominator << '\n';
      std::cout << prefix
                << ".geometry.channel_min_active_dimension_fraction_fixed_"
                   "seed_mean="
                << channel_min_active_fraction / seed_denominator << '\n';
    }
    double area_delta_mean = 0.0;
    for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
         ++seed_index) {
      const auto &arm_result =
          results[seed_index * kAttributionArms.size() + arm_index];
      area_delta_mean += arm_result.checkpoints.back().probe.area -
                         arm_result.checkpoints.front().probe.area;
    }
    std::cout << "summary.arm." << kAttributionArms[arm_index].name
              << ".step32_minus_step0.probe_area_fixed_seed_mean="
              << area_delta_mean / static_cast<double>(kAttributionSeeds.size())
              << '\n';
  }

  constexpr std::size_t kFullSoftIndex = 0;
  for (std::size_t checkpoint_index = 0;
       checkpoint_index < kSummarySteps.size(); ++checkpoint_index) {
    std::array<double, 4> branch_norm_mean{};
    std::array<double, 6> cosine_sum{};
    std::array<int64_t, 6> cosine_valid_count{};
    double jepa_mae_norm_mean = 0.0;
    double tf_vicreg_norm_mean = 0.0;
    double total_norm_mean = 0.0;
    double cancellation_mean = 0.0;
    for (std::size_t seed_index = 0; seed_index < kAttributionSeeds.size();
         ++seed_index) {
      const auto &gradient =
          results[seed_index * kAttributionArms.size() + kFullSoftIndex]
              .checkpoints[checkpoint_index]
              .gradients;
      for (std::size_t branch = 0; branch < branch_norm_mean.size(); ++branch) {
        branch_norm_mean[branch] += gradient.served_norm[branch];
      }
      for (std::size_t cosine = 0; cosine < cosine_sum.size(); ++cosine) {
        if (std::isfinite(gradient.served_cosine[cosine])) {
          cosine_sum[cosine] += gradient.served_cosine[cosine];
          ++cosine_valid_count[cosine];
        }
      }
      jepa_mae_norm_mean += gradient.jepa_mae_norm;
      tf_vicreg_norm_mean += gradient.tf_vicreg_norm;
      total_norm_mean += gradient.canonical_total_norm;
      cancellation_mean += gradient.cancellation_ratio;
    }
    const double seed_denominator =
        static_cast<double>(kAttributionSeeds.size());
    const std::string prefix = "summary.arm.full_soft.step_" +
                               std::to_string(kSummarySteps[checkpoint_index]) +
                               ".gradient";
    for (std::size_t branch = 0; branch < branch_norm_mean.size(); ++branch) {
      std::cout << prefix << ".branch_" << kAttributionBranchNames[branch]
                << "_served_norm_fixed_seed_mean="
                << branch_norm_mean[branch] / seed_denominator << '\n';
    }
    std::size_t cosine_index = 0;
    for (std::size_t left = 0; left < kAttributionBranchNames.size(); ++left) {
      for (std::size_t right = left + 1; right < kAttributionBranchNames.size();
           ++right) {
        const double cosine_mean =
            cosine_valid_count[cosine_index] > 0
                ? cosine_sum[cosine_index] /
                      static_cast<double>(cosine_valid_count[cosine_index])
                : std::numeric_limits<double>::quiet_NaN();
        std::cout << prefix << ".cosine_" << kAttributionBranchNames[left]
                  << "__" << kAttributionBranchNames[right]
                  << "_fixed_seed_mean=" << cosine_mean << '\n';
        std::cout << prefix << ".cosine_" << kAttributionBranchNames[left]
                  << "__" << kAttributionBranchNames[right]
                  << "_valid_seed_count=" << cosine_valid_count[cosine_index]
                  << '\n';
        ++cosine_index;
      }
    }
    std::cout << prefix << ".jepa_mae_norm_fixed_seed_mean="
              << jepa_mae_norm_mean / seed_denominator << '\n';
    std::cout << prefix << ".tf_vicreg_norm_fixed_seed_mean="
              << tf_vicreg_norm_mean / seed_denominator << '\n';
    std::cout << prefix << ".canonical_total_norm_fixed_seed_mean="
              << total_norm_mean / seed_denominator << '\n';
    std::cout << prefix << ".cancellation_ratio_fixed_seed_mean="
              << cancellation_mean / seed_denominator << '\n';
  }
  emit_tf_repair_gate(tf_gate, tf_ratios);
  emit_vicreg_repair_gate(vicreg_gate);
  for (std::size_t family = 0; family < kFamilies; ++family) {
    std::cout << "repair_gate.tf.family_delta." << kFamilyNames[family] << '='
              << tf_family_deltas[family] << '\n';
    std::cout << "repair_gate.vicreg.family_delta." << kFamilyNames[family]
              << '=' << vicreg_family_deltas[family] << '\n';
  }
  std::cout << "repair_gate.mechanics_complete=" << mechanics_complete << '\n';
  const bool combined_arm_authorized = tf_gate.pass && vicreg_gate.pass;
  std::cout << "combined_arm_authorized=" << combined_arm_authorized << '\n';
  std::string repair_classification;
  if (tf_gate.common.reference_not_reproduced &&
      vicreg_gate.common.reference_not_reproduced) {
    repair_classification = "both_references_not_reproduced";
  } else if (tf_gate.common.reference_not_reproduced) {
    repair_classification = "tf_reference_not_reproduced";
  } else if (vicreg_gate.common.reference_not_reproduced) {
    repair_classification = "vicreg_reference_not_reproduced";
  } else if (combined_arm_authorized) {
    repair_classification = "independent_repairs_pass_combined_authorized";
  } else if (tf_gate.pass) {
    repair_classification = "tf_repair_pass_vicreg_repair_fail";
  } else if (vicreg_gate.pass) {
    repair_classification = "tf_repair_fail_vicreg_repair_pass";
  } else {
    repair_classification = "independent_repairs_failed";
  }
  std::cout << "repair_gate.classification=" << repair_classification << '\n';
  std::cout << "summary.conditional_split_trigger="
               "jepa_mae_only_rescued_full_v2\n";
  std::cout << "execution_status=objective_mask_attribution_measurements_"
               "complete\n";
  std::cout << "causal_classification=not_applied_in_executable\n";
  std::cout << "full_quality_qualification=false\n";
  return 0;
}

int run_outer_augmentation_preflight(const Options &options) {
  if (options.device != "cuda" || !torch::cuda::is_available()) {
    throw std::runtime_error("outer augmentation preflight requires CUDA");
  }
  const torch::Device device(torch::kCUDA, 0);
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);
  validate_attribution_arm_configs(
      device, std::vector<AttributionArm>(kOuterAugmentationArms.begin(),
                                          kOuterAugmentationArms.end()));
  validate_outer_augmentation_configs(device);
  validate_outer_augmentation_seed_domain();
  if (options.qualifier_log.empty()) {
    throw std::runtime_error(
        "outer augmentation preflight requires --qualifier-log");
  }
  const auto qualifier =
      parse_semantic_qualifier_evidence(options.qualifier_log);
  if (!qualifier.pass) {
    throw std::runtime_error("outer augmentation preflight qualifier failed");
  }
  const auto clean_data =
      torch::linspace(-1.0, 1.0, 4 * kChannels * kHistory * kFeatures,
                      torch::TensorOptions().dtype(torch::kFloat32))
          .reshape({4, kChannels, kHistory, kFeatures})
          .contiguous();
  const auto clean_mask = torch::ones_like(clean_data, torch::kBool);
  std::array<uint64_t, kOuterAugmentationArms.size()> preprocessing_hashes{};
  uint64_t model_hash = 0;
  bool mechanics = true;
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.outer_augmentation_"
               "training_preflight.v1\n";
  constexpr const char *summary_prefix = "outer_augmentation.summary.preflight";
  std::cout << summary_prefix << ".optimizer_steps=0\n";
  std::cout << summary_prefix << ".semantic_qualifier_key_counts="
            << (qualifier.schema_count == 1 &&
                qualifier.candidate_qualified_count == 1 &&
                qualifier.full_active_not_qualified_count == 1 &&
                qualifier.global_not_qualified_count == 1)
            << '\n';
  std::cout << summary_prefix << ".semantic_qualifier_pass=" << qualifier.pass
            << '\n';
  for (std::size_t arm_index = 0; arm_index < kOuterAugmentationArms.size();
       ++arm_index) {
    const auto model_config =
        attribution_config(device, kOuterAugmentationArms[arm_index]);
    const auto preprocessing = outer_preprocessing_config(device, arm_index);
    const auto current_model_hash =
        fnv1a64(canonical_config_manifest(model_config));
    preprocessing_hashes[arm_index] =
        fnv1a64(canonical_preprocessing_manifest(preprocessing));
    if (arm_index == 0) {
      model_hash = current_model_hash;
    }
    const bool model_config_exact = current_model_hash == model_hash;
    const auto replayed = apply_outer_augmentation_replayed(
        clean_data, clean_mask, preprocessing,
        outer_augmentation_seed(kAttributionSeeds.front(), 0), device);
    const auto &diagnostic = replayed.diagnostic;
    const bool arm_mechanics =
        model_config_exact && diagnostic.augmentation_replay_exact &&
        diagnostic.augmentation_consumed_state_exact &&
        diagnostic.augmentation_cuda_unchanged &&
        diagnostic.augmentation_state_restored &&
        diagnostic.masked_values_zero &&
        diagnostic.retention.every_sample_channel_nonempty &&
        (arm_index != kOuterNeutralIndex ||
         diagnostic.neutral_identity_exact) &&
        (arm_index != kOuterQualifiedIndex ||
         (diagnostic.qualified_data_changed &&
          diagnostic.qualified_mask_exact));
    mechanics = mechanics && arm_mechanics;
    const std::string prefix =
        "outer_augmentation.seed_" + std::to_string(kAttributionSeeds.front()) +
        ".arm." + std::string(kOuterAugmentationArms[arm_index].name) +
        ".preflight";
    emit_fingerprint(prefix + ".model_config_fingerprint", current_model_hash);
    emit_fingerprint(prefix + ".preprocessing_config_fingerprint",
                     preprocessing_hashes[arm_index]);
    std::cout << prefix << ".augmentation_replay_exact="
              << diagnostic.augmentation_replay_exact << '\n';
    std::cout << prefix << ".pass=" << arm_mechanics << '\n';
  }
  const bool preprocessing_distinct =
      preprocessing_hashes[kOuterNeutralIndex] !=
          preprocessing_hashes[kOuterFullActiveIndex] &&
      preprocessing_hashes[kOuterNeutralIndex] !=
          preprocessing_hashes[kOuterQualifiedIndex] &&
      preprocessing_hashes[kOuterFullActiveIndex] !=
          preprocessing_hashes[kOuterQualifiedIndex];
  mechanics = mechanics && preprocessing_distinct;
  std::cout << summary_prefix
            << ".preprocessing_fingerprints_distinct=" << preprocessing_distinct
            << '\n';
  std::cout << summary_prefix << ".pass=" << mechanics << '\n';
  return mechanics ? 0 : 3;
}

int run_jmcd_preflight(const Options &options) {
  if (options.device != "cuda" || !torch::cuda::is_available()) {
    throw std::runtime_error("JMCD preflight requires CUDA");
  }
  if (options.steps > 0 || options.seeds > 0 || !options.weak_views) {
    throw std::runtime_error(
        "JMCD preflight accepts no training-step/seed override");
  }
  const torch::Device device(torch::kCUDA, 0);
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);
  validate_attribution_arm_configs(
      device, std::vector<AttributionArm>(kJmcdArms.begin(), kJmcdArms.end()));
  validate_jmcd_arm_table(device);

  auto ssl = generate_dataset(0, 256);
  const auto normalization = fit_normalization(ssl);
  normalize(ssl, normalization);
  validate_dataset(ssl);
  const auto rows = training_rows(ssl, kAttributionSeeds.front(), 0);
  const auto row_index = torch::tensor(rows, torch::kInt64);
  const auto data = ssl.data.index_select(0, row_index).to(device);
  const auto mask = ssl.mask.index_select(0, row_index).to(device);

  ParameterSnapshot parameter_reference{};
  torch::Tensor target_mask_reference{};
  torch::Tensor context_mask_reference{};
  WeakViewDigest weak_view_reference{};
  GeneratorStateSnapshot forward_pre_reference{};
  GeneratorStateSnapshot forward_post_reference{};
  std::array<double, 2> raw_loss_reference{};
  std::array<double, 2> raw_served_norm_reference{};
  std::array<double, 2> raw_branch_head_norm_reference{};
  double jepa_mae_cosine_reference{0.0};
  uint64_t common_config_fingerprint = 0;
  bool mechanics = true;

  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.jmcd_preflight.v1\n";
  std::cout << "jmcd.summary.preflight.optimizer_steps=0\n";
  std::cout << "jmcd.summary.preflight.arm_count=" << kJmcdArms.size() << '\n';
  for (std::size_t arm_index = 0; arm_index < kJmcdArms.size(); ++arm_index) {
    const auto &arm = kJmcdArms[arm_index];
    set_paired_rng(kAttributionSeeds.front(), device);
    auto model = mtf::MtfJepaMaeVicreg(attribution_config(device, arm));
    if (arm_index == 0) {
      parameter_reference = snapshot_parameters(model);
    }
    const bool initialization_exact =
        parameter_max_abs_diff(model, parameter_reference) == 0.0;
    set_paired_rng(paired_step_seed(kAttributionSeeds.front(), 0), device);
    const auto forward_pre = current_generator_state_snapshot(device);
    model->train();
    const auto output = model->forward(data, mask);
    const auto forward_post = current_generator_state_snapshot(device);
    validate_weak_view_debug_tensors(output, data, mask);
    const auto weak_view = weak_view_digest(output);
    const std::array<double, 2> raw_loss{output.loss_jepa.item<double>(),
                                         output.loss_mae.item<double>()};
    const auto diagnostic = checkpoint_gradient_diagnostic(
        model, ssl, device, arm, kAttributionSeeds.front(), 0);
    const std::array<double, 2> raw_served_norm{diagnostic.served_norm[0],
                                                diagnostic.served_norm[1]};
    const std::array<double, 2> raw_branch_head_norm{
        diagnostic.predictor_norm[0], diagnostic.mae_decoder_norm[1]};
    const double jepa_mae_cosine = diagnostic.served_cosine[0];
    const auto config = attribution_config(device, arm);
    const uint64_t full_fingerprint =
        fnv1a64(canonical_config_manifest(config));
    const uint64_t common_fingerprint =
        fnv1a64(jmcd_common_config_manifest(config));

    bool paired_forward_exact = true;
    if (arm_index == 0) {
      target_mask_reference = output.jepa_target_mask.to(torch::kCPU).clone();
      context_mask_reference = output.jepa_context_mask.to(torch::kCPU).clone();
      weak_view_reference = weak_view;
      forward_pre_reference = forward_pre;
      forward_post_reference = forward_post;
      raw_loss_reference = raw_loss;
      raw_served_norm_reference = raw_served_norm;
      raw_branch_head_norm_reference = raw_branch_head_norm;
      jepa_mae_cosine_reference = jepa_mae_cosine;
      common_config_fingerprint = common_fingerprint;
    } else {
      paired_forward_exact =
          torch::equal(output.jepa_target_mask.to(torch::kCPU),
                       target_mask_reference) &&
          torch::equal(output.jepa_context_mask.to(torch::kCPU),
                       context_mask_reference) &&
          weak_view_digests_equal(weak_view, weak_view_reference) &&
          generator_state_snapshot_equal(forward_pre, forward_pre_reference) &&
          generator_state_snapshot_equal(forward_post,
                                         forward_post_reference) &&
          raw_loss == raw_loss_reference &&
          raw_served_norm == raw_served_norm_reference &&
          raw_branch_head_norm == raw_branch_head_norm_reference &&
          jepa_mae_cosine == jepa_mae_cosine_reference &&
          common_fingerprint == common_config_fingerprint;
    }
    const bool active_gradient_exact =
        arm_index == kJmcdNullIndex
            ? diagnostic.actual_arm_all_trainable_norm == 0.0 &&
                  diagnostic.actual_arm_served_norm == 0.0
            : diagnostic.actual_arm_all_trainable_norm > 0.0 &&
                  diagnostic.actual_arm_served_norm > 0.0;
    const bool gradient_contract =
        raw_loss[0] > 0.0 && raw_loss[1] > 0.0 && raw_served_norm[0] > 0.0 &&
        raw_served_norm[1] > 0.0 && raw_branch_head_norm[0] > 0.0 &&
        raw_branch_head_norm[1] > 0.0 && std::isfinite(jepa_mae_cosine) &&
        diagnostic.all_trainable_relative_decomposition_error <= 1.0e-5 &&
        diagnostic.served_relative_decomposition_error <= 1.0e-5 &&
        active_gradient_exact;
    const bool arm_pass = initialization_exact && paired_forward_exact &&
                          gradient_contract &&
                          common_fingerprint == common_config_fingerprint;
    mechanics = mechanics && arm_pass;
    const std::string prefix = "jmcd.seed_" +
                               std::to_string(kAttributionSeeds.front()) +
                               ".arm." + arm.name + ".preflight";
    emit_fingerprint(prefix + ".full_config_fingerprint", full_fingerprint);
    emit_fingerprint(prefix + ".common_config_fingerprint", common_fingerprint);
    std::cout << prefix << ".initialization_exact=" << initialization_exact
              << '\n';
    std::cout << prefix << ".paired_forward_exact=" << paired_forward_exact
              << '\n';
    std::cout << prefix << ".raw_jepa_loss=" << raw_loss[0] << '\n';
    std::cout << prefix << ".raw_mae_loss=" << raw_loss[1] << '\n';
    std::cout << prefix << ".jepa_mae_served_cosine=" << jepa_mae_cosine
              << '\n';
    std::cout << prefix << ".active_all_trainable_norm="
              << diagnostic.actual_arm_all_trainable_norm << '\n';
    std::cout << prefix
              << ".active_served_norm=" << diagnostic.actual_arm_served_norm
              << '\n';
    std::cout << prefix << ".gradient_contract=" << gradient_contract << '\n';
    std::cout << prefix << ".pass=" << arm_pass << '\n';
  }
  emit_fingerprint("jmcd.summary.preflight.clean_data_hash",
                   hash_tensor_stable_bytes(data));
  emit_fingerprint("jmcd.summary.preflight.clean_mask_hash",
                   hash_tensor_stable_bytes(mask));
  std::cout << "jmcd.summary.preflight.pass=" << mechanics << '\n';
  return mechanics ? 0 : 3;
}

constexpr std::size_t kRssmSurfaceCount = 4;
constexpr std::size_t kRssmTrackCount = 2;
constexpr int64_t kRssmTokensPerChannel = 24;
constexpr int64_t kRssmTokenChannelWidth = kRssmTokensPerChannel * kLatentDim;
constexpr uint64_t kRssmTokenProjectionTag = 0x7273736d5f74655fULL;
constexpr uint64_t kRssmShuffleTrainTag = 0x7273736d5f74726eULL;
constexpr uint64_t kRssmShuffleValidationTag = 0x7273736d5f76616cULL;
constexpr uint64_t kRssmShuffleTestTag = 0x7273736d5f746573ULL;
constexpr uint64_t kRssmOrderShuffleTrainTag = 0x7273736d5f6f7472ULL;
constexpr uint64_t kRssmOrderShuffleValidationTag = 0x7273736d5f6f7661ULL;
constexpr uint64_t kRssmOrderShuffleTestTag = 0x7273736d5f6f7465ULL;
constexpr uint64_t kRssmBootstrapSeed = 8387496322364763509ULL;
constexpr int64_t kRssmBootstrapReplicates = 512;
constexpr std::array<int64_t, 4> kRssmSampleLadder{32, 64, 128, 256};
constexpr std::array<const char *, kRssmSurfaceCount> kRssmSurfaceNames{
    "raw", "tokenizer", "encoder", "served"};
constexpr std::array<const char *, kRssmTrackCount> kRssmTrackNames{"native",
                                                                    "fixed96"};

enum class RssmSurface : std::size_t {
  raw = 0,
  tokenizer = 1,
  encoder = 2,
  served = 3
};
enum class RssmTrack : std::size_t { native = 0, fixed96 = 1 };

[[nodiscard]] constexpr std::size_t rssm_index(RssmSurface value) {
  return static_cast<std::size_t>(value);
}

[[nodiscard]] constexpr std::size_t rssm_index(RssmTrack value) {
  return static_cast<std::size_t>(value);
}

struct RssmEncodedCapture {
  torch::Tensor tokenizer_by_channel{}; // [S,C,768], CPU float64
  torch::Tensor encoder_by_channel{};   // [S,C,768], CPU float64
  torch::Tensor served_by_channel{};    // [S,C,32], CPU float64
  torch::Tensor grouped_metadata_layout{}; // [C,24,4], CPU int64
  uint64_t tokenizer_source_hash{0};
  uint64_t encoder_source_hash{0};
  uint64_t served_source_hash{0};
  uint64_t tokenizer_float64_hash{0};
  uint64_t encoder_float64_hash{0};
  uint64_t served_float64_hash{0};
  uint64_t token_mask_structure_hash{0};
  uint64_t metadata_structure_hash{0};
  bool public_sandwich_exact{true};
  bool direct_encoder_exact{true};
  bool production_order_exact{true};
  bool cardinality_exact{true};
};

struct RssmSurfaceFeatures {
  torch::Tensor native_by_channel{};
  torch::Tensor native_flat{};
  torch::Tensor fixed96_by_channel{};
  torch::Tensor fixed96_flat{};
};

[[nodiscard]] uint64_t
rssm_parameter_snapshot_hash(const ParameterSnapshot &snapshot) {
  if (snapshot.names.size() != snapshot.values.size()) {
    throw std::runtime_error("RSSM parameter snapshot layout mismatch");
  }
  uint64_t hash = 0xcbf29ce484222325ULL;
  mix_hash_value(hash, static_cast<uint64_t>(snapshot.names.size()));
  for (std::size_t index = 0; index < snapshot.names.size(); ++index) {
    mix_hash_value(hash, fnv1a64(snapshot.names[index]));
    mix_hash_value(hash, hash_tensor_stable_bytes(snapshot.values[index]));
  }
  return hash;
}

[[nodiscard]] RssmSurfaceFeatures
rssm_raw_surface(const Dataset &dataset, const torch::Tensor &raw_projection);

using RssmFeatureSet = std::array<RssmSurfaceFeatures, kRssmSurfaceCount>;

[[nodiscard]] bool rssm_tensor_bytes_equal(const torch::Tensor &left,
                                           const torch::Tensor &right) {
  if (!left.defined() || !right.defined() ||
      left.scalar_type() != right.scalar_type() ||
      left.sizes() != right.sizes()) {
    return false;
  }
  const auto lhs = left.detach().to(torch::kCPU).contiguous();
  const auto rhs = right.detach().to(torch::kCPU).contiguous();
  const std::size_t bytes = static_cast<std::size_t>(lhs.numel()) *
                            static_cast<std::size_t>(lhs.element_size());
  return bytes == 0 || std::memcmp(lhs.data_ptr(), rhs.data_ptr(), bytes) == 0;
}

[[nodiscard]] bool
rssm_metadata_bytes_equal(const mtf::mtf_token_metadata_t &left,
                          const mtf::mtf_token_metadata_t &right) {
  return rssm_tensor_bytes_equal(left.start_index, right.start_index) &&
         rssm_tensor_bytes_equal(left.width, right.width) &&
         rssm_tensor_bytes_equal(left.scale_id, right.scale_id) &&
         rssm_tensor_bytes_equal(left.channel_id, right.channel_id) &&
         rssm_tensor_bytes_equal(left.domain_id, right.domain_id);
}

[[nodiscard]] bool
rssm_token_batch_bytes_equal(const mtf::mtf_token_batch_t &left,
                             const mtf::mtf_token_batch_t &right) {
  return rssm_tensor_bytes_equal(left.tokens, right.tokens) &&
         rssm_tensor_bytes_equal(left.reconstruction_targets,
                                 right.reconstruction_targets) &&
         rssm_tensor_bytes_equal(left.time_reconstruction_targets,
                                 right.time_reconstruction_targets) &&
         rssm_tensor_bytes_equal(left.frequency_reconstruction_targets,
                                 right.frequency_reconstruction_targets) &&
         rssm_tensor_bytes_equal(left.time_reconstruction_mask,
                                 right.time_reconstruction_mask) &&
         rssm_tensor_bytes_equal(left.frequency_reconstruction_mask,
                                 right.frequency_reconstruction_mask) &&
         rssm_tensor_bytes_equal(left.token_mask, right.token_mask) &&
         rssm_metadata_bytes_equal(left.metadata, right.metadata);
}

struct RssmTokenOrder {
  std::array<std::vector<int64_t>, kChannels> channel_indices{};
  bool production_order_exact{true};
  bool cardinality_exact{true};
};

struct RssmTokenizerPlanReceipt {
  int64_t total_tokens{0};
  int64_t clipped_full_history_collisions{0};
  int64_t shorter_window_tokens{0};
  int64_t shorter_tokens_changed{0};
  bool metadata_and_masks_exact{false};
  bool pass{false};
};

[[nodiscard]] RssmTokenizerPlanReceipt rssm_tokenizer_plan_receipt() {
  auto config = active_config(torch::Device(torch::kCPU), /*weak_views=*/true);
  config.dtype = torch::kFloat64;
  config.device = torch::Device(torch::kCPU);
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
  torch::manual_seed(1202);
  auto builder = mtf::TimeFrequencyViewBuilder(config);
  builder->eval();
  torch::NoGradGuard no_grad;
  const auto original = builder->forward(input);
  const auto reversed = builder->forward(input.flip({2}));
  RssmTokenizerPlanReceipt result{};
  result.total_tokens = original.tokens.size(1);
  result.metadata_and_masks_exact =
      rssm_metadata_bytes_equal(original.metadata, reversed.metadata) &&
      rssm_tensor_bytes_equal(original.token_mask, reversed.token_mask) &&
      rssm_tensor_bytes_equal(original.time_reconstruction_mask,
                              reversed.time_reconstruction_mask) &&
      rssm_tensor_bytes_equal(original.frequency_reconstruction_mask,
                              reversed.frequency_reconstruction_mask);

  const auto starts =
      original.metadata.start_index.to(torch::kCPU, torch::kInt64).contiguous();
  const auto widths =
      original.metadata.width.to(torch::kCPU, torch::kInt64).contiguous();
  const auto scales =
      original.metadata.scale_id.to(torch::kCPU, torch::kInt64).contiguous();
  const auto start = starts.accessor<int64_t, 1>();
  const auto width = widths.accessor<int64_t, 1>();
  const auto scale = scales.accessor<int64_t, 1>();
  bool clipped_exact = true;
  for (int64_t token = 0; token < original.tokens.size(1); ++token) {
    const bool clipped = start[token] == 0 && width[token] == kHistory &&
                         (scale[token] == 2 || scale[token] == 3);
    const bool equal =
        torch::allclose(original.tokens.index({0, token}),
                        reversed.tokens.index({0, token}), 1.0e-9, 1.0e-10);
    if (clipped) {
      ++result.clipped_full_history_collisions;
      clipped_exact = clipped_exact && equal;
    } else {
      ++result.shorter_window_tokens;
      result.shorter_tokens_changed += equal ? 0 : 1;
    }
  }
  result.pass = result.metadata_and_masks_exact && clipped_exact &&
                result.total_tokens == 72 &&
                result.clipped_full_history_collisions == 12 &&
                result.shorter_window_tokens == 60 &&
                result.shorter_tokens_changed == 60;
  return result;
}

[[nodiscard]] RssmTokenOrder
rssm_token_order(const mtf::mtf_token_metadata_t &metadata,
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
    throw std::runtime_error("RSSM token metadata width mismatch");
  }
  const auto channel = channels.accessor<int64_t, 1>();
  const auto domain = domains.accessor<int64_t, 1>();
  const auto scale = scales.accessor<int64_t, 1>();
  const auto start = starts.accessor<int64_t, 1>();
  const auto width = widths.accessor<int64_t, 1>();
  RssmTokenOrder result{};
  for (int64_t token = 0; token < token_count; ++token) {
    if (channel[token] < 0 || channel[token] >= kChannels ||
        domain[token] < 0 || domain[token] > 1 || scale[token] < 0 ||
        scale[token] >= 4) {
      throw std::runtime_error("RSSM token metadata value out of range");
    }
    result.channel_indices[static_cast<std::size_t>(channel[token])].push_back(
        token);
    if (token > 0) {
      result.production_order_exact =
          result.production_order_exact &&
          std::tuple{domain[token - 1], channel[token - 1], scale[token - 1],
                     start[token - 1],  width[token - 1],   token - 1} <=
              std::tuple{domain[token], channel[token], scale[token],
                         start[token],  width[token],   token};
    }
  }
  for (auto &indices : result.channel_indices) {
    std::sort(indices.begin(), indices.end(), [&](int64_t lhs, int64_t rhs) {
      return std::tuple{domain[lhs], scale[lhs], start[lhs], width[lhs], lhs} <
             std::tuple{domain[rhs], scale[rhs], start[rhs], width[rhs], rhs};
    });
  }
  result.cardinality_exact = token_count == 72;
  for (int64_t channel_id = 0; channel_id < kChannels; ++channel_id) {
    const auto &indices =
        result.channel_indices[static_cast<std::size_t>(channel_id)];
    int64_t time_count = 0;
    int64_t frequency_count = 0;
    for (const auto token : indices) {
      time_count += domain[token] == 0 ? 1 : 0;
      frequency_count += domain[token] == 1 ? 1 : 0;
    }
    result.cardinality_exact =
        result.cardinality_exact &&
        indices.size() == static_cast<std::size_t>(kRssmTokensPerChannel) &&
        time_count == 12 && frequency_count == 12;
  }
  return result;
}

[[nodiscard]] torch::Tensor
rssm_group_tokens_by_channel(const torch::Tensor &tokens,
                             const RssmTokenOrder &order) {
  if (tokens.dim() != 3 || tokens.size(1) != 72 ||
      tokens.size(2) != kLatentDim || !order.cardinality_exact) {
    throw std::runtime_error("RSSM token tensor shape mismatch");
  }
  std::vector<torch::Tensor> channels;
  channels.reserve(kChannels);
  for (const auto &indices : order.channel_indices) {
    const auto index = torch::tensor(
        indices,
        torch::TensorOptions().dtype(torch::kInt64).device(tokens.device()));
    channels.push_back(tokens.index_select(1, index).reshape(
        {tokens.size(0), kRssmTokenChannelWidth}));
  }
  return torch::stack(channels, 1).contiguous();
}

[[nodiscard]] RssmEncodedCapture rssm_capture_once(mtf::MtfJepaMaeVicreg &model,
                                                   const Dataset &dataset,
                                                   const torch::Device &device,
                                                   bool check_direct_encoder) {
  const bool was_training = model->is_training();
  model->eval();
  torch::NoGradGuard no_grad;
  std::vector<torch::Tensor> tokenizer_source_chunks;
  std::vector<torch::Tensor> encoder_source_chunks;
  std::vector<torch::Tensor> served_source_chunks;
  uint64_t token_mask_structure_hash = 0xcbf29ce484222325ULL;
  uint64_t metadata_structure_hash = 0;
  torch::Tensor grouped_metadata_layout{};
  bool metadata_initialized = false;
  bool public_exact = true;
  bool direct_exact = true;
  bool production_order_exact = true;
  bool cardinality_exact = true;
  for (int64_t begin = 0; begin < dataset.data.size(0);
       begin += kModelRowBatchSize) {
    const int64_t size =
        std::min<int64_t>(kModelRowBatchSize, dataset.data.size(0) - begin);
    const auto data = dataset.data.narrow(0, begin, size).to(device);
    const auto feature_mask = dataset.mask.narrow(0, begin, size).to(device);
    const auto tokens_before = model->tokenize(data, feature_mask);
    const auto encoded = model->encode(data, feature_mask);
    const auto served = mtf::select_mtf_serving_pool(
        encoded, mtf::mtf_serving_pool_policy_t::all_tokens, model->config());
    const auto tokens_after = model->tokenize(data, feature_mask);
    public_exact =
        public_exact &&
        rssm_token_batch_bytes_equal(tokens_before, tokens_after) &&
        rssm_tensor_bytes_equal(tokens_before.token_mask, encoded.token_mask) &&
        rssm_metadata_bytes_equal(tokens_before.metadata, encoded.metadata);
    mix_hash_value(token_mask_structure_hash,
                   hash_tensor_stable_bytes(tokens_before.token_mask));
    uint64_t batch_metadata_hash = 0xcbf29ce484222325ULL;
    mix_hash_value(
        batch_metadata_hash,
        hash_tensor_stable_bytes(tokens_before.metadata.start_index));
    mix_hash_value(batch_metadata_hash,
                   hash_tensor_stable_bytes(tokens_before.metadata.width));
    mix_hash_value(batch_metadata_hash,
                   hash_tensor_stable_bytes(tokens_before.metadata.scale_id));
    mix_hash_value(batch_metadata_hash,
                   hash_tensor_stable_bytes(tokens_before.metadata.channel_id));
    mix_hash_value(batch_metadata_hash,
                   hash_tensor_stable_bytes(tokens_before.metadata.domain_id));
    if (!metadata_initialized) {
      metadata_structure_hash = batch_metadata_hash;
      metadata_initialized = true;
    } else {
      public_exact =
          public_exact && metadata_structure_hash == batch_metadata_hash;
    }
    if (!tokens_before.token_mask.all().item<bool>() ||
        !encoded.sample_valid_mask.all().item<bool>() ||
        !encoded.channel_valid_mask.all().item<bool>() ||
        !served.valid_mask.all().item<bool>()) {
      throw std::runtime_error("RSSM fully observed row became invalid");
    }
    const auto order =
        rssm_token_order(tokens_before.metadata, tokens_before.tokens.size(1));
    production_order_exact =
        production_order_exact && order.production_order_exact;
    cardinality_exact = cardinality_exact && order.cardinality_exact;
    const auto metadata_layout = torch::stack(
        {tokens_before.metadata.domain_id, tokens_before.metadata.scale_id,
         tokens_before.metadata.start_index, tokens_before.metadata.width},
        1).to(torch::kCPU, torch::kInt64).contiguous();
    std::vector<torch::Tensor> grouped_layout_channels;
    grouped_layout_channels.reserve(kChannels);
    for (const auto &indices : order.channel_indices) {
      grouped_layout_channels.push_back(metadata_layout.index_select(
          0, torch::tensor(indices, torch::kInt64)));
    }
    const auto batch_grouped_metadata_layout =
        torch::stack(grouped_layout_channels, 0).contiguous();
    if (!grouped_metadata_layout.defined()) {
      grouped_metadata_layout = batch_grouped_metadata_layout;
    } else {
      public_exact =
          public_exact && rssm_tensor_bytes_equal(
                              grouped_metadata_layout,
                              batch_grouped_metadata_layout);
    }
    const auto tokenizer_by_channel =
        rssm_group_tokens_by_channel(tokens_before.tokens, order);
    const auto encoder_by_channel =
        rssm_group_tokens_by_channel(encoded.embeddings, order);
    if (served.values.sizes() !=
        torch::IntArrayRef({size, kChannels, kLatentDim})) {
      throw std::runtime_error("RSSM served tensor shape mismatch");
    }
    if (check_direct_encoder) {
      auto children = model->named_children();
      const auto *entry = children.find(std::string{"encoder"});
      if (entry == nullptr) {
        throw std::runtime_error("RSSM registered online encoder is missing");
      }
      auto *online_encoder = (*entry)->as<mtf::SharedTokenEncoder>();
      if (online_encoder == nullptr) {
        throw std::runtime_error("RSSM registered encoder type mismatch");
      }
      const auto direct_encoder = online_encoder->forward(
          tokens_before.tokens, tokens_before.token_mask);
      const auto direct_served = mtf::detail::pooled_by_channel(
          direct_encoder, tokens_before.token_mask, tokens_before.metadata,
          model->config());
      direct_exact =
          direct_exact &&
          rssm_tensor_bytes_equal(direct_encoder, encoded.embeddings) &&
          rssm_tensor_bytes_equal(direct_served, served.values);
    }
    tokenizer_source_chunks.push_back(
        tokenizer_by_channel.detach().to(torch::kCPU));
    encoder_source_chunks.push_back(
        encoder_by_channel.detach().to(torch::kCPU));
    served_source_chunks.push_back(served.values.detach().to(torch::kCPU));
  }
  model->train(was_training);
  auto tokenizer_source = torch::cat(tokenizer_source_chunks, 0).contiguous();
  auto encoder_source = torch::cat(encoder_source_chunks, 0).contiguous();
  auto served_source = torch::cat(served_source_chunks, 0).contiguous();
  auto tokenizer_float64 = tokenizer_source.to(torch::kFloat64).contiguous();
  auto encoder_float64 = encoder_source.to(torch::kFloat64).contiguous();
  auto served_float64 = served_source.to(torch::kFloat64).contiguous();
  return {
      .tokenizer_by_channel = tokenizer_float64,
      .encoder_by_channel = encoder_float64,
      .served_by_channel = served_float64,
      .grouped_metadata_layout = grouped_metadata_layout,
      .tokenizer_source_hash = hash_tensor_stable_bytes(tokenizer_source),
      .encoder_source_hash = hash_tensor_stable_bytes(encoder_source),
      .served_source_hash = hash_tensor_stable_bytes(served_source),
      .tokenizer_float64_hash = hash_tensor_stable_bytes(tokenizer_float64),
      .encoder_float64_hash = hash_tensor_stable_bytes(encoder_float64),
      .served_float64_hash = hash_tensor_stable_bytes(served_float64),
      .token_mask_structure_hash = token_mask_structure_hash,
      .metadata_structure_hash = metadata_structure_hash,
      .public_sandwich_exact = public_exact,
      .direct_encoder_exact = direct_exact,
      .production_order_exact = production_order_exact,
      .cardinality_exact = cardinality_exact,
  };
}

[[nodiscard]] bool rssm_capture_exact(const RssmEncodedCapture &left,
                                      const RssmEncodedCapture &right) {
  return left.tokenizer_source_hash == right.tokenizer_source_hash &&
         left.encoder_source_hash == right.encoder_source_hash &&
         left.served_source_hash == right.served_source_hash &&
         left.tokenizer_float64_hash == right.tokenizer_float64_hash &&
         left.encoder_float64_hash == right.encoder_float64_hash &&
         left.served_float64_hash == right.served_float64_hash &&
         left.token_mask_structure_hash == right.token_mask_structure_hash &&
         left.metadata_structure_hash == right.metadata_structure_hash &&
         rssm_tensor_bytes_equal(left.grouped_metadata_layout,
                                 right.grouped_metadata_layout) &&
         rssm_tensor_bytes_equal(left.tokenizer_by_channel,
                                 right.tokenizer_by_channel) &&
         rssm_tensor_bytes_equal(left.encoder_by_channel,
                                 right.encoder_by_channel) &&
         rssm_tensor_bytes_equal(left.served_by_channel,
                                 right.served_by_channel);
}

[[nodiscard]] torch::Tensor rssm_make_token_projection() {
  torch::NoGradGuard no_grad;
  auto dense =
      torch::empty({kRssmTokenChannelWidth, kLatentDim}, torch::kFloat64);
  auto values = dense.accessor<double, 2>();
  for (int64_t row = 0; row < kRssmTokenChannelWidth; ++row) {
    for (int64_t column = 0; column < kLatentDim; ++column) {
      const uint64_t projection_key = splitmix64(
          kRssmTokenProjectionTag ^ splitmix64(static_cast<uint64_t>(row)) ^
          splitmix64(static_cast<uint64_t>(column) << 32U));
      values[row][column] = signed_uniform(projection_key);
    }
  }
  auto [projection, upper] = at::linalg_qr(dense, "reduced");
  const auto diagonal = upper.diagonal();
  const auto signs = torch::where(diagonal.lt(0.0), -torch::ones_like(diagonal),
                                  torch::ones_like(diagonal));
  projection = projection * signs.unsqueeze(0);
  const auto identity = torch::eye(kLatentDim, torch::kFloat64);
  const double error =
      (projection.transpose(0, 1).matmul(projection) - identity)
          .abs()
          .max()
          .item<double>();
  if (projection.sizes() !=
          torch::IntArrayRef({kRssmTokenChannelWidth, kLatentDim}) ||
      !torch::isfinite(projection).all().item<bool>() || error > 1.0e-10) {
    throw std::runtime_error("RSSM shared token projection contract failed");
  }
  return projection.contiguous();
}

[[nodiscard]] double rssm_projection_error(const torch::Tensor &projection) {
  const auto identity = torch::eye(
      projection.size(1), torch::TensorOptions().dtype(torch::kFloat64));
  return (projection.transpose(0, 1).matmul(projection) - identity)
      .abs()
      .max()
      .item<double>();
}

[[nodiscard]] torch::Tensor
rssm_project_by_channel(const torch::Tensor &by_channel,
                        const torch::Tensor &projection) {
  if (by_channel.dim() != 3 || by_channel.size(1) != kChannels ||
      by_channel.size(2) != projection.size(0) ||
      projection.size(1) != kLatentDim) {
    throw std::runtime_error("RSSM channel projection shape mismatch");
  }
  return by_channel.to(torch::kCPU, torch::kFloat64)
      .matmul(projection)
      .contiguous();
}

[[nodiscard]] RssmFeatureSet
rssm_feature_set(const RssmSurfaceFeatures &raw_surface,
                 const RssmEncodedCapture &capture,
                 const torch::Tensor &token_projection) {
  RssmFeatureSet result{};
  result[rssm_index(RssmSurface::raw)] = raw_surface;
  const int64_t rows = capture.tokenizer_by_channel.size(0);
  if (raw_surface.native_by_channel.size(0) != rows) {
    throw std::runtime_error("RSSM raw/captured row mismatch");
  }

  const std::array<torch::Tensor, 2> token_surfaces{
      capture.tokenizer_by_channel, capture.encoder_by_channel};
  for (std::size_t index = 0; index < token_surfaces.size(); ++index) {
    auto &surface = result[index + rssm_index(RssmSurface::tokenizer)];
    surface.native_by_channel = token_surfaces[index];
    surface.native_flat =
        token_surfaces[index].reshape({rows, -1}).contiguous();
    surface.fixed96_by_channel =
        rssm_project_by_channel(token_surfaces[index], token_projection);
    surface.fixed96_flat =
        surface.fixed96_by_channel.reshape({rows, -1}).contiguous();
  }

  auto &served = result[rssm_index(RssmSurface::served)];
  served.native_by_channel = capture.served_by_channel;
  served.native_flat =
      capture.served_by_channel.reshape({rows, -1}).contiguous();
  served.fixed96_by_channel = served.native_by_channel;
  served.fixed96_flat = served.native_flat;
  return result;
}

[[nodiscard]] torch::Tensor rssm_sattolo_permutation(int64_t rows,
                                                     uint64_t tag) {
  if (rows < 2) {
    throw std::runtime_error("RSSM derangement requires at least two rows");
  }
  std::vector<int64_t> indices(static_cast<std::size_t>(rows));
  std::iota(indices.begin(), indices.end(), int64_t{0});
  uint64_t state = splitmix64(tag ^ splitmix64(static_cast<uint64_t>(rows)));
  for (int64_t index = rows - 1; index > 0; --index) {
    state = splitmix64(state);
    const int64_t selected =
        static_cast<int64_t>(state % static_cast<uint64_t>(index));
    std::swap(indices[static_cast<std::size_t>(index)],
              indices[static_cast<std::size_t>(selected)]);
  }
  const auto permutation = torch::tensor(indices, torch::kInt64);
  const auto identity = torch::arange(rows, torch::kInt64);
  if (std::set<int64_t>(indices.begin(), indices.end()).size() !=
          static_cast<std::size_t>(rows) ||
      permutation.eq(identity).any().item<bool>()) {
    throw std::runtime_error("RSSM Sattolo derangement contract failed");
  }
  return permutation.contiguous();
}

struct RssmPermutationReceipt {
  int64_t rows{0};
  int64_t unique_count{0};
  int64_t fixed_point_count{0};
  uint64_t hash{0};
  bool pass{false};
};

[[nodiscard]] RssmPermutationReceipt
rssm_permutation_receipt(const torch::Tensor &permutation) {
  const auto values =
      permutation.to(torch::kCPU, torch::kInt64).contiguous().reshape({-1});
  const auto identity = torch::arange(values.numel(), torch::kInt64);
  RssmPermutationReceipt result{};
  result.rows = values.numel();
  const auto access = values.accessor<int64_t, 1>();
  std::set<int64_t> unique;
  for (int64_t index = 0; index < values.numel(); ++index) {
    unique.insert(access[index]);
  }
  result.unique_count = static_cast<int64_t>(unique.size());
  result.fixed_point_count = values.eq(identity).sum().item<int64_t>();
  result.hash = hash_tensor_stable_bytes(values);
  result.pass = result.rows >= 2 && result.unique_count == result.rows &&
                result.fixed_point_count == 0;
  return result;
}

void rssm_emit_permutation(const std::string &prefix,
                           const torch::Tensor &permutation) {
  const auto values =
      permutation.to(torch::kCPU, torch::kInt64).contiguous().reshape({-1});
  const auto receipt = rssm_permutation_receipt(values);
  std::cout << prefix << ".rows=" << receipt.rows << '\n';
  std::cout << prefix << ".unique_count=" << receipt.unique_count << '\n';
  std::cout << prefix << ".fixed_point_count=" << receipt.fixed_point_count
            << '\n';
  emit_fingerprint(prefix + ".hash", receipt.hash);
  std::cout << prefix << ".values=";
  const auto access = values.accessor<int64_t, 1>();
  for (int64_t index = 0; index < values.numel(); ++index) {
    if (index != 0) {
      std::cout << ',';
    }
    std::cout << access[index];
  }
  std::cout << '\n';
  std::cout << prefix << ".pass=" << receipt.pass << '\n';
}

[[nodiscard]] std::vector<torch::Tensor> rssm_bootstrap_rows(int64_t groups) {
  std::vector<torch::Tensor> result;
  result.reserve(kRssmBootstrapReplicates);
  for (int64_t replicate = 0; replicate < kRssmBootstrapReplicates;
       ++replicate) {
    uint64_t state = splitmix64(kRssmBootstrapSeed ^
                                splitmix64(static_cast<uint64_t>(replicate)));
    std::vector<int64_t> rows;
    rows.reserve(static_cast<std::size_t>(groups));
    for (int64_t draw = 0; draw < groups; ++draw) {
      state = splitmix64(state);
      rows.push_back(
          static_cast<int64_t>(state % static_cast<uint64_t>(groups)));
    }
    result.push_back(torch::tensor(rows, torch::kInt64));
  }
  return result;
}

[[nodiscard]] bool
rssm_bootstrap_contract(const std::vector<torch::Tensor> &rows,
                        int64_t groups) {
  if (rows.size() != static_cast<std::size_t>(kRssmBootstrapReplicates)) {
    return false;
  }
  for (const auto &replicate : rows) {
    if (replicate.scalar_type() != torch::kInt64 || replicate.dim() != 1 ||
        replicate.numel() != groups || replicate.min().item<int64_t>() < 0 ||
        replicate.max().item<int64_t>() >= groups) {
      return false;
    }
  }
  return true;
}

[[nodiscard]] uint64_t
rssm_tensor_vector_hash(const std::vector<torch::Tensor> &values) {
  uint64_t hash = 0xcbf29ce484222325ULL;
  mix_hash_value(hash, static_cast<uint64_t>(values.size()));
  for (const auto &value : values) {
    mix_hash_value(hash, hash_tensor_stable_bytes(value));
  }
  return hash;
}

[[nodiscard]] RidgeModel fit_ridge_dual(const torch::Tensor &features_input,
                                        const torch::Tensor &target_input,
                                        double alpha) {
  torch::NoGradGuard no_grad;
  const auto features = features_input.to(torch::kCPU, torch::kFloat64);
  const auto target = target_input.to(torch::kCPU, torch::kFloat64);
  const auto mean = features.mean(0);
  const auto variance = (features - mean).pow(2).mean(0);
  const auto inv_std = torch::where(variance > 1.0e-12, variance.rsqrt(),
                                    torch::ones_like(variance));
  const auto x = (features - mean) * inv_std;
  const auto bias = target.mean(0);
  const auto y = target - bias;
  auto gram = x.matmul(x.transpose(0, 1));
  gram.diagonal(0, 0, 1).add_(features.size(0) * alpha);
  auto [cholesky, info] = at::linalg_cholesky_ex(gram, false, false);
  if (info.max().item<int64_t>() != 0) {
    throw std::runtime_error("RSSM dual ridge Cholesky factorization failed");
  }
  const auto dual = at::cholesky_solve(y, cholesky, false);
  auto weights = x.transpose(0, 1).matmul(dual);
  if (!torch::isfinite(weights).all().item<bool>()) {
    throw std::runtime_error("RSSM dual ridge solution is non-finite");
  }
  return {.mean = mean,
          .inv_std = inv_std,
          .weights = std::move(weights),
          .bias = bias};
}

[[nodiscard]] ProbePoint rssm_probe_at_sample_count(
    const torch::Tensor &train_features,
    const torch::Tensor &validation_features,
    const torch::Tensor &test_features, const torch::Tensor &train_target,
    const torch::Tensor &validation_target, const torch::Tensor &test_target,
    int64_t samples, bool dual) {
  const auto x = train_features.narrow(0, 0, samples);
  const auto y = train_target.narrow(0, 0, samples);
  std::array<RidgeModel, kRidgeGrid.size()> models{};
  std::array<torch::Tensor, kRidgeGrid.size()> validation_predictions{};
  for (std::size_t index = 0; index < kRidgeGrid.size(); ++index) {
    models[index] = dual ? fit_ridge_dual(x, y, kRidgeGrid[index])
                         : fit_ridge(x, y, kRidgeGrid[index]);
    validation_predictions[index] = predict(models[index], validation_features);
  }
  auto chosen = torch::empty({test_target.size(0), kTargets}, torch::kFloat64);
  ProbePoint result{};
  result.samples = samples;
  std::array<std::size_t, kTargets> selected{};
  for (int64_t task = 0; task < kTargets; ++task) {
    double best_mse = std::numeric_limits<double>::infinity();
    std::size_t best = 0;
    for (std::size_t index = 0; index < kRidgeGrid.size(); ++index) {
      const double mse = (validation_predictions[index].select(1, task) -
                          validation_target.select(1, task))
                             .pow(2)
                             .mean()
                             .item<double>();
      if (mse < best_mse) {
        best_mse = mse;
        best = index;
      }
    }
    selected[static_cast<std::size_t>(task)] = best;
    result.selected_alpha[static_cast<std::size_t>(task)] = kRidgeGrid[best];
  }
  std::array<torch::Tensor, kRidgeGrid.size()> selected_test_predictions{};
  for (int64_t task = 0; task < kTargets; ++task) {
    const std::size_t best = selected[static_cast<std::size_t>(task)];
    if (!selected_test_predictions[best].defined()) {
      selected_test_predictions[best] = predict(models[best], test_features);
    }
    chosen.select(1, task).copy_(
        selected_test_predictions[best].select(1, task));
  }
  result.prediction = std::move(chosen);
  result.score = score(result.prediction, test_target);
  return result;
}

[[nodiscard]] ProbeCurve
rssm_probe_curve(const torch::Tensor &train_features,
                 const torch::Tensor &validation_features,
                 const torch::Tensor &test_features,
                 const torch::Tensor &train_target,
                 const torch::Tensor &validation_target,
                 const torch::Tensor &test_target, bool dual) {
  ProbeCurve result{};
  for (const int64_t samples : {32, 64, 128, 256}) {
    result.points.push_back(rssm_probe_at_sample_count(
        train_features, validation_features, test_features, train_target,
        validation_target, test_target, samples, dual));
    result.area += result.points.back().score.macro;
  }
  result.area /= static_cast<double>(result.points.size());
  return result;
}

[[nodiscard]] std::array<double, kFamilies>
rssm_family_areas(const ProbeCurve &curve) {
  std::array<double, kFamilies> result{};
  if (curve.points.empty()) {
    throw std::runtime_error("RSSM family AULC requires probe points");
  }
  for (const auto &point : curve.points) {
    for (std::size_t family = 0; family < result.size(); ++family) {
      result[family] += point.score.family[family];
    }
  }
  for (double &value : result) {
    value /= static_cast<double>(curve.points.size());
  }
  return result;
}

[[nodiscard]] double rssm_primal_dual_max_abs_difference(int64_t rows,
                                                         int64_t dimensions,
                                                         double alpha) {
  auto features = torch::empty({rows, dimensions}, torch::kFloat64);
  auto target = torch::empty({rows, 3}, torch::kFloat64);
  auto x = features.accessor<double, 2>();
  auto y = target.accessor<double, 2>();
  for (int64_t row = 0; row < rows; ++row) {
    for (int64_t column = 0; column < dimensions; ++column) {
      x[row][column] =
          std::sin(0.17 * static_cast<double>((row + 1) * (column + 2))) +
          0.013 * static_cast<double>(row - column);
    }
    for (int64_t task = 0; task < 3; ++task) {
      y[row][task] =
          std::cos(0.11 * static_cast<double>((row + 3) * (task + 1))) +
          0.07 * static_cast<double>(row + task);
    }
  }
  const auto primal = fit_ridge(features, target, alpha);
  const auto dual = fit_ridge_dual(features, target, alpha);
  return (predict(primal, features) - predict(dual, features))
      .abs()
      .max()
      .item<double>();
}

struct RssmOrderPoint {
  int64_t samples{0};
  torch::Tensor prediction{}; // [2*test_groups,1], CPU float64
  double selected_alpha{0.0};
  double accuracy{0.0};
};

struct RssmOrderCurve {
  std::vector<RssmOrderPoint> points{};
  double area{0.0};
};

[[nodiscard]] torch::Tensor
rssm_interleave_pairs(const torch::Tensor &original,
                      const torch::Tensor &reversed) {
  if (original.sizes() != reversed.sizes() || original.dim() != 2) {
    throw std::runtime_error("RSSM reversal feature pairing mismatch");
  }
  return torch::stack({original, reversed}, 1)
      .reshape({2 * original.size(0), original.size(1)})
      .contiguous();
}

[[nodiscard]] torch::Tensor rssm_order_labels(int64_t groups) {
  auto labels = torch::empty({groups, 2, 1}, torch::kFloat64);
  labels.select(1, 0).fill_(1.0);
  labels.select(1, 1).fill_(-1.0);
  return labels.reshape({2 * groups, 1}).contiguous();
}

using RssmOrderFitPermutations =
    std::array<torch::Tensor, kRssmSampleLadder.size()>;
using RssmOrderFitTargets = std::array<torch::Tensor, kRssmSampleLadder.size()>;

[[nodiscard]] RssmOrderFitPermutations rssm_order_fit_permutations() {
  RssmOrderFitPermutations result{};
  for (std::size_t index = 0; index < kRssmSampleLadder.size(); ++index) {
    result[index] = rssm_sattolo_permutation(2 * kRssmSampleLadder[index],
                                             kRssmOrderShuffleTrainTag);
  }
  return result;
}

[[nodiscard]] RssmOrderFitTargets
rssm_order_fit_targets(const RssmOrderFitPermutations *permutations) {
  RssmOrderFitTargets result{};
  for (std::size_t index = 0; index < kRssmSampleLadder.size(); ++index) {
    auto labels = rssm_order_labels(kRssmSampleLadder[index]);
    result[index] =
        permutations == nullptr
            ? std::move(labels)
            : labels.index_select(0, (*permutations)[index]).contiguous();
  }
  return result;
}

[[nodiscard]] bool rssm_order_target_balanced(const torch::Tensor &target) {
  return target.dim() == 2 && target.size(1) == 1 && target.size(0) % 2 == 0 &&
         target.gt(0.0).sum().item<int64_t>() == target.size(0) / 2 &&
         target.lt(0.0).sum().item<int64_t>() == target.size(0) / 2;
}

[[nodiscard]] double rssm_order_accuracy(const torch::Tensor &prediction,
                                         const torch::Tensor &target) {
  if (prediction.sizes() != target.sizes() || prediction.dim() != 2 ||
      prediction.size(1) != 1) {
    throw std::runtime_error("RSSM order score shape mismatch");
  }
  return prediction.ge(0.0)
      .eq(target.ge(0.0))
      .to(torch::kFloat64)
      .mean()
      .item<double>();
}

[[nodiscard]] RssmOrderCurve
rssm_order_curve(const torch::Tensor &train_features,
                 const torch::Tensor &validation_features,
                 const torch::Tensor &test_features,
                 const RssmOrderFitTargets &train_targets,
                 const torch::Tensor &validation_target,
                 const torch::Tensor &test_target, bool dual) {
  RssmOrderCurve result{};
  for (std::size_t sample_index = 0; sample_index < kRssmSampleLadder.size();
       ++sample_index) {
    const int64_t samples = kRssmSampleLadder[sample_index];
    const int64_t rows = 2 * samples;
    const auto x = train_features.narrow(0, 0, rows);
    const auto &y = train_targets[sample_index];
    if (y.sizes() != torch::IntArrayRef({rows, 1})) {
      throw std::runtime_error("RSSM order fit target ladder mismatch");
    }
    double best_mse = std::numeric_limits<double>::infinity();
    std::size_t best = 0;
    std::array<RidgeModel, kRidgeGrid.size()> models{};
    for (std::size_t index = 0; index < kRidgeGrid.size(); ++index) {
      models[index] = dual ? fit_ridge_dual(x, y, kRidgeGrid[index])
                           : fit_ridge(x, y, kRidgeGrid[index]);
      const auto validation_prediction =
          predict(models[index], validation_features);
      const double mse = (validation_prediction - validation_target)
                             .pow(2)
                             .mean()
                             .item<double>();
      if (mse < best_mse) {
        best_mse = mse;
        best = index;
      }
    }
    RssmOrderPoint point{};
    point.samples = samples;
    point.prediction = predict(models[best], test_features);
    point.selected_alpha = kRidgeGrid[best];
    point.accuracy = rssm_order_accuracy(point.prediction, test_target);
    result.area += point.accuracy;
    result.points.push_back(std::move(point));
  }
  result.area /= static_cast<double>(result.points.size());
  return result;
}

void rssm_validate_order_curve_finite(const RssmOrderCurve &curve,
                                      const std::string &label) {
  if (curve.points.size() != kRssmSampleLadder.size() ||
      !std::isfinite(curve.area) || curve.area < 0.0 || curve.area > 1.0) {
    throw std::runtime_error(label + " order curve is invalid");
  }
  for (std::size_t index = 0; index < curve.points.size(); ++index) {
    const auto &point = curve.points[index];
    const bool alpha_known =
        std::find(kRidgeGrid.begin(), kRidgeGrid.end(), point.selected_alpha) !=
        kRidgeGrid.end();
    if (point.samples != kRssmSampleLadder[index] ||
        !point.prediction.defined() ||
        !torch::isfinite(point.prediction).all().item<bool>() ||
        !std::isfinite(point.accuracy) || point.accuracy < 0.0 ||
        point.accuracy > 1.0 || !alpha_known) {
      throw std::runtime_error(label + " order point is invalid");
    }
  }
}

struct RssmAreaSummary {
  double macro{0.0};
  std::array<double, kFamilies> family{};
};

[[nodiscard]] RssmAreaSummary
rssm_resampled_area(const ProbeCurve &curve, const torch::Tensor &target,
                    const torch::Tensor &row_index) {
  if (curve.points.empty() || target.dim() != 2 || target.size(1) != kTargets) {
    throw std::runtime_error("RSSM resampled curve contract failed");
  }
  RssmAreaSummary result{};
  const auto sampled_target = target.index_select(0, row_index);
  for (const auto &point : curve.points) {
    const auto sampled_prediction = point.prediction.index_select(0, row_index);
    const auto sampled_score = score(sampled_prediction, sampled_target);
    result.macro += sampled_score.macro;
    for (std::size_t family = 0; family < result.family.size(); ++family) {
      result.family[family] += sampled_score.family[family];
    }
  }
  const double denominator = static_cast<double>(curve.points.size());
  result.macro /= denominator;
  for (double &value : result.family) {
    value /= denominator;
  }
  return result;
}

using RssmCurveBySeed = std::array<ProbeCurve, rssm_gate::kSeedCount>;
using RssmOrderCurveBySeed = std::array<RssmOrderCurve, rssm_gate::kSeedCount>;

[[nodiscard]] rssm_gate::TransitionInput rssm_contrast(
    const RssmCurveBySeed &downstream, const torch::Tensor &downstream_target,
    const RssmCurveBySeed &upstream, const torch::Tensor &upstream_target,
    const std::vector<torch::Tensor> &bootstrap_rows) {
  rssm_gate::TransitionInput result{};
  for (std::size_t seed = 0; seed < rssm_gate::kSeedCount; ++seed) {
    result.seed_deltas[seed] = downstream[seed].area - upstream[seed].area;
    result.point += result.seed_deltas[seed];
    const auto downstream_family = rssm_family_areas(downstream[seed]);
    const auto upstream_family = rssm_family_areas(upstream[seed]);
    for (std::size_t family = 0; family < rssm_gate::kFamilyCount; ++family) {
      result.family_deltas[family] +=
          downstream_family[family] - upstream_family[family];
    }
  }
  const double seed_denominator = static_cast<double>(rssm_gate::kSeedCount);
  result.point /= seed_denominator;
  for (double &value : result.family_deltas) {
    value /= seed_denominator;
  }
  std::vector<double> replicates;
  replicates.reserve(bootstrap_rows.size());
  for (const auto &rows : bootstrap_rows) {
    double value = 0.0;
    for (std::size_t seed = 0; seed < rssm_gate::kSeedCount; ++seed) {
      value +=
          rssm_resampled_area(downstream[seed], downstream_target, rows).macro -
          rssm_resampled_area(upstream[seed], upstream_target, rows).macro;
    }
    replicates.push_back(value / seed_denominator);
  }
  const auto interval = percentile_interval(std::move(replicates));
  result.low = interval.low;
  result.high = interval.high;
  return result;
}

struct RssmPointInterval {
  double point{0.0};
  Interval interval{};
};

[[nodiscard]] RssmPointInterval
rssm_curve_interval(const RssmCurveBySeed &curves, const torch::Tensor &target,
                    const std::vector<torch::Tensor> &bootstrap_rows) {
  RssmPointInterval result{};
  for (const auto &curve : curves) {
    result.point += curve.area;
  }
  result.point /= static_cast<double>(curves.size());
  std::vector<double> replicates;
  replicates.reserve(bootstrap_rows.size());
  for (const auto &rows : bootstrap_rows) {
    double value = 0.0;
    for (const auto &curve : curves) {
      value += rssm_resampled_area(curve, target, rows).macro;
    }
    replicates.push_back(value / static_cast<double>(curves.size()));
  }
  result.interval = percentile_interval(std::move(replicates));
  return result;
}

[[nodiscard]] double
rssm_resampled_order_area(const RssmOrderCurve &curve,
                          const torch::Tensor &target,
                          const torch::Tensor &group_index) {
  if (target.dim() != 2 || target.size(1) != 1 || target.size(0) % 2 != 0 ||
      curve.points.empty()) {
    throw std::runtime_error("RSSM order bootstrap contract failed");
  }
  const int64_t groups = target.size(0) / 2;
  auto paired_target = target.reshape({groups, 2, 1});
  const auto sampled_target =
      paired_target.index_select(0, group_index).reshape({-1, 1});
  double area = 0.0;
  for (const auto &point : curve.points) {
    const auto sampled_prediction = point.prediction.reshape({groups, 2, 1})
                                        .index_select(0, group_index)
                                        .reshape({-1, 1});
    area += rssm_order_accuracy(sampled_prediction, sampled_target);
  }
  return area / static_cast<double>(curve.points.size());
}

[[nodiscard]] RssmPointInterval
rssm_order_curve_interval(const RssmOrderCurveBySeed &curves,
                          const torch::Tensor &target,
                          const std::vector<torch::Tensor> &bootstrap_rows) {
  RssmPointInterval result{};
  for (const auto &curve : curves) {
    result.point += curve.area;
  }
  result.point /= static_cast<double>(curves.size());
  std::vector<double> replicates;
  replicates.reserve(bootstrap_rows.size());
  for (const auto &rows : bootstrap_rows) {
    double value = 0.0;
    for (const auto &curve : curves) {
      value += rssm_resampled_order_area(curve, target, rows);
    }
    replicates.push_back(value / static_cast<double>(curves.size()));
  }
  result.interval = percentile_interval(std::move(replicates));
  return result;
}

[[nodiscard]] Geometry
rssm_geometry_for_channel(const torch::Tensor &features) {
  torch::NoGradGuard no_grad;
  const auto values = features.to(torch::kCPU, torch::kFloat64);
  if (values.dim() != 2 || values.size(0) < 2 || values.size(1) <= 0) {
    throw std::runtime_error("RSSM geometry features must be [n>=2,D]");
  }
  const auto centered = values - values.mean(0);
  const auto sample_gram = centered.matmul(centered.transpose(0, 1)) /
                           static_cast<double>(values.size(0) - 1);
  auto eigenvalues = at::linalg_eigvalsh(sample_gram, "L").clamp_min(0.0);
  const double total = eigenvalues.sum().item<double>();
  if (!(total > 1.0e-20)) {
    return {};
  }
  const auto probabilities = eigenvalues / total;
  const double entropy =
      -(probabilities * probabilities.clamp_min(1.0e-30).log())
           .sum()
           .item<double>();
  const double rank_cap = static_cast<double>(
      std::min<int64_t>(values.size(1), values.size(0) - 1));
  Geometry result{};
  result.effective_rank_ratio = std::exp(entropy) / rank_cap;
  result.participation_rank_ratio =
      total * total / eigenvalues.pow(2).sum().item<double>() / rank_cap;
  result.top_eigenvalue_share = eigenvalues.max().item<double>() / total;
  const auto feature_variance =
      centered.pow(2).sum(0) / static_cast<double>(values.size(0) - 1);
  const auto standard_deviation = feature_variance.clamp_min(0.0).sqrt();
  const double scale = std::sqrt(total / static_cast<double>(values.size(1)));
  result.active_dimension_fraction =
      standard_deviation.gt(std::max(1.0e-12, 1.0e-3 * scale))
          .to(torch::kFloat64)
          .mean()
          .item<double>();
  result.passed = result.effective_rank_ratio >= 0.25 &&
                  result.participation_rank_ratio >= 0.20 &&
                  result.top_eigenvalue_share <= 0.80 &&
                  result.active_dimension_fraction >= 0.75;
  validate_geometry_finite(result, "RSSM normalized geometry");
  return result;
}

[[nodiscard]] std::array<Geometry, kChannels>
rssm_geometry_by_channel(const torch::Tensor &features) {
  if (features.dim() != 3 || features.size(1) != kChannels) {
    throw std::runtime_error("RSSM geometry channel tensor mismatch");
  }
  std::array<Geometry, kChannels> result{};
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    result[static_cast<std::size_t>(channel)] =
        rssm_geometry_for_channel(features.select(1, channel));
  }
  return result;
}

struct RssmRobustness {
  PairSeparation separation{};
  Interval semantic_greater_fraction_interval{};
  bool supported{false};
};

void rssm_validate_robustness_finite(const RssmRobustness &value,
                                     const std::string &label) {
  const auto &separation = value.separation;
  if (!std::isfinite(separation.nuisance_distance) ||
      !std::isfinite(separation.semantic_distance) ||
      !std::isfinite(separation.semantic_over_nuisance_mean_ratio) ||
      !std::isfinite(separation.semantic_over_nuisance_fraction) ||
      !std::isfinite(value.semantic_greater_fraction_interval.low) ||
      !std::isfinite(value.semantic_greater_fraction_interval.high) ||
      value.semantic_greater_fraction_interval.low >
          value.semantic_greater_fraction_interval.high) {
    throw std::runtime_error(label + " robustness diagnostic is invalid");
  }
}

[[nodiscard]] RssmRobustness
rssm_robustness(const torch::Tensor &base_input,
                const torch::Tensor &nuisance_input,
                const torch::Tensor &semantic_input,
                const std::vector<torch::Tensor> &bootstrap_rows) {
  const auto unit = [](const torch::Tensor &input) {
    return input / input.norm(2, 1, true).clamp_min(1.0e-12);
  };
  const auto base_values = base_input.to(torch::kCPU, torch::kFloat64);
  const auto center = base_values.mean(0, true);
  const auto base = unit(base_values - center);
  const auto nuisance =
      unit(nuisance_input.to(torch::kCPU, torch::kFloat64) - center);
  const auto semantic =
      unit(semantic_input.to(torch::kCPU, torch::kFloat64) - center);
  const auto nuisance_distance = 1.0 - (base * nuisance).sum(1);
  const auto semantic_distance = 1.0 - (base * semantic).sum(1);
  const auto indicator =
      semantic_distance.gt(nuisance_distance).to(torch::kFloat64);
  std::vector<double> replicates;
  replicates.reserve(bootstrap_rows.size());
  for (const auto &rows : bootstrap_rows) {
    replicates.push_back(indicator.index_select(0, rows).mean().item<double>());
  }
  RssmRobustness result{};
  result.separation =
      pair_separation(base_input, nuisance_input, semantic_input);
  result.semantic_greater_fraction_interval =
      percentile_interval(std::move(replicates));
  result.supported = result.semantic_greater_fraction_interval.low >= 0.75;
  return result;
}

struct RssmRidgeEquivalence {
  double d_less_n_prediction_max_abs{0.0};
  double d_greater_n_prediction_max_abs{0.0};
  bool d_less_n_selected_alphas_exact{false};
  bool d_greater_n_selected_alphas_exact{false};
  bool finite{false};
  bool pass{false};
};

struct RssmAlphaParity {
  bool selected_alphas_exact{false};
  bool finite{false};
};

[[nodiscard]] RssmAlphaParity rssm_alpha_parity_fixture(int64_t train_rows,
                                                        int64_t validation_rows,
                                                        int64_t dimensions) {
  constexpr int64_t tasks = 3;
  const auto make_features = [dimensions](int64_t rows, int64_t offset) {
    auto values = torch::empty({rows, dimensions}, torch::kFloat64);
    auto access = values.accessor<double, 2>();
    for (int64_t row = 0; row < rows; ++row) {
      for (int64_t column = 0; column < dimensions; ++column) {
        access[row][column] =
            std::sin(0.019 *
                     static_cast<double>((row + offset + 1) * (column + 3))) +
            0.004 * static_cast<double>(row - column);
      }
    }
    return values;
  };
  auto train_features = make_features(train_rows, 0);
  auto validation_features = make_features(validation_rows, 101);
  auto train_target = torch::empty({train_rows, tasks}, torch::kFloat64);
  auto validation_target =
      torch::empty({validation_rows, tasks}, torch::kFloat64);
  const auto fill_target = [](torch::Tensor &target, int64_t offset) {
    auto access = target.accessor<double, 2>();
    for (int64_t row = 0; row < target.size(0); ++row) {
      for (int64_t task = 0; task < target.size(1); ++task) {
        access[row][task] =
            std::cos(0.13 * static_cast<double>(row + offset + task + 1)) +
            0.03 * static_cast<double>((row + offset) * (task + 1));
      }
    }
  };
  fill_target(train_target, 0);
  fill_target(validation_target, 101);
  std::array<std::size_t, tasks> primal_best{};
  std::array<std::size_t, tasks> dual_best{};
  std::array<double, tasks> primal_mse{};
  std::array<double, tasks> dual_mse{};
  primal_mse.fill(std::numeric_limits<double>::infinity());
  dual_mse.fill(std::numeric_limits<double>::infinity());
  bool finite = true;
  for (std::size_t alpha = 0; alpha < kRidgeGrid.size(); ++alpha) {
    const auto primal_prediction =
        predict(fit_ridge(train_features, train_target, kRidgeGrid[alpha]),
                validation_features);
    const auto dual_prediction =
        predict(fit_ridge_dual(train_features, train_target, kRidgeGrid[alpha]),
                validation_features);
    finite = finite && torch::isfinite(primal_prediction).all().item<bool>() &&
             torch::isfinite(dual_prediction).all().item<bool>();
    for (int64_t task = 0; task < tasks; ++task) {
      const double current_primal = (primal_prediction.select(1, task) -
                                     validation_target.select(1, task))
                                        .pow(2)
                                        .mean()
                                        .item<double>();
      const double current_dual =
          (dual_prediction.select(1, task) - validation_target.select(1, task))
              .pow(2)
              .mean()
              .item<double>();
      finite = finite && std::isfinite(current_primal) &&
               std::isfinite(current_dual);
      if (current_primal < primal_mse[static_cast<std::size_t>(task)]) {
        primal_mse[static_cast<std::size_t>(task)] = current_primal;
        primal_best[static_cast<std::size_t>(task)] = alpha;
      }
      if (current_dual < dual_mse[static_cast<std::size_t>(task)]) {
        dual_mse[static_cast<std::size_t>(task)] = current_dual;
        dual_best[static_cast<std::size_t>(task)] = alpha;
      }
    }
  }
  return {.selected_alphas_exact = primal_best == dual_best, .finite = finite};
}

[[nodiscard]] RssmRidgeEquivalence rssm_ridge_equivalence_fixture() {
  RssmRidgeEquivalence result{};
  result.d_less_n_prediction_max_abs =
      rssm_primal_dual_max_abs_difference(48, 9, 0.1);
  result.d_greater_n_prediction_max_abs =
      rssm_primal_dual_max_abs_difference(17, 41, 0.1);
  const auto d_less_n = rssm_alpha_parity_fixture(48, 23, 9);
  const auto d_greater_n = rssm_alpha_parity_fixture(40, 23, 67);
  result.d_less_n_selected_alphas_exact = d_less_n.selected_alphas_exact;
  result.d_greater_n_selected_alphas_exact = d_greater_n.selected_alphas_exact;
  result.finite = d_less_n.finite && d_greater_n.finite &&
                  std::isfinite(result.d_less_n_prediction_max_abs) &&
                  std::isfinite(result.d_greater_n_prediction_max_abs);
  result.pass = result.d_less_n_prediction_max_abs <= 1.0e-9 &&
                result.d_greater_n_prediction_max_abs <= 1.0e-9 &&
                result.d_less_n_selected_alphas_exact &&
                result.d_greater_n_selected_alphas_exact && result.finite;
  return result;
}

int run_rssm_preflight(const Options &options) {
  if (options.device != "cuda" || !torch::cuda::is_available()) {
    throw std::runtime_error("RSSM preflight requires CUDA");
  }
  if (options.steps > 0 || options.seeds > 0 || !options.weak_views) {
    throw std::runtime_error(
        "RSSM preflight accepts no training-step/seed override");
  }
  const torch::Device device(torch::kCUDA, 0);
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);

  auto normalizer_rows = generate_dataset(4100000, 32);
  auto capture_rows = generate_dataset(4200000, 101, 1, false);
  const auto normalization = fit_normalization(normalizer_rows);
  normalize(normalizer_rows, normalization);
  normalize(capture_rows, normalization);
  validate_dataset(normalizer_rows);
  validate_dataset(capture_rows);

  const auto raw_projection = make_raw_equal_width_projection();
  const auto token_projection = rssm_make_token_projection();
  const auto tokenizer_plan = rssm_tokenizer_plan_receipt();
  const auto fit_permutation =
      rssm_sattolo_permutation(256, kRssmShuffleTrainTag);
  const auto validation_permutation =
      rssm_sattolo_permutation(128, kRssmShuffleValidationTag);
  const auto test_permutation =
      rssm_sattolo_permutation(256, kRssmShuffleTestTag);
  const auto order_fit_permutations = rssm_order_fit_permutations();
  const auto order_fit_shuffled_targets =
      rssm_order_fit_targets(&order_fit_permutations);
  const auto order_validation_permutation =
      rssm_sattolo_permutation(256, kRssmOrderShuffleValidationTag);
  const auto order_test_permutation =
      rssm_sattolo_permutation(512, kRssmOrderShuffleTestTag);
  const auto bootstrap_rows = rssm_bootstrap_rows(256);
  const bool bootstrap_valid = rssm_bootstrap_contract(bootstrap_rows, 256);
  bool permutation_contracts =
      rssm_permutation_receipt(fit_permutation).pass &&
      rssm_permutation_receipt(validation_permutation).pass &&
      rssm_permutation_receipt(test_permutation).pass &&
      rssm_permutation_receipt(order_validation_permutation).pass &&
      rssm_permutation_receipt(order_test_permutation).pass;
  for (const auto &permutation : order_fit_permutations) {
    permutation_contracts =
        permutation_contracts && rssm_permutation_receipt(permutation).pass;
  }
  bool order_shuffle_balanced =
      rssm_order_target_balanced(rssm_order_labels(128).index_select(
          0, order_validation_permutation)) &&
      rssm_order_target_balanced(
          rssm_order_labels(256).index_select(0, order_test_permutation));
  for (const auto &target : order_fit_shuffled_targets) {
    order_shuffle_balanced =
        order_shuffle_balanced && rssm_order_target_balanced(target);
  }

  set_paired_rng(1701, device);
  auto model = mtf::MtfJepaMaeVicreg(
      attribution_config(device, kJmcdArms[kJmcdCombinedIndex]));
  const bool initial_mode = model->is_training();
  const auto parameter_snapshot = snapshot_parameters(model);
  const uint64_t parameter_hash_before =
      rssm_parameter_snapshot_hash(parameter_snapshot);
  const auto generator_before = current_generator_state_snapshot(device);
  const auto first = rssm_capture_once(model, capture_rows, device,
                                       /*check_direct_encoder=*/true);
  const auto second = rssm_capture_once(model, capture_rows, device,
                                        /*check_direct_encoder=*/true);
  const auto generator_after = current_generator_state_snapshot(device);
  const auto parameter_snapshot_after = snapshot_parameters(model);
  const uint64_t parameter_hash_after =
      rssm_parameter_snapshot_hash(parameter_snapshot_after);
  const bool repeated_capture_exact = rssm_capture_exact(first, second);
  const bool parameters_exact =
      parameter_max_abs_diff(model, parameter_snapshot) == 0.0 &&
      parameter_hash_before == parameter_hash_after;
  const bool generator_exact =
      generator_state_snapshot_equal(generator_before, generator_after);
  const bool mode_exact = model->is_training() == initial_mode;
  const auto raw_surface = rssm_raw_surface(capture_rows, raw_projection);
  const auto features = rssm_feature_set(raw_surface, first, token_projection);
  const bool shape_contract =
      features[rssm_index(RssmSurface::raw)].native_flat.sizes() ==
          torch::IntArrayRef({101, 810}) &&
      features[rssm_index(RssmSurface::tokenizer)].native_flat.sizes() ==
          torch::IntArrayRef({101, 2304}) &&
      features[rssm_index(RssmSurface::encoder)].native_flat.sizes() ==
          torch::IntArrayRef({101, 2304}) &&
      features[rssm_index(RssmSurface::served)].native_flat.sizes() ==
          torch::IntArrayRef({101, 96});
  const auto ridge = rssm_ridge_equivalence_fixture();
  const bool cublas_workspace = [] {
    const char *value = std::getenv("CUBLAS_WORKSPACE_CONFIG");
    return value != nullptr && std::string(value) == ":4096:8";
  }();
  const bool mechanics =
      first.public_sandwich_exact && second.public_sandwich_exact &&
      first.direct_encoder_exact && second.direct_encoder_exact &&
      first.production_order_exact && second.production_order_exact &&
      first.cardinality_exact && second.cardinality_exact &&
      repeated_capture_exact && parameters_exact && generator_exact &&
      mode_exact && shape_contract && ridge.pass && cublas_workspace &&
      tokenizer_plan.pass && permutation_contracts && order_shuffle_balanced &&
      bootstrap_valid && rssm_projection_error(raw_projection) <= 1.0e-10 &&
      rssm_projection_error(token_projection) <= 1.0e-10;

  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.rssm_preflight.v1\n";
  std::cout << "rssm.preflight.scientific_rows_used=false\n";
  std::cout << "rssm.preflight.optimizer_constructed=false\n";
  std::cout << "rssm.preflight.optimizer_steps=0\n";
  std::cout << "rssm.preflight.backward_calls=0\n";
  std::cout << "rssm.preflight.scientific_probe_fits=0\n";
  std::cout << "rssm.preflight.mechanical_ridge_fixture=true\n";
  std::cout << "rssm.preflight.cublas_workspace_exact=" << cublas_workspace
            << '\n';
  std::cout << "rssm.preflight.public_sandwich_exact="
            << first.public_sandwich_exact << '\n';
  std::cout << "rssm.preflight.direct_encoder_exact="
            << first.direct_encoder_exact << '\n';
  std::cout << "rssm.preflight.production_order_exact="
            << first.production_order_exact << '\n';
  std::cout << "rssm.preflight.token_cardinality_exact="
            << first.cardinality_exact << '\n';
  std::cout << "rssm.preflight.tokenizer_plan.total_tokens="
            << tokenizer_plan.total_tokens << '\n';
  std::cout << "rssm.preflight.tokenizer_plan.clipped_collisions="
            << tokenizer_plan.clipped_full_history_collisions << '\n';
  std::cout << "rssm.preflight.tokenizer_plan.shorter_tokens="
            << tokenizer_plan.shorter_window_tokens << '\n';
  std::cout << "rssm.preflight.tokenizer_plan.shorter_tokens_changed="
            << tokenizer_plan.shorter_tokens_changed << '\n';
  std::cout << "rssm.preflight.tokenizer_plan.pass=" << tokenizer_plan.pass
            << '\n';
  std::cout << "rssm.preflight.repeated_capture_exact="
            << repeated_capture_exact << '\n';
  std::cout << "rssm.preflight.parameters_exact=" << parameters_exact << '\n';
  emit_fingerprint("rssm.preflight.parameter_hash_before",
                   parameter_hash_before);
  emit_fingerprint("rssm.preflight.parameter_hash_after", parameter_hash_after);
  std::cout << "rssm.preflight.generator_state_exact=" << generator_exact
            << '\n';
  emit_fingerprint("rssm.preflight.cpu_generator_hash_before",
                   generator_before.digest.cpu);
  emit_fingerprint("rssm.preflight.cpu_generator_hash_after",
                   generator_after.digest.cpu);
  emit_fingerprint("rssm.preflight.cuda_generator_hash_before",
                   generator_before.digest.cuda);
  emit_fingerprint("rssm.preflight.cuda_generator_hash_after",
                   generator_after.digest.cuda);
  std::cout << "rssm.preflight.model_mode_exact=" << mode_exact << '\n';
  std::cout << "rssm.preflight.shape_contract=" << shape_contract << '\n';
  std::cout << "rssm.preflight.raw_projection_orthogonality_error="
            << rssm_projection_error(raw_projection) << '\n';
  std::cout << "rssm.preflight.token_projection_orthogonality_error="
            << rssm_projection_error(token_projection) << '\n';
  emit_fingerprint("rssm.preflight.raw_projection_hash",
                   hash_tensor_stable_bytes(raw_projection));
  emit_fingerprint("rssm.preflight.token_projection_hash",
                   hash_tensor_stable_bytes(token_projection));
  rssm_emit_permutation("rssm.preflight.shuffle_fit", fit_permutation);
  rssm_emit_permutation("rssm.preflight.shuffle_validation",
                        validation_permutation);
  rssm_emit_permutation("rssm.preflight.shuffle_test", test_permutation);
  for (std::size_t index = 0; index < kRssmSampleLadder.size(); ++index) {
    rssm_emit_permutation("rssm.preflight.order_shuffle_fit.n_" +
                              std::to_string(kRssmSampleLadder[index]),
                          order_fit_permutations[index]);
  }
  rssm_emit_permutation("rssm.preflight.order_shuffle_validation",
                        order_validation_permutation);
  rssm_emit_permutation("rssm.preflight.order_shuffle_test",
                        order_test_permutation);
  std::cout << "rssm.preflight.permutation_contracts=" << permutation_contracts
            << '\n';
  std::cout << "rssm.preflight.order_shuffle_balanced="
            << order_shuffle_balanced << '\n';
  emit_fingerprint("rssm.preflight.bootstrap_table_hash",
                   rssm_tensor_vector_hash(bootstrap_rows));
  std::cout << "rssm.preflight.bootstrap_contract=" << bootstrap_valid << '\n';
  std::cout << "rssm.preflight.ridge.d_less_n_prediction_max_abs="
            << ridge.d_less_n_prediction_max_abs << '\n';
  std::cout << "rssm.preflight.ridge.d_greater_n_prediction_max_abs="
            << ridge.d_greater_n_prediction_max_abs << '\n';
  std::cout << "rssm.preflight.ridge.d_less_n_selected_alphas_exact="
            << ridge.d_less_n_selected_alphas_exact << '\n';
  std::cout << "rssm.preflight.ridge.d_greater_n_selected_alphas_exact="
            << ridge.d_greater_n_selected_alphas_exact << '\n';
  std::cout << "rssm.preflight.ridge.finite=" << ridge.finite << '\n';
  std::cout << "rssm.preflight.ridge.pass=" << ridge.pass << '\n';
  emit_fingerprint("rssm.preflight.tokenizer_source_hash",
                   first.tokenizer_source_hash);
  emit_fingerprint("rssm.preflight.encoder_source_hash",
                   first.encoder_source_hash);
  emit_fingerprint("rssm.preflight.served_source_hash",
                   first.served_source_hash);
  emit_fingerprint("rssm.preflight.tokenizer_float64_hash",
                   first.tokenizer_float64_hash);
  emit_fingerprint("rssm.preflight.encoder_float64_hash",
                   first.encoder_float64_hash);
  emit_fingerprint("rssm.preflight.served_float64_hash",
                   first.served_float64_hash);
  emit_fingerprint("rssm.preflight.token_mask_structure_hash",
                   first.token_mask_structure_hash);
  emit_fingerprint("rssm.preflight.metadata_structure_hash",
                   first.metadata_structure_hash);
  std::cout
      << "rssm.preflight.authoritative_command="
         "CUBLAS_WORKSPACE_CONFIG=:4096:8 .build/tests/quality_wikimyei_mtf_"
         "jepa_mae_vicreg_representation --experiment representation-surface-"
         "sufficiency-map --device cuda\n";
  std::cout << "rssm.preflight.pass=" << mechanics << '\n';
  std::cout << "training_authorized=false\n";
  std::cout << "long_run_authorized=false\n";
  std::cout << "production_or_end_to_end_authorized=false\n";
  std::cout << "follow_on_repair_authorized=false\n";
  return mechanics ? 0 : 3;
}

[[nodiscard]] Dataset rssm_reversed_dataset(const Dataset &source) {
  return {.data = source.data.flip({2}).contiguous(),
          .mask = source.mask.flip({2}).contiguous(),
          .target = source.target,
          .group_begin = source.group_begin};
}

[[nodiscard]] torch::Tensor rssm_group_ids(const Dataset &dataset) {
  return torch::arange(
      dataset.group_begin, dataset.group_begin + dataset.data.size(0),
      torch::TensorOptions().dtype(torch::kInt64).device(torch::kCPU));
}

void rssm_emit_dataset_identity(const std::string &prefix,
                                const Dataset &dataset,
                                const std::string &data_state) {
  const std::string item = prefix + "." + data_state;
  emit_fingerprint(item + ".group_ids_hash",
                   hash_tensor_stable_bytes(rssm_group_ids(dataset)));
  emit_fingerprint(item + ".data_hash", hash_tensor_stable_bytes(dataset.data));
  emit_fingerprint(item + ".mask_hash", hash_tensor_stable_bytes(dataset.mask));
  emit_fingerprint(item + ".target_hash",
                   hash_tensor_stable_bytes(dataset.target));
  std::cout << item << ".group_begin=" << dataset.group_begin << '\n';
  std::cout << item << ".groups=" << dataset.data.size(0) << '\n';
}

[[nodiscard]] bool rssm_group_pair_exact(const Dataset &left,
                                         const Dataset &right) {
  return left.group_begin == right.group_begin &&
         left.data.size(0) == right.data.size(0) &&
         rssm_tensor_bytes_equal(rssm_group_ids(left), rssm_group_ids(right));
}

[[nodiscard]] RssmSurfaceFeatures
rssm_raw_surface(const Dataset &dataset, const torch::Tensor &raw_projection) {
  RssmSurfaceFeatures result{};
  result.native_by_channel =
      dataset.data.to(torch::kCPU, torch::kFloat64)
          .reshape({dataset.data.size(0), kChannels, kRawChannelWidth})
          .contiguous();
  result.native_flat =
      result.native_by_channel.reshape({dataset.data.size(0), -1}).contiguous();
  result.fixed96_by_channel =
      rssm_project_by_channel(result.native_by_channel, raw_projection);
  result.fixed96_flat =
      result.fixed96_by_channel.reshape({dataset.data.size(0), -1})
          .contiguous();
  return result;
}

[[nodiscard]] const torch::Tensor &rssm_flat(const RssmFeatureSet &features,
                                             RssmSurface surface,
                                             RssmTrack track) {
  const auto &selected = features[rssm_index(surface)];
  return track == RssmTrack::native ? selected.native_flat
                                    : selected.fixed96_flat;
}

[[nodiscard]] const torch::Tensor &
rssm_by_channel(const RssmFeatureSet &features, RssmSurface surface,
                RssmTrack track) {
  const auto &selected = features[rssm_index(surface)];
  return track == RssmTrack::native ? selected.native_by_channel
                                    : selected.fixed96_by_channel;
}

using RssmProbeGrid =
    std::array<std::array<ProbeCurve, kRssmTrackCount>, kRssmSurfaceCount>;
using RssmOrderGrid =
    std::array<std::array<RssmOrderCurve, kRssmTrackCount>, kRssmSurfaceCount>;
using RssmGeometryGrid =
    std::array<std::array<std::array<Geometry, kChannels>, kRssmTrackCount>,
               kRssmSurfaceCount>;
using RssmRobustnessGrid =
    std::array<std::array<RssmRobustness, kRssmTrackCount>, kRssmSurfaceCount>;

struct RssmSeedMeasurement {
  int64_t seed{0};
  RssmProbeGrid real{};
  RssmProbeGrid shuffled{};
  RssmOrderGrid order{};
  RssmOrderGrid order_shuffled{};
  RssmGeometryGrid geometry{};
  RssmRobustnessGrid robustness{};
  std::array<Geometry, kChannels> accepted_served_geometry{};
  bool public_sandwich_exact{true};
  bool repeated_capture_exact{true};
  bool parameters_and_rng_unchanged{true};
  bool production_order_exact{true};
  bool cardinality_exact{true};
};

[[nodiscard]] bool rssm_native_uses_dual(RssmSurface surface, RssmTrack track) {
  return track == RssmTrack::native && surface != RssmSurface::served;
}

[[nodiscard]] RssmCurveBySeed rssm_curves_for(
    const std::array<RssmSeedMeasurement, rssm_gate::kSeedCount> &measurements,
    RssmSurface surface, RssmTrack track, bool shuffled) {
  RssmCurveBySeed result{};
  for (std::size_t seed = 0; seed < result.size(); ++seed) {
    const auto &grid =
        shuffled ? measurements[seed].shuffled : measurements[seed].real;
    result[seed] = grid[rssm_index(surface)][rssm_index(track)];
  }
  return result;
}

[[nodiscard]] RssmOrderCurveBySeed rssm_order_curves_for(
    const std::array<RssmSeedMeasurement, rssm_gate::kSeedCount> &measurements,
    RssmSurface surface, RssmTrack track, bool shuffled) {
  RssmOrderCurveBySeed result{};
  for (std::size_t seed = 0; seed < result.size(); ++seed) {
    const auto &grid =
        shuffled ? measurements[seed].order_shuffled : measurements[seed].order;
    result[seed] = grid[rssm_index(surface)][rssm_index(track)];
  }
  return result;
}

void rssm_emit_probe_curve(const std::string &prefix, const ProbeCurve &curve) {
  std::cout << prefix << ".area=" << curve.area << '\n';
  const auto family_area = rssm_family_areas(curve);
  for (std::size_t family = 0; family < family_area.size(); ++family) {
    std::cout << prefix << ".family_" << kFamilyNames[family]
              << "_aulc=" << family_area[family] << '\n';
  }
  for (std::size_t point = 0; point < curve.points.size(); ++point) {
    const auto &value = curve.points[point];
    const std::string point_prefix =
        prefix + ".n_" + std::to_string(value.samples);
    std::cout << point_prefix << ".macro_r2=" << value.score.macro << '\n';
    for (std::size_t family = 0; family < kFamilies; ++family) {
      std::cout << point_prefix << ".family_" << kFamilyNames[family]
                << "_r2=" << value.score.family[family] << '\n';
    }
    for (std::size_t target = 0; target < kTargets; ++target) {
      std::cout << point_prefix << ".target_" << target
                << ".alpha=" << value.selected_alpha[target] << '\n';
    }
  }
}

void rssm_emit_order_curve(const std::string &prefix,
                           const RssmOrderCurve &curve) {
  std::cout << prefix << ".accuracy_aulc=" << curve.area << '\n';
  for (const auto &point : curve.points) {
    const std::string point_prefix =
        prefix + ".n_" + std::to_string(point.samples);
    std::cout << point_prefix << ".accuracy=" << point.accuracy << '\n';
    std::cout << point_prefix << ".alpha=" << point.selected_alpha << '\n';
  }
}

void rssm_emit_transition(const std::string &prefix,
                          const rssm_gate::TransitionInput &input) {
  const auto result = rssm_gate::evaluate_transition(input);
  std::cout << prefix << ".point=" << input.point << '\n';
  std::cout << prefix << ".bootstrap_95_low=" << input.low << '\n';
  std::cout << prefix << ".bootstrap_95_high=" << input.high << '\n';
  for (std::size_t seed = 0; seed < input.seed_deltas.size(); ++seed) {
    std::cout << prefix << ".seed_" << kAttributionSeeds[seed]
              << "_delta=" << input.seed_deltas[seed] << '\n';
  }
  for (std::size_t family = 0; family < input.family_deltas.size(); ++family) {
    std::cout << prefix << ".family_" << kFamilyNames[family]
              << "_delta=" << input.family_deltas[family] << '\n';
  }
  std::cout << prefix << ".negative_seed_count=" << result.negative_seed_count
            << '\n';
  std::cout << prefix
            << ".negative_family_count=" << result.negative_family_count
            << '\n';
  std::cout << prefix << ".classification="
            << rssm_gate::transition_classification_name(result.classification)
            << '\n';
}

void rssm_emit_geometry_grid(const std::string &prefix,
                             const RssmGeometryGrid &grid) {
  for (std::size_t surface = 0; surface < kRssmSurfaceCount; ++surface) {
    for (std::size_t track = 0; track < kRssmTrackCount; ++track) {
      emit_geometry(prefix + ".surface." + kRssmSurfaceNames[surface] + "." +
                        kRssmTrackNames[track] + ".geometry",
                    grid[surface][track], /*emit_passed=*/false);
    }
  }
}

void rssm_emit_robustness_grid(const std::string &prefix,
                               const RssmRobustnessGrid &grid) {
  for (std::size_t surface = 0; surface < kRssmSurfaceCount; ++surface) {
    for (std::size_t track = 0; track < kRssmTrackCount; ++track) {
      const auto &value = grid[surface][track];
      const std::string item = prefix + ".surface." +
                               kRssmSurfaceNames[surface] + "." +
                               kRssmTrackNames[track] + ".robustness";
      std::cout << item
                << ".nuisance_distance=" << value.separation.nuisance_distance
                << '\n';
      std::cout << item
                << ".semantic_distance=" << value.separation.semantic_distance
                << '\n';
      std::cout << item << ".semantic_over_nuisance_mean_ratio="
                << value.separation.semantic_over_nuisance_mean_ratio << '\n';
      std::cout << item << ".semantic_greater_fraction="
                << value.separation.semantic_over_nuisance_fraction << '\n';
      std::cout << item << ".bootstrap_95_low="
                << value.semantic_greater_fraction_interval.low << '\n';
      std::cout << item << ".bootstrap_95_high="
                << value.semantic_greater_fraction_interval.high << '\n';
      std::cout << item << ".supported=" << value.supported << '\n';
    }
  }
}

[[nodiscard]] bool rssm_exact(double observed, double expected) {
  return observed == expected;
}

[[nodiscard]] bool rssm_legacy_raw_reference_exact(const ProbeCurve &curve) {
  if (curve.points.size() != 4) {
    return false;
  }
  const auto &final = curve.points.back().score;
  return rssm_exact(curve.area, 0.60228658165276872) &&
         rssm_exact(final.macro, 0.71347572477392507) &&
         rssm_exact(final.family[0], 0.67227470335906514) &&
         rssm_exact(final.family[1], 0.52514829310154743) &&
         rssm_exact(final.family[2], 0.67986003180423782) &&
         rssm_exact(final.family[3], 0.97661987083084989);
}

void rssm_emit_legacy_raw_reference(const ProbeCurve &curve) {
  const auto &final = curve.points.back().score;
  std::cout << "control.raw_equal_width.area=" << curve.area << '\n';
  std::cout << "control.raw_equal_width.final_macro_r2=" << final.macro << '\n';
  for (std::size_t family = 0; family < kFamilies; ++family) {
    std::cout << "control.raw_equal_width.family_" << kFamilyNames[family]
              << "_r2=" << final.family[family] << '\n';
  }
}

void rssm_emit_accepted_seed_reference(const RssmSeedMeasurement &measurement) {
  const auto &curve = measurement.real[rssm_index(RssmSurface::served)]
                                      [rssm_index(RssmTrack::fixed96)];
  const auto &final = curve.points.back().score;
  const std::string prefix =
      "seed_" + std::to_string(measurement.seed) + ".arm.jepa_mae_only.step_0";
  std::cout << prefix << ".probe.area=" << curve.area << '\n';
  std::cout << prefix << ".probe.final_macro_r2=" << final.macro << '\n';
  for (std::size_t family = 0; family < kFamilies; ++family) {
    std::cout << prefix << ".probe.family_" << kFamilyNames[family]
              << "_r2=" << final.family[family] << '\n';
  }
  emit_geometry(prefix + ".geometry", measurement.accepted_served_geometry);
}

[[nodiscard]] bool rssm_emit_accepted_summary(
    const std::array<RssmSeedMeasurement, rssm_gate::kSeedCount>
        &measurements) {
  double area = 0.0;
  std::array<double, kFamilies> family{};
  double effective = 0.0;
  double participation = 0.0;
  double top = 0.0;
  double active = 0.0;
  constexpr std::array<double, rssm_gate::kSeedCount> expected_seed_area{
      0.51029806802386968, 0.5121433689059538, 0.53534605970626181};
  bool exact = true;
  for (std::size_t seed = 0; seed < measurements.size(); ++seed) {
    const auto &curve = measurements[seed].real[rssm_index(RssmSurface::served)]
                                               [rssm_index(RssmTrack::fixed96)];
    exact = exact && curve.area == expected_seed_area[seed];
    area += curve.area;
    const auto &final = curve.points.back().score;
    for (std::size_t index = 0; index < family.size(); ++index) {
      family[index] += final.family[index];
    }
    double seed_effective = 0.0;
    double seed_participation = 0.0;
    double seed_top = 0.0;
    double seed_active = 1.0;
    for (const auto &channel : measurements[seed].accepted_served_geometry) {
      seed_effective += channel.effective_rank_ratio;
      seed_participation += channel.participation_rank_ratio;
      seed_top = std::max(seed_top, channel.top_eigenvalue_share);
      seed_active = std::min(seed_active, channel.active_dimension_fraction);
    }
    effective += seed_effective / static_cast<double>(kChannels);
    participation += seed_participation / static_cast<double>(kChannels);
    top += seed_top;
    active += seed_active;
  }
  const double denominator = static_cast<double>(measurements.size());
  area /= denominator;
  effective /= denominator;
  participation /= denominator;
  top /= denominator;
  active /= denominator;
  for (double &value : family) {
    value /= denominator;
  }
  const std::string prefix = "summary.arm.jepa_mae_only.step_0";
  std::cout << prefix << ".probe_area_fixed_seed_mean=" << area << '\n';
  for (std::size_t index = 0; index < family.size(); ++index) {
    std::cout << prefix << ".family_" << kFamilyNames[index]
              << "_r2_fixed_seed_mean=" << family[index] << '\n';
  }
  std::cout << prefix
            << ".geometry.channel_mean_effective_rank_ratio_fixed_seed_mean="
            << effective << '\n';
  std::cout
      << prefix
      << ".geometry.channel_mean_participation_rank_ratio_fixed_seed_mean="
      << participation << '\n';
  std::cout << prefix
            << ".geometry.channel_max_top_eigenvalue_share_fixed_seed_mean="
            << top << '\n';
  std::cout
      << prefix
      << ".geometry.channel_min_active_dimension_fraction_fixed_seed_mean="
      << active << '\n';
  exact =
      exact && area == 0.51926249887869513 &&
      family[0] == 0.59617515541946775 && family[1] == 0.55399559859135994 &&
      family[2] == 0.61208013000568784 && family[3] == 0.82426389251311549 &&
      effective == 0.12086530975211111 &&
      participation == 0.083191131199457799 && top == 0.65366167667477137 &&
      active == 1.0;
  return exact;
}

int run_rssm(const Options &options) {
  if (options.device != "cuda" || !torch::cuda::is_available()) {
    throw std::runtime_error("RSSM authoritative map requires CUDA");
  }
  if (options.steps > 0 || options.seeds > 0 || !options.weak_views) {
    throw std::runtime_error(
        "RSSM accepts no training-step/seed/weak-view override");
  }
  const char *workspace = std::getenv("CUBLAS_WORKSPACE_CONFIG");
  if (workspace == nullptr || std::string(workspace) != ":4096:8") {
    throw std::runtime_error("RSSM requires CUBLAS_WORKSPACE_CONFIG=:4096:8");
  }
  const torch::Device device(torch::kCUDA, 0);
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.rssm.v1\n";
  std::cout << "module_only=true\n";
  std::cout << "device=cuda:0\n";
  std::cout << "model_seeds=17,31,47\n";
  std::cout << "surfaces=raw,tokenizer,encoder,served\n";
  std::cout << "tracks=native,fixed96\n";
  std::cout << "optimizer_constructed=false\n";
  std::cout << "optimizer_steps=0\n";
  std::cout << "backward_calls=0\n";
  std::cout << "launcher_augmentation=false\n";
  std::cout << "model_row_batch_size=" << kModelRowBatchSize << '\n';
  std::cout << "probe_sample_ladder=32,64,128,256\n";
  std::cout << "bootstrap_replicates=" << kRssmBootstrapReplicates << '\n';
  std::cout << "bootstrap_seed=" << kRssmBootstrapSeed << '\n';
  std::cout << "rssm.attempt.consumption_boundary="
               "first_accepted_row_probe_fit_or_endpoint\n";

  const auto tokenizer_plan = rssm_tokenizer_plan_receipt();
  std::cout << "rssm.tokenizer_plan.total_tokens="
            << tokenizer_plan.total_tokens << '\n';
  std::cout << "rssm.tokenizer_plan.clipped_collisions="
            << tokenizer_plan.clipped_full_history_collisions << '\n';
  std::cout << "rssm.tokenizer_plan.shorter_tokens="
            << tokenizer_plan.shorter_window_tokens << '\n';
  std::cout << "rssm.tokenizer_plan.shorter_tokens_changed="
            << tokenizer_plan.shorter_tokens_changed << '\n';
  std::cout << "rssm.tokenizer_plan.pass=" << tokenizer_plan.pass << '\n';

  auto ssl = generate_dataset(0, 256);
  auto probe_train = generate_dataset(1000000, 256);
  auto probe_validation = generate_dataset(2000000, 128);
  auto test = generate_dataset(3000000, 256);
  auto nuisance = generate_dataset(3000000, 256, 1, false);
  auto semantic = generate_dataset(3000000, 256, 0, true);
  const std::array<uint64_t, 6> pre_normalization_mask_hashes{
      hash_tensor_stable_bytes(ssl.mask),
      hash_tensor_stable_bytes(probe_train.mask),
      hash_tensor_stable_bytes(probe_validation.mask),
      hash_tensor_stable_bytes(test.mask),
      hash_tensor_stable_bytes(nuisance.mask),
      hash_tensor_stable_bytes(semantic.mask)};
  const std::array<uint64_t, 6> pre_normalization_target_hashes{
      hash_tensor_stable_bytes(ssl.target),
      hash_tensor_stable_bytes(probe_train.target),
      hash_tensor_stable_bytes(probe_validation.target),
      hash_tensor_stable_bytes(test.target),
      hash_tensor_stable_bytes(nuisance.target),
      hash_tensor_stable_bytes(semantic.target)};
  rssm_emit_dataset_identity("rssm.data.normalizer", ssl, "unnormalized");
  rssm_emit_dataset_identity("rssm.data.probe_train", probe_train,
                             "unnormalized");
  rssm_emit_dataset_identity("rssm.data.probe_validation", probe_validation,
                             "unnormalized");
  rssm_emit_dataset_identity("rssm.data.test", test, "unnormalized");
  rssm_emit_dataset_identity("rssm.data.nuisance", nuisance, "unnormalized");
  rssm_emit_dataset_identity("rssm.data.semantic", semantic, "unnormalized");
  const auto raw_projection = make_raw_equal_width_projection();
  const auto legacy_raw_train =
      raw_equal_width_features(probe_train, raw_projection);
  const auto legacy_raw_validation =
      raw_equal_width_features(probe_validation, raw_projection);
  const auto legacy_raw_test = raw_equal_width_features(test, raw_projection);

  const auto normalization = fit_normalization(ssl);
  for (Dataset *dataset :
       {&ssl, &probe_train, &probe_validation, &test, &nuisance, &semantic}) {
    normalize(*dataset, normalization);
    validate_dataset(*dataset);
  }
  auto reversed_train = rssm_reversed_dataset(probe_train);
  auto reversed_validation = rssm_reversed_dataset(probe_validation);
  auto reversed_test = rssm_reversed_dataset(test);
  rssm_emit_dataset_identity("rssm.data.normalizer", ssl, "normalized");
  rssm_emit_dataset_identity("rssm.data.probe_train", probe_train,
                             "normalized");
  rssm_emit_dataset_identity("rssm.data.probe_validation", probe_validation,
                             "normalized");
  rssm_emit_dataset_identity("rssm.data.test", test, "normalized");
  rssm_emit_dataset_identity("rssm.data.nuisance", nuisance, "normalized");
  rssm_emit_dataset_identity("rssm.data.semantic", semantic, "normalized");
  rssm_emit_dataset_identity("rssm.data.reversed_train", reversed_train,
                             "normalized");
  rssm_emit_dataset_identity("rssm.data.reversed_validation",
                             reversed_validation, "normalized");
  rssm_emit_dataset_identity("rssm.data.reversed_test", reversed_test,
                             "normalized");
  const std::array<const Dataset *, 6> normalized_datasets{
      &ssl, &probe_train, &probe_validation, &test, &nuisance, &semantic};
  bool normalization_preserved_identity = true;
  for (std::size_t index = 0; index < normalized_datasets.size(); ++index) {
    normalization_preserved_identity =
        normalization_preserved_identity &&
        pre_normalization_mask_hashes[index] ==
            hash_tensor_stable_bytes(normalized_datasets[index]->mask) &&
        pre_normalization_target_hashes[index] ==
            hash_tensor_stable_bytes(normalized_datasets[index]->target);
  }
  const bool dataset_identity_exact =
      normalization_preserved_identity && ssl.group_begin == 0 &&
      ssl.data.size(0) == 256 && probe_train.group_begin == 1000000 &&
      probe_train.data.size(0) == 256 &&
      probe_validation.group_begin == 2000000 &&
      probe_validation.data.size(0) == 128 && test.group_begin == 3000000 &&
      test.data.size(0) == 256 && rssm_group_pair_exact(test, nuisance) &&
      rssm_group_pair_exact(test, semantic) &&
      rssm_group_pair_exact(probe_train, reversed_train) &&
      rssm_group_pair_exact(probe_validation, reversed_validation) &&
      rssm_group_pair_exact(test, reversed_test);
  emit_fingerprint("rssm.data.counterfactual_pair_group_ids_hash",
                   hash_tensor_stable_bytes(rssm_group_ids(test)));
  std::cout << "rssm.data.normalization_preserved_identity="
            << normalization_preserved_identity << '\n';
  std::cout << "rssm.data.identity_exact=" << dataset_identity_exact << '\n';

  const auto token_projection = rssm_make_token_projection();
  const double raw_projection_error = rssm_projection_error(raw_projection);
  const double token_projection_error = rssm_projection_error(token_projection);
  const bool projections_valid =
      raw_projection_error <= 1.0e-10 && token_projection_error <= 1.0e-10;
  emit_fingerprint("rssm.projection.raw.hash",
                   hash_tensor_stable_bytes(raw_projection));
  emit_fingerprint("rssm.projection.token_encoder_shared.hash",
                   hash_tensor_stable_bytes(token_projection));
  std::cout << "rssm.projection.raw.orthogonality_error="
            << raw_projection_error << '\n';
  std::cout << "rssm.projection.token_encoder_shared.orthogonality_error="
            << token_projection_error << '\n';

  const auto shuffle_train =
      rssm_sattolo_permutation(256, kRssmShuffleTrainTag);
  const auto shuffle_validation =
      rssm_sattolo_permutation(128, kRssmShuffleValidationTag);
  const auto shuffle_test = rssm_sattolo_permutation(256, kRssmShuffleTestTag);
  const auto shuffled_train_target =
      probe_train.target.index_select(0, shuffle_train).contiguous();
  const auto shuffled_validation_target =
      probe_validation.target.index_select(0, shuffle_validation).contiguous();
  const auto shuffled_test_target =
      test.target.index_select(0, shuffle_test).contiguous();

  const auto order_fit_permutations = rssm_order_fit_permutations();
  const auto order_fit_targets = rssm_order_fit_targets(nullptr);
  const auto shuffled_order_fit_targets =
      rssm_order_fit_targets(&order_fit_permutations);
  const auto order_validation_target = rssm_order_labels(128);
  const auto order_test_target = rssm_order_labels(256);
  const auto order_shuffle_validation =
      rssm_sattolo_permutation(256, kRssmOrderShuffleValidationTag);
  const auto order_shuffle_test =
      rssm_sattolo_permutation(512, kRssmOrderShuffleTestTag);
  const auto shuffled_order_validation_target =
      order_validation_target.index_select(0, order_shuffle_validation)
          .contiguous();
  const auto shuffled_order_test_target =
      order_test_target.index_select(0, order_shuffle_test).contiguous();
  bool order_shuffle_balanced =
      rssm_order_target_balanced(shuffled_order_validation_target) &&
      rssm_order_target_balanced(shuffled_order_test_target);
  for (const auto &target : shuffled_order_fit_targets) {
    order_shuffle_balanced =
        order_shuffle_balanced && rssm_order_target_balanced(target);
  }

  const auto bootstrap_rows = rssm_bootstrap_rows(256);
  const bool bootstrap_valid = rssm_bootstrap_contract(bootstrap_rows, 256);
  const auto ridge_equivalence = rssm_ridge_equivalence_fixture();
  bool permutation_contracts =
      rssm_permutation_receipt(shuffle_train).pass &&
      rssm_permutation_receipt(shuffle_validation).pass &&
      rssm_permutation_receipt(shuffle_test).pass &&
      rssm_permutation_receipt(order_shuffle_validation).pass &&
      rssm_permutation_receipt(order_shuffle_test).pass;
  rssm_emit_permutation("rssm.shuffle.fit", shuffle_train);
  rssm_emit_permutation("rssm.shuffle.validation", shuffle_validation);
  rssm_emit_permutation("rssm.shuffle.test", shuffle_test);
  for (std::size_t index = 0; index < kRssmSampleLadder.size(); ++index) {
    const auto &permutation = order_fit_permutations[index];
    permutation_contracts =
        permutation_contracts && rssm_permutation_receipt(permutation).pass;
    rssm_emit_permutation("rssm.order_shuffle.fit.n_" +
                              std::to_string(kRssmSampleLadder[index]),
                          permutation);
  }
  rssm_emit_permutation("rssm.order_shuffle.validation",
                        order_shuffle_validation);
  rssm_emit_permutation("rssm.order_shuffle.test", order_shuffle_test);
  std::cout << "rssm.permutation_contracts=" << permutation_contracts << '\n';
  std::cout << "rssm.order_shuffle_balanced=" << order_shuffle_balanced << '\n';
  emit_fingerprint("rssm.bootstrap.table_hash",
                   rssm_tensor_vector_hash(bootstrap_rows));
  std::cout << "rssm.bootstrap.contract=" << bootstrap_valid << '\n';
  std::cout << "rssm.ridge_equivalence.pass=" << ridge_equivalence.pass << '\n';
  emit_fingerprint("rssm.normalization.mean_hash",
                   hash_tensor_stable_bytes(normalization.mean));
  emit_fingerprint("rssm.normalization.inv_std_hash",
                   hash_tensor_stable_bytes(normalization.inv_std));

  constexpr std::size_t kRssmDatasetCount = 8;
  const std::array<const Dataset *, kRssmDatasetCount> datasets{
      &probe_train,
      &probe_validation,
      &test,
      &nuisance,
      &semantic,
      &reversed_train,
      &reversed_validation,
      &reversed_test};
  constexpr std::array<const char *, kRssmDatasetCount> dataset_names{
      "probe_train",
      "probe_validation",
      "test",
      "nuisance",
      "semantic",
      "reversed_train",
      "reversed_validation",
      "reversed_test"};
  constexpr std::size_t kTrainDataset = 0;
  constexpr std::size_t kValidationDataset = 1;
  constexpr std::size_t kTestDataset = 2;
  constexpr std::size_t kNuisanceDataset = 3;
  constexpr std::size_t kSemanticDataset = 4;
  constexpr std::size_t kReversedTrainDataset = 5;
  constexpr std::size_t kReversedValidationDataset = 6;
  constexpr std::size_t kReversedTestDataset = 7;

  std::array<RssmSurfaceFeatures, kRssmDatasetCount> raw_surfaces{};
  for (std::size_t dataset_index = 0; dataset_index < datasets.size();
       ++dataset_index) {
    raw_surfaces[dataset_index] =
        rssm_raw_surface(*datasets[dataset_index], raw_projection);
    const std::string prefix =
        "rssm.surface.raw." + std::string(dataset_names[dataset_index]);
    emit_fingerprint(prefix + ".source_hash",
                     hash_tensor_stable_bytes(datasets[dataset_index]->data));
    emit_fingerprint(prefix + ".cpu_float64_hash",
                     hash_tensor_stable_bytes(
                         raw_surfaces[dataset_index].native_by_channel));
    emit_fingerprint(prefix + ".fixed96_float64_hash",
                     hash_tensor_stable_bytes(
                         raw_surfaces[dataset_index].fixed96_by_channel));
  }

  std::array<ProbeCurve, kRssmTrackCount> raw_real{};
  std::array<ProbeCurve, kRssmTrackCount> raw_shuffled{};
  std::array<RssmOrderCurve, kRssmTrackCount> raw_order{};
  std::array<RssmOrderCurve, kRssmTrackCount> raw_order_shuffled{};
  std::array<std::array<Geometry, kChannels>, kRssmTrackCount> raw_geometry{};
  std::array<RssmRobustness, kRssmTrackCount> raw_robustness{};

  std::array<RssmSeedMeasurement, rssm_gate::kSeedCount> measurements{};
  std::array<std::array<RssmFeatureSet, kRssmDatasetCount>,
             rssm_gate::kSeedCount>
      feature_sets_by_seed{};
  bool all_public_sandwich_exact = true;
  bool all_repeated_capture_exact = true;
  bool all_parameters_and_rng_unchanged = true;
  bool all_production_order_exact = true;
  bool all_cardinality_exact = true;
  bool cross_seed_token_structure_exact = true;
  bool all_metadata_plan_exact = true;
  bool metadata_reference_initialized = false;
  uint64_t metadata_plan_reference = 0;
  std::array<uint64_t, kRssmDatasetCount> token_mask_structure_reference{};

  for (std::size_t seed_index = 0; seed_index < measurements.size();
       ++seed_index) {
    auto &measurement = measurements[seed_index];
    measurement.seed = kAttributionSeeds[seed_index];
    set_paired_rng(measurement.seed, device);
    auto model = mtf::MtfJepaMaeVicreg(
        attribution_config(device, kJmcdArms[kJmcdCombinedIndex]));
    const bool initial_mode = model->is_training();
    const auto parameters_before = snapshot_parameters(model);
    const uint64_t parameter_hash_before =
        rssm_parameter_snapshot_hash(parameters_before);
    const auto generator_before = current_generator_state_snapshot(device);
    auto &feature_sets = feature_sets_by_seed[seed_index];
    for (std::size_t dataset_index = 0; dataset_index < datasets.size();
         ++dataset_index) {
      const auto first =
          rssm_capture_once(model, *datasets[dataset_index], device,
                            /*check_direct_encoder=*/false);
      const auto second =
          rssm_capture_once(model, *datasets[dataset_index], device,
                            /*check_direct_encoder=*/false);
      measurement.public_sandwich_exact = measurement.public_sandwich_exact &&
                                          first.public_sandwich_exact &&
                                          second.public_sandwich_exact;
      measurement.repeated_capture_exact = measurement.repeated_capture_exact &&
                                           rssm_capture_exact(first, second);
      measurement.production_order_exact = measurement.production_order_exact &&
                                           first.production_order_exact &&
                                           second.production_order_exact;
      measurement.cardinality_exact = measurement.cardinality_exact &&
                                      first.cardinality_exact &&
                                      second.cardinality_exact;
      if (seed_index == 0) {
        token_mask_structure_reference[dataset_index] =
            first.token_mask_structure_hash;
      } else {
        cross_seed_token_structure_exact =
            cross_seed_token_structure_exact &&
            token_mask_structure_reference[dataset_index] ==
                first.token_mask_structure_hash;
      }
      if (!metadata_reference_initialized) {
        metadata_plan_reference = first.metadata_structure_hash;
        metadata_reference_initialized = true;
      } else {
        all_metadata_plan_exact =
            all_metadata_plan_exact &&
            metadata_plan_reference == first.metadata_structure_hash;
      }
      feature_sets[dataset_index] = rssm_feature_set(
          raw_surfaces[dataset_index], first, token_projection);
      const std::string prefix = "rssm.seed_" +
                                 std::to_string(measurement.seed) +
                                 ".capture." + dataset_names[dataset_index];
      emit_fingerprint(prefix + ".tokenizer_hash", first.tokenizer_source_hash);
      emit_fingerprint(prefix + ".encoder_hash", first.encoder_source_hash);
      emit_fingerprint(prefix + ".served_hash", first.served_source_hash);
      emit_fingerprint(prefix + ".tokenizer_float64_hash",
                       first.tokenizer_float64_hash);
      emit_fingerprint(prefix + ".encoder_float64_hash",
                       first.encoder_float64_hash);
      emit_fingerprint(prefix + ".served_float64_hash",
                       first.served_float64_hash);
      emit_fingerprint(prefix + ".token_mask_structure_hash",
                       first.token_mask_structure_hash);
      emit_fingerprint(prefix + ".metadata_structure_hash",
                       first.metadata_structure_hash);
      for (std::size_t surface_index = rssm_index(RssmSurface::tokenizer);
           surface_index < kRssmSurfaceCount; ++surface_index) {
        const auto &surface = feature_sets[dataset_index][surface_index];
        const std::string surface_prefix =
            prefix + ".surface." + kRssmSurfaceNames[surface_index];
        emit_fingerprint(surface_prefix + ".native_float64_hash",
                         hash_tensor_stable_bytes(surface.native_by_channel));
        emit_fingerprint(surface_prefix + ".fixed96_float64_hash",
                         hash_tensor_stable_bytes(surface.fixed96_by_channel));
      }
    }
    const auto generator_after = current_generator_state_snapshot(device);
    const auto parameters_after = snapshot_parameters(model);
    const uint64_t parameter_hash_after =
        rssm_parameter_snapshot_hash(parameters_after);
    measurement.parameters_and_rng_unchanged =
        parameter_max_abs_diff(model, parameters_before) == 0.0 &&
        parameter_hash_before == parameter_hash_after &&
        generator_state_snapshot_equal(generator_before, generator_after) &&
        model->is_training() == initial_mode;

    all_public_sandwich_exact =
        all_public_sandwich_exact && measurement.public_sandwich_exact;
    all_repeated_capture_exact =
        all_repeated_capture_exact && measurement.repeated_capture_exact;
    all_parameters_and_rng_unchanged = all_parameters_and_rng_unchanged &&
                                       measurement.parameters_and_rng_unchanged;
    all_production_order_exact =
        all_production_order_exact && measurement.production_order_exact;
    all_cardinality_exact =
        all_cardinality_exact && measurement.cardinality_exact;

    const std::string seed_prefix =
        "rssm.seed_" + std::to_string(measurement.seed);
    std::cout << seed_prefix
              << ".public_sandwich_exact=" << measurement.public_sandwich_exact
              << '\n';
    std::cout << seed_prefix << ".repeated_capture_exact="
              << measurement.repeated_capture_exact << '\n';
    std::cout << seed_prefix << ".parameters_and_rng_unchanged="
              << measurement.parameters_and_rng_unchanged << '\n';
    emit_fingerprint(seed_prefix + ".parameter_hash_before",
                     parameter_hash_before);
    emit_fingerprint(seed_prefix + ".parameter_hash_after",
                     parameter_hash_after);
    emit_fingerprint(seed_prefix + ".cpu_generator_hash_before",
                     generator_before.digest.cpu);
    emit_fingerprint(seed_prefix + ".cpu_generator_hash_after",
                     generator_after.digest.cpu);
    emit_fingerprint(seed_prefix + ".cuda_generator_hash_before",
                     generator_before.digest.cuda);
    emit_fingerprint(seed_prefix + ".cuda_generator_hash_after",
                     generator_after.digest.cuda);
    std::cout << seed_prefix << ".production_order_exact="
              << measurement.production_order_exact << '\n';
    std::cout << seed_prefix
              << ".token_cardinality_exact=" << measurement.cardinality_exact
              << '\n';
  }

  const bool prefit_mechanics =
      tokenizer_plan.pass && projections_valid && permutation_contracts &&
      order_shuffle_balanced && bootstrap_valid && ridge_equivalence.pass &&
      dataset_identity_exact && all_public_sandwich_exact &&
      all_repeated_capture_exact && all_parameters_and_rng_unchanged &&
      all_production_order_exact && all_cardinality_exact &&
      cross_seed_token_structure_exact && all_metadata_plan_exact;
  std::cout << "rssm.prefit.mechanics_pass=" << prefit_mechanics << '\n';
  if (!prefit_mechanics) {
    std::cout << "rssm.attempt.consumed=false\n";
    std::cout << "training_authorized=false\n";
    std::cout << "long_run_authorized=false\n";
    std::cout << "production_or_end_to_end_authorized=false\n";
    std::cout << "follow_on_repair_authorized=false\n";
    std::cout << "execution_status=rssm_prefit_mechanics_failed\n";
    return 3;
  }

  std::cout << "rssm.attempt.consumed=true\n";
  const auto legacy_raw_curve =
      probe_curve(legacy_raw_train, legacy_raw_validation, legacy_raw_test,
                  probe_train.target, probe_validation.target, test.target,
                  {32, 64, 128, 256});
  validate_probe_curve_finite(legacy_raw_curve, "RSSM legacy raw reference");
  rssm_emit_legacy_raw_reference(legacy_raw_curve);
  const bool legacy_raw_reference_exact =
      rssm_legacy_raw_reference_exact(legacy_raw_curve);

  for (std::size_t track_index = 0; track_index < kRssmTrackCount;
       ++track_index) {
    const auto track = static_cast<RssmTrack>(track_index);
    const bool dual = rssm_native_uses_dual(RssmSurface::raw, track);
    const auto select_flat =
        [&](std::size_t dataset_index) -> const torch::Tensor & {
      const auto &surface = raw_surfaces[dataset_index];
      return track == RssmTrack::native ? surface.native_flat
                                        : surface.fixed96_flat;
    };
    const auto select_by_channel =
        [&](std::size_t dataset_index) -> const torch::Tensor & {
      const auto &surface = raw_surfaces[dataset_index];
      return track == RssmTrack::native ? surface.native_by_channel
                                        : surface.fixed96_by_channel;
    };
    raw_real[track_index] = rssm_probe_curve(
        select_flat(kTrainDataset), select_flat(kValidationDataset),
        select_flat(kTestDataset), probe_train.target, probe_validation.target,
        test.target, dual);
    raw_shuffled[track_index] = rssm_probe_curve(
        select_flat(kTrainDataset), select_flat(kValidationDataset),
        select_flat(kTestDataset), shuffled_train_target,
        shuffled_validation_target, shuffled_test_target, dual);
    const auto order_train_features = rssm_interleave_pairs(
        select_flat(kTrainDataset), select_flat(kReversedTrainDataset));
    const auto order_validation_features =
        rssm_interleave_pairs(select_flat(kValidationDataset),
                              select_flat(kReversedValidationDataset));
    const auto order_test_features = rssm_interleave_pairs(
        select_flat(kTestDataset), select_flat(kReversedTestDataset));
    raw_order[track_index] = rssm_order_curve(
        order_train_features, order_validation_features, order_test_features,
        order_fit_targets, order_validation_target, order_test_target, dual);
    raw_order_shuffled[track_index] = rssm_order_curve(
        order_train_features, order_validation_features, order_test_features,
        shuffled_order_fit_targets, shuffled_order_validation_target,
        shuffled_order_test_target, dual);
    rssm_validate_order_curve_finite(raw_order[track_index], "RSSM raw order");
    rssm_validate_order_curve_finite(raw_order_shuffled[track_index],
                                     "RSSM raw shuffled order");
    raw_geometry[track_index] =
        rssm_geometry_by_channel(select_by_channel(kTestDataset));
    raw_robustness[track_index] = rssm_robustness(
        select_flat(kTestDataset), select_flat(kNuisanceDataset),
        select_flat(kSemanticDataset), bootstrap_rows);
    rssm_validate_robustness_finite(raw_robustness[track_index], "RSSM raw");
  }

  for (std::size_t seed_index = 0; seed_index < measurements.size();
       ++seed_index) {
    auto &measurement = measurements[seed_index];
    const auto &feature_sets = feature_sets_by_seed[seed_index];
    for (std::size_t surface_index = 0; surface_index < kRssmSurfaceCount;
         ++surface_index) {
      const auto surface = static_cast<RssmSurface>(surface_index);
      for (std::size_t track_index = 0; track_index < kRssmTrackCount;
           ++track_index) {
        const auto track = static_cast<RssmTrack>(track_index);
        if (surface == RssmSurface::raw) {
          measurement.real[surface_index][track_index] = raw_real[track_index];
          measurement.shuffled[surface_index][track_index] =
              raw_shuffled[track_index];
          measurement.order[surface_index][track_index] =
              raw_order[track_index];
          measurement.order_shuffled[surface_index][track_index] =
              raw_order_shuffled[track_index];
        } else if (surface == RssmSurface::served &&
                   track == RssmTrack::fixed96) {
          measurement.real[surface_index][track_index] =
              measurement.real[surface_index][rssm_index(RssmTrack::native)];
          measurement.shuffled[surface_index][track_index] =
              measurement
                  .shuffled[surface_index][rssm_index(RssmTrack::native)];
          measurement.order[surface_index][track_index] =
              measurement.order[surface_index][rssm_index(RssmTrack::native)];
          measurement.order_shuffled[surface_index][track_index] =
              measurement
                  .order_shuffled[surface_index][rssm_index(RssmTrack::native)];
        } else {
          const bool dual = rssm_native_uses_dual(surface, track);
          const auto &train_features =
              rssm_flat(feature_sets[kTrainDataset], surface, track);
          const auto &validation_features =
              rssm_flat(feature_sets[kValidationDataset], surface, track);
          const auto &test_features =
              rssm_flat(feature_sets[kTestDataset], surface, track);
          measurement.real[surface_index][track_index] = rssm_probe_curve(
              train_features, validation_features, test_features,
              probe_train.target, probe_validation.target, test.target, dual);
          measurement.shuffled[surface_index][track_index] = rssm_probe_curve(
              train_features, validation_features, test_features,
              shuffled_train_target, shuffled_validation_target,
              shuffled_test_target, dual);
          const auto order_train_features = rssm_interleave_pairs(
              train_features,
              rssm_flat(feature_sets[kReversedTrainDataset], surface, track));
          const auto order_validation_features = rssm_interleave_pairs(
              validation_features,
              rssm_flat(feature_sets[kReversedValidationDataset], surface,
                        track));
          const auto order_test_features = rssm_interleave_pairs(
              test_features,
              rssm_flat(feature_sets[kReversedTestDataset], surface, track));
          measurement.order[surface_index][track_index] = rssm_order_curve(
              order_train_features, order_validation_features,
              order_test_features, order_fit_targets, order_validation_target,
              order_test_target, dual);
          measurement.order_shuffled[surface_index][track_index] =
              rssm_order_curve(order_train_features, order_validation_features,
                               order_test_features, shuffled_order_fit_targets,
                               shuffled_order_validation_target,
                               shuffled_order_test_target, dual);
        }
        validate_probe_curve_finite(
            measurement.real[surface_index][track_index],
            "RSSM real surface probe");
        validate_probe_curve_finite(
            measurement.shuffled[surface_index][track_index],
            "RSSM shuffled surface probe");
        rssm_validate_order_curve_finite(
            measurement.order[surface_index][track_index], "RSSM real order");
        rssm_validate_order_curve_finite(
            measurement.order_shuffled[surface_index][track_index],
            "RSSM shuffled order");
        if (surface == RssmSurface::raw) {
          measurement.geometry[surface_index][track_index] =
              raw_geometry[track_index];
          measurement.robustness[surface_index][track_index] =
              raw_robustness[track_index];
        } else {
          measurement.geometry[surface_index][track_index] =
              rssm_geometry_by_channel(
                  rssm_by_channel(feature_sets[kTestDataset], surface, track));
          measurement.robustness[surface_index][track_index] = rssm_robustness(
              rssm_flat(feature_sets[kTestDataset], surface, track),
              rssm_flat(feature_sets[kNuisanceDataset], surface, track),
              rssm_flat(feature_sets[kSemanticDataset], surface, track),
              bootstrap_rows);
          rssm_validate_robustness_finite(
              measurement.robustness[surface_index][track_index],
              "RSSM surface");
        }
      }
    }
    const auto &accepted_by_channel =
        feature_sets[kTestDataset][rssm_index(RssmSurface::served)]
            .fixed96_by_channel;
    measurement.accepted_served_geometry =
        geometry({.by_channel = accepted_by_channel,
                  .flat = accepted_by_channel.reshape(
                      {accepted_by_channel.size(0), -1})});

    const std::string seed_prefix =
        "rssm.seed_" + std::to_string(measurement.seed);
    for (std::size_t surface = 0; surface < kRssmSurfaceCount; ++surface) {
      for (std::size_t track = 0; track < kRssmTrackCount; ++track) {
        const std::string prefix = seed_prefix + ".surface." +
                                   kRssmSurfaceNames[surface] + "." +
                                   kRssmTrackNames[track];
        rssm_emit_probe_curve(prefix + ".probe",
                              measurement.real[surface][track]);
        rssm_emit_probe_curve(prefix + ".shuffled_probe",
                              measurement.shuffled[surface][track]);
        rssm_emit_order_curve(prefix + ".order_probe",
                              measurement.order[surface][track]);
        rssm_emit_order_curve(prefix + ".order_shuffled_probe",
                              measurement.order_shuffled[surface][track]);
      }
    }
    rssm_emit_geometry_grid(seed_prefix, measurement.geometry);
    rssm_emit_robustness_grid(seed_prefix, measurement.robustness);
    rssm_emit_accepted_seed_reference(measurement);
  }

  const bool accepted_reference_exact =
      rssm_emit_accepted_summary(measurements);

  rssm_gate::GateInput gate_input{};
  const auto make_track = [&](RssmTrack track) {
    rssm_gate::TrackInput result{};
    const auto raw = rssm_curves_for(measurements, RssmSurface::raw, track,
                                     /*shuffled=*/false);
    const auto tokenizer = rssm_curves_for(measurements, RssmSurface::tokenizer,
                                           track, /*shuffled=*/false);
    const auto encoder = rssm_curves_for(measurements, RssmSurface::encoder,
                                         track, /*shuffled=*/false);
    const auto served = rssm_curves_for(measurements, RssmSurface::served,
                                        track, /*shuffled=*/false);
    result.tokenizer_minus_raw =
        rssm_contrast(tokenizer, test.target, raw, test.target, bootstrap_rows);
    result.encoder_minus_tokenizer = rssm_contrast(
        encoder, test.target, tokenizer, test.target, bootstrap_rows);
    result.served_minus_encoder = rssm_contrast(served, test.target, encoder,
                                                test.target, bootstrap_rows);
    result.served_minus_raw =
        rssm_contrast(served, test.target, raw, test.target, bootstrap_rows);
    return result;
  };
  gate_input.native = make_track(RssmTrack::native);
  gate_input.fixed96 = make_track(RssmTrack::fixed96);
  RssmCurveBySeed legacy_raw_by_seed{};
  legacy_raw_by_seed.fill(legacy_raw_curve);
  const auto served_fixed =
      rssm_curves_for(measurements, RssmSurface::served, RssmTrack::fixed96,
                      /*shuffled=*/false);
  gate_input.legacy_served_minus_raw =
      rssm_contrast(served_fixed, test.target, legacy_raw_by_seed, test.target,
                    bootstrap_rows);

  bool continuous_shuffle_pass = true;
  bool order_shuffle_pass = true;
  for (std::size_t surface_index = 0; surface_index < kRssmSurfaceCount;
       ++surface_index) {
    for (std::size_t track_index = 0; track_index < kRssmTrackCount;
         ++track_index) {
      if (surface_index == rssm_index(RssmSurface::served) &&
          track_index == rssm_index(RssmTrack::fixed96)) {
        continue;
      }
      const auto surface = static_cast<RssmSurface>(surface_index);
      const auto track = static_cast<RssmTrack>(track_index);
      const auto shuffled_summary = rssm_curve_interval(
          rssm_curves_for(measurements, surface, track, /*shuffled=*/true),
          shuffled_test_target, bootstrap_rows);
      const auto order_summary = rssm_order_curve_interval(
          rssm_order_curves_for(measurements, surface, track,
                                /*shuffled=*/false),
          order_test_target, bootstrap_rows);
      const auto order_shuffled_summary = rssm_order_curve_interval(
          rssm_order_curves_for(measurements, surface, track,
                                /*shuffled=*/true),
          shuffled_order_test_target, bootstrap_rows);
      continuous_shuffle_pass = continuous_shuffle_pass &&
                                shuffled_summary.point <= 0.02 &&
                                shuffled_summary.interval.high <= 0.05;
      order_shuffle_pass = order_shuffle_pass &&
                           order_shuffled_summary.point <= 0.55 &&
                           order_shuffled_summary.interval.high <= 0.60;
      int64_t order_positive_seed_count = 0;
      const auto order_curves = rssm_order_curves_for(
          measurements, surface, track, /*shuffled=*/false);
      for (const auto &curve : order_curves) {
        order_positive_seed_count += curve.area > 0.50 ? 1 : 0;
      }
      std::string order_status = "order_unresolved";
      if (order_summary.point >= 0.60 && order_summary.interval.low > 0.50 &&
          order_positive_seed_count >= 2) {
        order_status = "order_decodable";
      } else if (order_summary.interval.high <= 0.55) {
        order_status = "order_chance_consistent";
      }
      const std::string prefix = "rssm.summary.surface." +
                                 std::string(kRssmSurfaceNames[surface_index]) +
                                 "." + kRssmTrackNames[track_index];
      std::cout << prefix << ".shuffled_aulc.point=" << shuffled_summary.point
                << '\n';
      std::cout << prefix << ".shuffled_aulc.bootstrap_95_low="
                << shuffled_summary.interval.low << '\n';
      std::cout << prefix << ".shuffled_aulc.bootstrap_95_high="
                << shuffled_summary.interval.high << '\n';
      std::cout << prefix << ".order.point=" << order_summary.point << '\n';
      std::cout << prefix
                << ".order.bootstrap_95_low=" << order_summary.interval.low
                << '\n';
      std::cout << prefix
                << ".order.bootstrap_95_high=" << order_summary.interval.high
                << '\n';
      std::cout << prefix
                << ".order.positive_seed_count=" << order_positive_seed_count
                << '\n';
      std::cout << prefix << ".order.status=" << order_status << '\n';
      std::cout << prefix
                << ".order_shuffled.point=" << order_shuffled_summary.point
                << '\n';
      std::cout << prefix << ".order_shuffled.bootstrap_95_high="
                << order_shuffled_summary.interval.high << '\n';
    }
  }

  const auto normalized_raw_real =
      rssm_curves_for(measurements, RssmSurface::raw, RssmTrack::fixed96,
                      /*shuffled=*/false);
  const auto normalized_raw_shuffled =
      rssm_curves_for(measurements, RssmSurface::raw, RssmTrack::fixed96,
                      /*shuffled=*/true);
  const auto normalized_raw_summary =
      rssm_curve_interval(normalized_raw_real, test.target, bootstrap_rows);
  const auto normalized_raw_minus_shuffled =
      rssm_contrast(normalized_raw_real, test.target, normalized_raw_shuffled,
                    shuffled_test_target, bootstrap_rows);
  const bool normalized_raw_informative =
      normalized_raw_summary.point >= 0.50 &&
      normalized_raw_minus_shuffled.low > 0.20;
  const bool surface_identity_hashes_match =
      dataset_identity_exact && cross_seed_token_structure_exact &&
      all_metadata_plan_exact && all_public_sandwich_exact;

  gate_input.validity = {
      .no_optimizer_or_backward = true,
      .parameters_and_rng_unchanged = all_parameters_and_rng_unchanged,
      .repeated_capture_identical = all_repeated_capture_exact,
      .surface_identity_hashes_match = surface_identity_hashes_match,
      .projections_valid = projections_valid && permutation_contracts,
      .accepted_reference_reproduced = accepted_reference_exact,
      .legacy_raw_reference_reproduced = legacy_raw_reference_exact,
      .tokenizer_plan_reproduced = tokenizer_plan.pass &&
                                   all_production_order_exact &&
                                   all_cardinality_exact,
      .normalized_fixed96_informative = normalized_raw_informative,
      .shuffled_controls_pass = continuous_shuffle_pass && order_shuffle_pass,
  };
  const auto gate = rssm_gate::evaluate(gate_input);

  rssm_emit_transition("rssm.summary.transition.native.tokenizer_minus_raw",
                       gate_input.native.tokenizer_minus_raw);
  rssm_emit_transition("rssm.summary.transition.native.encoder_minus_tokenizer",
                       gate_input.native.encoder_minus_tokenizer);
  rssm_emit_transition("rssm.summary.transition.native.served_minus_encoder",
                       gate_input.native.served_minus_encoder);
  rssm_emit_transition("rssm.summary.transition.native.served_minus_raw",
                       gate_input.native.served_minus_raw);
  rssm_emit_transition("rssm.summary.transition.fixed96.tokenizer_minus_raw",
                       gate_input.fixed96.tokenizer_minus_raw);
  rssm_emit_transition(
      "rssm.summary.transition.fixed96.encoder_minus_tokenizer",
      gate_input.fixed96.encoder_minus_tokenizer);
  rssm_emit_transition("rssm.summary.transition.fixed96.served_minus_encoder",
                       gate_input.fixed96.served_minus_encoder);
  rssm_emit_transition("rssm.summary.transition.fixed96.served_minus_raw",
                       gate_input.fixed96.served_minus_raw);
  rssm_emit_transition("rssm.summary.transition.legacy.served_minus_raw",
                       gate_input.legacy_served_minus_raw);
  rssm_emit_transition(
      "rssm.summary.control.normalized_raw_real_minus_shuffled",
      normalized_raw_minus_shuffled);

  std::cout << "rssm.summary.control.normalized_raw_fixed96_aulc="
            << normalized_raw_summary.point << '\n';
  std::cout << "rssm.summary.validity.public_sandwich_exact="
            << all_public_sandwich_exact << '\n';
  std::cout << "rssm.summary.validity.repeated_capture_exact="
            << all_repeated_capture_exact << '\n';
  std::cout << "rssm.summary.validity.parameters_and_rng_unchanged="
            << all_parameters_and_rng_unchanged << '\n';
  std::cout << "rssm.summary.validity.production_order_exact="
            << all_production_order_exact << '\n';
  std::cout << "rssm.summary.validity.token_cardinality_exact="
            << all_cardinality_exact << '\n';
  std::cout << "rssm.summary.validity.dataset_identity_exact="
            << dataset_identity_exact << '\n';
  std::cout << "rssm.summary.validity.cross_seed_token_structure_exact="
            << cross_seed_token_structure_exact << '\n';
  std::cout << "rssm.summary.validity.metadata_plan_exact="
            << all_metadata_plan_exact << '\n';
  std::cout << "rssm.summary.validity.surface_identity_hashes_match="
            << surface_identity_hashes_match << '\n';
  std::cout << "rssm.summary.validity.permutation_contracts="
            << permutation_contracts << '\n';
  std::cout << "rssm.summary.validity.tokenizer_plan_reproduced="
            << gate_input.validity.tokenizer_plan_reproduced << '\n';
  std::cout << "rssm.summary.validity.accepted_reference_exact_internal="
            << accepted_reference_exact << '\n';
  std::cout << "rssm.summary.validity.legacy_raw_reference_exact_internal="
            << legacy_raw_reference_exact << '\n';
  std::cout << "rssm.summary.validity.normalized_raw_informative="
            << normalized_raw_informative << '\n';
  std::cout << "rssm.summary.validity.continuous_shuffle_pass="
            << continuous_shuffle_pass << '\n';
  std::cout << "rssm.summary.validity.order_shuffle_pass=" << order_shuffle_pass
            << '\n';
  std::cout << "rssm.summary.validity.all_controls_pass="
            << gate.validity_controls_pass << '\n';
  std::cout << "rssm.summary.gate.raw_classification="
            << rssm_gate::terminal_classification_name(gate.classification)
            << '\n';
  std::cout << "rssm.summary.gate.reference_audit_pending=true\n";
  std::cout << "rssm.summary.gate.final_classification_requires_postrun_audit="
               "true\n";
  std::cout << "training_authorized=false\n";
  std::cout << "long_run_authorized=false\n";
  std::cout << "production_or_end_to_end_authorized=false\n";
  std::cout << "follow_on_repair_authorized=false\n";
  std::cout << "execution_status=rssm_measurements_complete\n";
  return gate.validity_controls_pass ? 0 : 3;
}

constexpr std::size_t kPsmArmCount = 5;
constexpr std::array<const char *, kPsmArmCount> kPsmArmNames{
    "channel", "channel_domain", "channel_domain_scale",
    "channel_domain_scale_bin", "encoder"};
constexpr std::array<int64_t, kPsmArmCount> kPsmCellCounts{1, 2, 8, 16,
                                                           24};

enum class PsmArm : std::size_t {
  channel = 0,
  channel_domain = 1,
  channel_domain_scale = 2,
  channel_domain_scale_bin = 3,
  encoder = 4,
};

[[nodiscard]] constexpr std::size_t psm_index(PsmArm arm) {
  return static_cast<std::size_t>(arm);
}

constexpr std::array<int64_t, kRssmTokensPerChannel> kPsmChannelCells{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
constexpr std::array<int64_t, kRssmTokensPerChannel> kPsmDomainCells{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
constexpr std::array<int64_t, kRssmTokensPerChannel> kPsmDomainScaleCells{
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 3,
    4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 6, 7};
constexpr std::array<int64_t, kRssmTokensPerChannel>
    kPsmDomainScaleBinCells{0,  0,  1,  1,  1,  2,  2,  3,
                            4,  5,  6,  7,  8,  8,  9,  9,
                            9,  10, 10, 11, 12, 13, 14, 15};
constexpr std::array<int64_t, kRssmTokensPerChannel> kPsmEncoderCells{
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11,
    12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23};
constexpr std::array<std::array<int64_t, kRssmTokensPerChannel>, kPsmArmCount>
    kPsmCellIds{kPsmChannelCells, kPsmDomainCells, kPsmDomainScaleCells,
                kPsmDomainScaleBinCells, kPsmEncoderCells};
constexpr std::array<int64_t, kRssmTokensPerChannel> kPsmExpectedDomains{
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1};
constexpr std::array<int64_t, kRssmTokensPerChannel> kPsmExpectedScales{
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 3,
    0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 3};

struct PsmTokenLayoutReceipt {
  bool production_order_exact{false};
  bool domain_scale_layout_exact{false};
  bool channels_share_layout{false};
  uint64_t layout_hash{0};
  bool pass{false};
};

[[nodiscard]] PsmTokenLayoutReceipt psm_token_layout_receipt() {
  auto config = active_config(torch::Device(torch::kCPU), /*weak_views=*/true);
  config.dtype = torch::kFloat64;
  config.device = torch::Device(torch::kCPU);
  torch::manual_seed(1203);
  auto builder = mtf::TimeFrequencyViewBuilder(config);
  builder->eval();
  torch::NoGradGuard no_grad;
  const auto batch = builder->forward(torch::zeros(
      {1, kChannels, kHistory, kFeatures}, torch::kFloat64));
  const auto order = rssm_token_order(batch.metadata, batch.tokens.size(1));
  const auto domains =
      batch.metadata.domain_id.to(torch::kCPU, torch::kInt64).contiguous();
  const auto scales =
      batch.metadata.scale_id.to(torch::kCPU, torch::kInt64).contiguous();
  const auto starts =
      batch.metadata.start_index.to(torch::kCPU, torch::kInt64).contiguous();
  const auto widths =
      batch.metadata.width.to(torch::kCPU, torch::kInt64).contiguous();
  const auto domain = domains.accessor<int64_t, 1>();
  const auto scale = scales.accessor<int64_t, 1>();
  const auto start = starts.accessor<int64_t, 1>();
  const auto width = widths.accessor<int64_t, 1>();
  auto reference = torch::empty({kRssmTokensPerChannel, 4}, torch::kInt64);
  auto reference_values = reference.accessor<int64_t, 2>();
  PsmTokenLayoutReceipt result{};
  result.production_order_exact =
      order.production_order_exact && order.cardinality_exact;
  result.domain_scale_layout_exact = true;
  result.channels_share_layout = true;
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    const auto &indices =
        order.channel_indices[static_cast<std::size_t>(channel)];
    for (int64_t position = 0; position < kRssmTokensPerChannel; ++position) {
      const int64_t token = indices[static_cast<std::size_t>(position)];
      result.domain_scale_layout_exact =
          result.domain_scale_layout_exact &&
          domain[token] ==
              kPsmExpectedDomains[static_cast<std::size_t>(position)] &&
          scale[token] ==
              kPsmExpectedScales[static_cast<std::size_t>(position)];
      if (channel == 0) {
        reference_values[position][0] = domain[token];
        reference_values[position][1] = scale[token];
        reference_values[position][2] = start[token];
        reference_values[position][3] = width[token];
      } else {
        result.channels_share_layout =
            result.channels_share_layout &&
            reference_values[position][0] == domain[token] &&
            reference_values[position][1] == scale[token] &&
            reference_values[position][2] == start[token] &&
            reference_values[position][3] == width[token];
      }
    }
  }
  result.layout_hash = hash_tensor_stable_bytes(reference);
  result.pass = result.production_order_exact &&
                result.domain_scale_layout_exact &&
                result.channels_share_layout;
  return result;
}

[[nodiscard]] bool
psm_capture_layout_exact(const RssmEncodedCapture &capture,
                         const PsmTokenLayoutReceipt &expected) {
  if (!capture.grouped_metadata_layout.defined() ||
      capture.grouped_metadata_layout.sizes() !=
          torch::IntArrayRef(
              {kChannels, kRssmTokensPerChannel, int64_t{4}}) ||
      capture.grouped_metadata_layout.scalar_type() != torch::kInt64 ||
      !capture.grouped_metadata_layout.device().is_cpu()) {
    return false;
  }
  const auto layout = capture.grouped_metadata_layout.contiguous();
  const auto values = layout.accessor<int64_t, 3>();
  bool exact = expected.pass;
  for (int64_t channel = 0; channel < kChannels; ++channel) {
    exact = exact &&
            hash_tensor_stable_bytes(layout.select(0, channel).contiguous()) ==
                expected.layout_hash;
    for (int64_t position = 0; position < kRssmTokensPerChannel; ++position) {
      exact = exact &&
              values[channel][position][0] ==
                  kPsmExpectedDomains[static_cast<std::size_t>(position)] &&
              values[channel][position][1] ==
                  kPsmExpectedScales[static_cast<std::size_t>(position)];
    }
  }
  return exact;
}

[[nodiscard]] torch::Tensor psm_partition_lift(const torch::Tensor &input,
                                                PsmArm arm) {
  if (input.dim() != 3 || input.size(1) != kChannels ||
      input.size(2) != kRssmTokenChannelWidth ||
      input.scalar_type() != torch::kFloat64 || !input.device().is_cpu()) {
    throw std::runtime_error(
        "PSM partition input must be CPU float64 [S,3,768]");
  }
  if (arm == PsmArm::encoder) {
    return input.contiguous();
  }
  const auto values = input.reshape({input.size(0), kChannels,
                                     kRssmTokensPerChannel, kLatentDim});
  const auto &cell_ids = kPsmCellIds[psm_index(arm)];
  const int64_t cell_count = kPsmCellCounts[psm_index(arm)];
  std::vector<torch::Tensor> means(static_cast<std::size_t>(cell_count));
  for (int64_t cell = 0; cell < cell_count; ++cell) {
    std::vector<int64_t> positions;
    for (int64_t position = 0; position < kRssmTokensPerChannel; ++position) {
      if (cell_ids[static_cast<std::size_t>(position)] == cell) {
        positions.push_back(position);
      }
    }
    if (positions.empty()) {
      throw std::runtime_error("PSM partition contains an empty cell");
    }
    const auto index = torch::tensor(positions, torch::kInt64);
    means[static_cast<std::size_t>(cell)] =
        values.index_select(2, index).mean(2);
  }
  std::vector<torch::Tensor> lifted;
  lifted.reserve(kRssmTokensPerChannel);
  for (int64_t position = 0; position < kRssmTokensPerChannel; ++position) {
    lifted.push_back(means[static_cast<std::size_t>(
        cell_ids[static_cast<std::size_t>(position)])]);
  }
  return torch::stack(lifted, 2)
      .reshape({input.size(0), kChannels, kRssmTokenChannelWidth})
      .contiguous();
}

[[nodiscard]] double psm_max_abs(const torch::Tensor &left,
                                 const torch::Tensor &right) {
  if (left.sizes() != right.sizes()) {
    return std::numeric_limits<double>::infinity();
  }
  return (left.to(torch::kCPU, torch::kFloat64) -
          right.to(torch::kCPU, torch::kFloat64))
      .abs()
      .max()
      .item<double>();
}

struct PsmPartitionReceipt {
  bool cell_counts_exact{false};
  bool finite_and_shape_exact{false};
  bool encoder_identity_exact{false};
  double channel_mean_max_abs{0.0};
  double idempotence_max_abs{0.0};
  double nesting_max_abs{0.0};
  bool pass{false};
};

[[nodiscard]] PsmPartitionReceipt psm_partition_receipt() {
  const auto input =
      torch::arange(4 * kChannels * kRssmTokenChannelWidth, torch::kFloat64)
          .reshape({4, kChannels, kRssmTokenChannelWidth}) /
      997.0;
  PsmPartitionReceipt result{};
  result.cell_counts_exact = true;
  result.finite_and_shape_exact = true;
  for (std::size_t arm_index = 0; arm_index < kPsmArmCount; ++arm_index) {
    std::set<int64_t> cells(kPsmCellIds[arm_index].begin(),
                            kPsmCellIds[arm_index].end());
    result.cell_counts_exact =
        result.cell_counts_exact &&
        cells.size() == static_cast<std::size_t>(kPsmCellCounts[arm_index]) &&
        *cells.begin() == 0 &&
        *cells.rbegin() == kPsmCellCounts[arm_index] - 1;
    const auto arm = static_cast<PsmArm>(arm_index);
    const auto once = psm_partition_lift(input, arm);
    const auto twice = psm_partition_lift(once, arm);
    result.finite_and_shape_exact =
        result.finite_and_shape_exact && once.sizes() == input.sizes() &&
        once.scalar_type() == input.scalar_type() &&
        once.device() == input.device() &&
        torch::isfinite(once).all().item<bool>();
    result.idempotence_max_abs =
        std::max(result.idempotence_max_abs, psm_max_abs(once, twice));
  }
  const auto tokens = input.reshape(
      {4, kChannels, kRssmTokensPerChannel, kLatentDim});
  const auto direct_channel =
      tokens.mean(2, true)
          .expand({4, kChannels, kRssmTokensPerChannel, kLatentDim})
          .reshape_as(input);
  result.channel_mean_max_abs =
      psm_max_abs(psm_partition_lift(input, PsmArm::channel), direct_channel);
  result.encoder_identity_exact = rssm_tensor_bytes_equal(
      psm_partition_lift(input, PsmArm::encoder), input);
  for (std::size_t coarse = 0; coarse < kPsmArmCount; ++coarse) {
    const auto direct =
        psm_partition_lift(input, static_cast<PsmArm>(coarse));
    for (std::size_t fine = coarse; fine < kPsmArmCount; ++fine) {
      const auto nested = psm_partition_lift(
          psm_partition_lift(input, static_cast<PsmArm>(fine)),
          static_cast<PsmArm>(coarse));
      result.nesting_max_abs =
          std::max(result.nesting_max_abs, psm_max_abs(nested, direct));
    }
  }
  result.pass = result.cell_counts_exact && result.finite_and_shape_exact &&
                result.encoder_identity_exact &&
                result.channel_mean_max_abs <= 1.0e-12 &&
                result.idempotence_max_abs <= 1.0e-12 &&
                result.nesting_max_abs <= 1.0e-12;
  return result;
}

[[nodiscard]] torch::Tensor psm_mean_basis() {
  auto basis =
      torch::zeros({kRssmTokenChannelWidth, kLatentDim}, torch::kFloat64);
  auto values = basis.accessor<double, 2>();
  const double scale = 1.0 / std::sqrt(kRssmTokensPerChannel);
  for (int64_t token = 0; token < kRssmTokensPerChannel; ++token) {
    for (int64_t feature = 0; feature < kLatentDim; ++feature) {
      values[token * kLatentDim + feature][feature] = scale;
    }
  }
  return basis;
}

[[nodiscard]] torch::Tensor
psm_make_projection(const torch::Tensor &rssm_projection) {
  torch::NoGradGuard no_grad;
  const auto basis = psm_mean_basis();
  const auto contrast_seed =
      rssm_projection - basis.matmul(basis.transpose(0, 1).matmul(
                            rssm_projection));
  auto [contrast, upper] = at::linalg_qr(contrast_seed, "reduced");
  const auto diagonal = upper.diagonal();
  const auto signs = torch::where(diagonal.lt(0.0), -torch::ones_like(diagonal),
                                  torch::ones_like(diagonal));
  contrast = contrast * signs.unsqueeze(0);
  const auto projection =
      basis / std::sqrt(kRssmTokensPerChannel) +
      std::sqrt(static_cast<double>(kRssmTokensPerChannel - 1) /
                static_cast<double>(kRssmTokensPerChannel)) *
          contrast;
  return projection.contiguous();
}

struct PsmProjectionReceipt {
  double orthogonality_error{0.0};
  double contrast_mean_error{0.0};
  double block_sum_identity_error{0.0};
  bool rssm_projection_hash_exact{false};
  bool finite_and_shape_exact{false};
  bool pass{false};
};

[[nodiscard]] PsmProjectionReceipt
psm_projection_receipt(const torch::Tensor &rssm_projection,
                       const torch::Tensor &psm_projection) {
  const auto basis = psm_mean_basis();
  const auto contrast =
      (psm_projection - basis / std::sqrt(kRssmTokensPerChannel)) /
      std::sqrt(static_cast<double>(kRssmTokensPerChannel - 1) /
                static_cast<double>(kRssmTokensPerChannel));
  const auto identity = torch::eye(kLatentDim, torch::kFloat64);
  PsmProjectionReceipt result{};
  result.orthogonality_error = rssm_projection_error(psm_projection);
  result.contrast_mean_error =
      basis.transpose(0, 1).matmul(contrast).abs().max().item<double>();
  result.block_sum_identity_error =
      (psm_projection.reshape(
                           {kRssmTokensPerChannel, kLatentDim, kLatentDim})
           .sum(0) -
       identity)
          .abs()
          .max()
          .item<double>();
  result.rssm_projection_hash_exact =
      hash_tensor_stable_bytes(rssm_projection) == 0xf8c9f35282de2ee0ULL;
  result.finite_and_shape_exact =
      psm_projection.sizes() ==
          torch::IntArrayRef({kRssmTokenChannelWidth, kLatentDim}) &&
      psm_projection.scalar_type() == torch::kFloat64 &&
      psm_projection.device().is_cpu() && psm_projection.is_contiguous() &&
      torch::isfinite(psm_projection).all().item<bool>();
  result.pass = result.finite_and_shape_exact &&
                result.rssm_projection_hash_exact &&
                result.orthogonality_error <= 1.0e-10 &&
                result.contrast_mean_error <= 1.0e-10 &&
                result.block_sum_identity_error <= 1.0e-10;
  return result;
}

struct PsmSurfaceFeatures {
  torch::Tensor by_channel{}; // [S,3,32], CPU float64
  torch::Tensor flat{};       // [S,96], CPU float64
};

struct PsmFeatureSet {
  std::array<PsmSurfaceFeatures, kPsmArmCount> arms{};
  torch::Tensor rssm_encoder_reference_flat{};
  double channel_served_max_abs{0.0};
  double encoder_projection_max_abs{0.0};
  bool finite_and_shape_exact{false};
  bool pass{false};
};

[[nodiscard]] PsmFeatureSet
psm_feature_set(const RssmEncodedCapture &capture,
                const torch::Tensor &psm_projection,
                const torch::Tensor &rssm_projection) {
  PsmFeatureSet result{};
  result.finite_and_shape_exact = true;
  for (std::size_t arm_index = 0; arm_index < kPsmArmCount; ++arm_index) {
    const auto arm = static_cast<PsmArm>(arm_index);
    // C is the exact production serving baseline.  Lift that captured value
    // back to a constant 24-token block, then apply the same Qpsm projection
    // as every other arm.  Recomputing the mean after the CUDA float32 result
    // has been converted to CPU float64 can differ by one float32 ULP and is
    // not the served representation that C is intended to reproduce.
    const auto lifted =
        arm == PsmArm::channel
            ? capture.served_by_channel.unsqueeze(2)
                  .expand({capture.served_by_channel.size(0), kChannels,
                           kRssmTokensPerChannel, kLatentDim})
                  .reshape({capture.served_by_channel.size(0), kChannels,
                            kRssmTokenChannelWidth})
                  .contiguous()
            : psm_partition_lift(capture.encoder_by_channel, arm);
    const auto projected = rssm_project_by_channel(lifted, psm_projection);
    auto &surface = result.arms[arm_index];
    if (arm == PsmArm::channel) {
      result.channel_served_max_abs =
          psm_max_abs(projected, capture.served_by_channel);
    }
    surface.by_channel = projected;
    surface.flat = surface.by_channel
                       .reshape({surface.by_channel.size(0), kServedWidth})
                       .contiguous();
    result.finite_and_shape_exact =
        result.finite_and_shape_exact &&
        surface.by_channel.sizes() ==
            torch::IntArrayRef({capture.encoder_by_channel.size(0), kChannels,
                                kLatentDim}) &&
        surface.flat.sizes() ==
            torch::IntArrayRef(
                {capture.encoder_by_channel.size(0), kServedWidth}) &&
        torch::isfinite(surface.flat).all().item<bool>();
    if (arm == PsmArm::encoder) {
      result.encoder_projection_max_abs = psm_max_abs(
          projected, rssm_project_by_channel(capture.encoder_by_channel,
                                             psm_projection));
    }
  }
  result.rssm_encoder_reference_flat =
      rssm_project_by_channel(capture.encoder_by_channel, rssm_projection)
          .reshape({capture.encoder_by_channel.size(0), kServedWidth})
          .contiguous();
  result.pass = result.finite_and_shape_exact &&
                result.channel_served_max_abs <= 1.0e-10 &&
                result.encoder_projection_max_abs <= 1.0e-12 &&
                torch::isfinite(result.rssm_encoder_reference_flat)
                    .all()
                    .item<bool>();
  return result;
}

int run_psm_preflight(const Options &options) {
  if (options.device != "cuda" || !torch::cuda::is_available()) {
    throw std::runtime_error("PSM preflight requires CUDA");
  }
  if (options.steps > 0 || options.seeds > 0 || !options.weak_views) {
    throw std::runtime_error(
        "PSM preflight accepts no training-step/seed/weak-view override");
  }
  const char *workspace = std::getenv("CUBLAS_WORKSPACE_CONFIG");
  const bool cublas_workspace =
      workspace != nullptr && std::string(workspace) == ":4096:8";
  const torch::Device device(torch::kCUDA, 0);
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);

  auto normalizer_rows = generate_dataset(4300000, 32);
  auto capture_rows = generate_dataset(4400000, 101, 1, false);
  const auto normalization = fit_normalization(normalizer_rows);
  normalize(normalizer_rows, normalization);
  normalize(capture_rows, normalization);
  validate_dataset(normalizer_rows);
  validate_dataset(capture_rows);

  const auto tokenizer_plan = rssm_tokenizer_plan_receipt();
  const auto token_layout = psm_token_layout_receipt();
  const auto partitions = psm_partition_receipt();
  const auto rssm_projection = rssm_make_token_projection();
  const auto psm_projection = psm_make_projection(rssm_projection);
  const auto projection =
      psm_projection_receipt(rssm_projection, psm_projection);
  const auto shuffle_train =
      rssm_sattolo_permutation(256, kRssmShuffleTrainTag);
  const auto shuffle_validation =
      rssm_sattolo_permutation(128, kRssmShuffleValidationTag);
  const auto shuffle_test =
      rssm_sattolo_permutation(256, kRssmShuffleTestTag);
  const auto order_fit_permutations = rssm_order_fit_permutations();
  const auto order_fit_shuffled_targets =
      rssm_order_fit_targets(&order_fit_permutations);
  const auto order_validation =
      rssm_sattolo_permutation(256, kRssmOrderShuffleValidationTag);
  const auto order_test =
      rssm_sattolo_permutation(512, kRssmOrderShuffleTestTag);
  bool permutations_valid =
      rssm_permutation_receipt(shuffle_train).pass &&
      rssm_permutation_receipt(shuffle_validation).pass &&
      rssm_permutation_receipt(shuffle_test).pass &&
      rssm_permutation_receipt(order_validation).pass &&
      rssm_permutation_receipt(order_test).pass;
  for (const auto &permutation : order_fit_permutations) {
    permutations_valid =
        permutations_valid && rssm_permutation_receipt(permutation).pass;
  }
  bool order_shuffle_balanced =
      rssm_order_target_balanced(
          rssm_order_labels(128).index_select(0, order_validation)) &&
      rssm_order_target_balanced(
          rssm_order_labels(256).index_select(0, order_test));
  for (const auto &target : order_fit_shuffled_targets) {
    order_shuffle_balanced =
        order_shuffle_balanced && rssm_order_target_balanced(target);
  }
  const auto bootstrap_rows = rssm_bootstrap_rows(256);
  const bool bootstrap_valid = rssm_bootstrap_contract(bootstrap_rows, 256);
  const auto ridge = rssm_ridge_equivalence_fixture();

  set_paired_rng(1701, device);
  auto model = mtf::MtfJepaMaeVicreg(
      attribution_config(device, kJmcdArms[kJmcdCombinedIndex]));
  const bool initial_mode = model->is_training();
  const auto parameters_before = snapshot_parameters(model);
  const uint64_t parameter_hash_before =
      rssm_parameter_snapshot_hash(parameters_before);
  const auto generator_before = current_generator_state_snapshot(device);
  const auto first = rssm_capture_once(model, capture_rows, device,
                                       /*check_direct_encoder=*/true);
  const auto second = rssm_capture_once(model, capture_rows, device,
                                        /*check_direct_encoder=*/true);
  const auto generator_after = current_generator_state_snapshot(device);
  const auto parameters_after = snapshot_parameters(model);
  const uint64_t parameter_hash_after =
      rssm_parameter_snapshot_hash(parameters_after);
  const bool repeated_capture_exact = rssm_capture_exact(first, second);
  const bool parameters_exact =
      parameter_max_abs_diff(model, parameters_before) == 0.0 &&
      parameter_hash_before == parameter_hash_after;
  const bool generator_exact =
      generator_state_snapshot_equal(generator_before, generator_after);
  const bool mode_exact = model->is_training() == initial_mode;
  const auto features =
      psm_feature_set(first, psm_projection, rssm_projection);

  const bool mechanics =
      cublas_workspace && tokenizer_plan.pass && token_layout.pass &&
      partitions.pass && projection.pass && permutations_valid &&
      order_shuffle_balanced && bootstrap_valid && ridge.pass &&
      first.public_sandwich_exact && second.public_sandwich_exact &&
      first.direct_encoder_exact && second.direct_encoder_exact &&
      first.production_order_exact && second.production_order_exact &&
      first.cardinality_exact && second.cardinality_exact &&
      psm_capture_layout_exact(first, token_layout) &&
      psm_capture_layout_exact(second, token_layout) &&
      repeated_capture_exact && parameters_exact && generator_exact &&
      mode_exact && features.pass;

  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.psm_preflight.v1\n";
  std::cout << "psm.preflight.scientific_rows_used=false\n";
  std::cout << "optimizer_constructed=false\n";
  std::cout << "optimizer_steps=0\n";
  std::cout << "backward_calls=0\n";
  std::cout << "scientific_probe_fits=0\n";
  std::cout << "psm.preflight.cublas_workspace_exact=" << cublas_workspace
            << '\n';
  std::cout << "psm.preflight.tokenizer_plan.pass=" << tokenizer_plan.pass
            << '\n';
  std::cout << "psm.preflight.token_layout.production_order_exact="
            << token_layout.production_order_exact << '\n';
  std::cout << "psm.preflight.token_layout.domain_scale_exact="
            << token_layout.domain_scale_layout_exact << '\n';
  std::cout << "psm.preflight.token_layout.channels_share_layout="
            << token_layout.channels_share_layout << '\n';
  emit_fingerprint("psm.preflight.token_layout.hash",
                   token_layout.layout_hash);
  std::cout << "psm.preflight.token_layout.pass=" << token_layout.pass << '\n';
  std::cout << "psm.preflight.partition.cell_counts_exact="
            << partitions.cell_counts_exact << '\n';
  std::cout << "psm.preflight.partition.finite_and_shape_exact="
            << partitions.finite_and_shape_exact << '\n';
  std::cout << "psm.preflight.partition.encoder_identity_exact="
            << partitions.encoder_identity_exact << '\n';
  std::cout << "psm.preflight.partition.channel_mean_max_abs="
            << partitions.channel_mean_max_abs << '\n';
  std::cout << "psm.preflight.partition.idempotence_max_abs="
            << partitions.idempotence_max_abs << '\n';
  std::cout << "psm.preflight.partition.nesting_max_abs="
            << partitions.nesting_max_abs << '\n';
  std::cout << "psm.preflight.partition.pass=" << partitions.pass << '\n';
  emit_fingerprint("psm.preflight.projection.rssm_hash",
                   hash_tensor_stable_bytes(rssm_projection));
  emit_fingerprint("psm.preflight.projection.psm_hash",
                   hash_tensor_stable_bytes(psm_projection));
  std::cout << "psm.preflight.projection.orthogonality_error="
            << projection.orthogonality_error << '\n';
  std::cout << "psm.preflight.projection.contrast_mean_error="
            << projection.contrast_mean_error << '\n';
  std::cout << "psm.preflight.projection.block_sum_identity_error="
            << projection.block_sum_identity_error << '\n';
  std::cout << "psm.preflight.projection.rssm_hash_exact="
            << projection.rssm_projection_hash_exact << '\n';
  std::cout << "psm.preflight.projection.pass=" << projection.pass << '\n';
  std::cout << "psm.preflight.features.channel_served_max_abs="
            << features.channel_served_max_abs << '\n';
  std::cout << "psm.preflight.features.encoder_projection_max_abs="
            << features.encoder_projection_max_abs << '\n';
  std::cout << "psm.preflight.features.pass=" << features.pass << '\n';
  std::cout << "psm.preflight.accepted_capture_layout_exact="
            << (psm_capture_layout_exact(first, token_layout) &&
                psm_capture_layout_exact(second, token_layout))
            << '\n';
  std::cout << "psm.preflight.public_sandwich_exact="
            << first.public_sandwich_exact << '\n';
  std::cout << "psm.preflight.direct_encoder_exact="
            << first.direct_encoder_exact << '\n';
  std::cout << "psm.preflight.repeated_capture_exact="
            << repeated_capture_exact << '\n';
  std::cout << "psm.preflight.parameters_exact=" << parameters_exact << '\n';
  std::cout << "psm.preflight.generator_state_exact=" << generator_exact
            << '\n';
  std::cout << "psm.preflight.model_mode_exact=" << mode_exact << '\n';
  emit_fingerprint("psm.preflight.parameter_hash_before",
                   parameter_hash_before);
  emit_fingerprint("psm.preflight.parameter_hash_after", parameter_hash_after);
  emit_fingerprint("psm.preflight.cpu_generator_hash_before",
                   generator_before.digest.cpu);
  emit_fingerprint("psm.preflight.cpu_generator_hash_after",
                   generator_after.digest.cpu);
  emit_fingerprint("psm.preflight.cuda_generator_hash_before",
                   generator_before.digest.cuda);
  emit_fingerprint("psm.preflight.cuda_generator_hash_after",
                   generator_after.digest.cuda);
  rssm_emit_permutation("psm.preflight.shuffle.fit", shuffle_train);
  rssm_emit_permutation("psm.preflight.shuffle.validation",
                        shuffle_validation);
  rssm_emit_permutation("psm.preflight.shuffle.test", shuffle_test);
  for (std::size_t index = 0; index < kRssmSampleLadder.size(); ++index) {
    rssm_emit_permutation("psm.preflight.order_shuffle.fit.n_" +
                              std::to_string(kRssmSampleLadder[index]),
                          order_fit_permutations[index]);
  }
  rssm_emit_permutation("psm.preflight.order_shuffle.validation",
                        order_validation);
  rssm_emit_permutation("psm.preflight.order_shuffle.test", order_test);
  std::cout << "psm.preflight.permutations_valid=" << permutations_valid
            << '\n';
  std::cout << "psm.preflight.order_shuffle_balanced="
            << order_shuffle_balanced << '\n';
  emit_fingerprint("psm.preflight.bootstrap_table_hash",
                   rssm_tensor_vector_hash(bootstrap_rows));
  std::cout << "psm.preflight.bootstrap_valid=" << bootstrap_valid << '\n';
  std::cout << "psm.preflight.ridge.pass=" << ridge.pass << '\n';
  emit_fingerprint("psm.preflight.encoder_source_hash",
                   first.encoder_source_hash);
  emit_fingerprint("psm.preflight.served_source_hash",
                   first.served_source_hash);
  emit_fingerprint("psm.preflight.metadata_structure_hash",
                   first.metadata_structure_hash);
  std::cout
      << "psm.preflight.authoritative_command="
         "CUBLAS_WORKSPACE_CONFIG=:4096:8 .build/tests/quality_wikimyei_mtf_"
         "jepa_mae_vicreg_representation --experiment pooling-structure-"
         "mechanism-map --device cuda\n";
  std::cout << "psm.preflight.pass=" << mechanics << '\n';
  std::cout << "training_authorized=false\n";
  std::cout << "long_run_authorized=false\n";
  std::cout << "production_or_end_to_end_authorized=false\n";
  std::cout << "follow_on_repair_authorized=false\n";
  return mechanics ? 0 : 3;
}

constexpr std::size_t kPsmDatasetCount = 6;
constexpr std::size_t kPsmTrainDataset = 0;
constexpr std::size_t kPsmValidationDataset = 1;
constexpr std::size_t kPsmTestDataset = 2;
constexpr std::size_t kPsmReversedTrainDataset = 3;
constexpr std::size_t kPsmReversedValidationDataset = 4;
constexpr std::size_t kPsmReversedTestDataset = 5;
constexpr std::array<const char *, kPsmDatasetCount> kPsmDatasetNames{
    "probe_train", "probe_validation", "test", "reversed_train",
    "reversed_validation", "reversed_test"};

struct PsmSeedMeasurement {
  int64_t seed{0};
  std::array<ProbeCurve, kPsmArmCount> real{};
  std::array<ProbeCurve, kPsmArmCount> shuffled{};
  std::array<RssmOrderCurve, kPsmArmCount> order{};
  std::array<RssmOrderCurve, kPsmArmCount> order_shuffled{};
  ProbeCurve rssm_encoder_reference{};
  RssmOrderCurve rssm_encoder_order_reference{};
  bool public_sandwich_exact{true};
  bool direct_encoder_exact{true};
  bool repeated_capture_exact{true};
  bool parameters_and_rng_unchanged{true};
  bool production_order_exact{true};
  bool cardinality_exact{true};
  bool token_layout_exact{true};
  bool feature_contracts_exact{true};
};

using PsmMeasurements =
    std::array<PsmSeedMeasurement, psm_gate::kSeedCount>;

[[nodiscard]] RssmCurveBySeed
psm_curves_for(const PsmMeasurements &measurements, PsmArm arm,
               bool shuffled) {
  RssmCurveBySeed result{};
  for (std::size_t seed = 0; seed < measurements.size(); ++seed) {
    result[seed] = shuffled ? measurements[seed].shuffled[psm_index(arm)]
                            : measurements[seed].real[psm_index(arm)];
  }
  return result;
}

[[nodiscard]] RssmOrderCurveBySeed
psm_order_curves_for(const PsmMeasurements &measurements, PsmArm arm,
                     bool shuffled) {
  RssmOrderCurveBySeed result{};
  for (std::size_t seed = 0; seed < measurements.size(); ++seed) {
    result[seed] = shuffled
                       ? measurements[seed].order_shuffled[psm_index(arm)]
                       : measurements[seed].order[psm_index(arm)];
  }
  return result;
}

[[nodiscard]] psm_gate::ContinuousInput
psm_continuous_input(const rssm_gate::TransitionInput &input) {
  return {.point = input.point,
          .low = input.low,
          .high = input.high,
          .seed_deltas = input.seed_deltas,
          .family_deltas = input.family_deltas};
}

[[nodiscard]] psm_gate::OrderInput
psm_order_input(const RssmOrderCurveBySeed &curves,
                const torch::Tensor &target,
                const std::vector<torch::Tensor> &bootstrap_rows) {
  const auto summary =
      rssm_order_curve_interval(curves, target, bootstrap_rows);
  psm_gate::OrderInput result{.point = summary.point,
                              .low = summary.interval.low,
                              .high = summary.interval.high};
  for (std::size_t seed = 0; seed < curves.size(); ++seed) {
    result.seed_points[seed] = curves[seed].area;
  }
  return result;
}

void psm_emit_continuous(const std::string &prefix,
                         const psm_gate::ContinuousInput &input) {
  const auto result = psm_gate::evaluate_continuous(input);
  std::cout << prefix << ".point=" << input.point << '\n';
  std::cout << prefix << ".bootstrap_95_low=" << input.low << '\n';
  std::cout << prefix << ".bootstrap_95_high=" << input.high << '\n';
  for (std::size_t seed = 0; seed < input.seed_deltas.size(); ++seed) {
    std::cout << prefix << ".seed_" << kAttributionSeeds[seed]
              << "_delta=" << input.seed_deltas[seed] << '\n';
  }
  for (std::size_t family = 0; family < input.family_deltas.size(); ++family) {
    std::cout << prefix << ".family_" << kFamilyNames[family]
              << "_delta=" << input.family_deltas[family] << '\n';
  }
  std::cout << prefix << ".classification="
            << psm_gate::continuous_classification_name(result.classification)
            << '\n';
}

void psm_emit_order(const std::string &prefix,
                    const psm_gate::OrderInput &input) {
  const auto result = psm_gate::evaluate_order(input);
  std::cout << prefix << ".point=" << input.point << '\n';
  std::cout << prefix << ".bootstrap_95_low=" << input.low << '\n';
  std::cout << prefix << ".bootstrap_95_high=" << input.high << '\n';
  for (std::size_t seed = 0; seed < input.seed_points.size(); ++seed) {
    std::cout << prefix << ".seed_" << kAttributionSeeds[seed]
              << "=" << input.seed_points[seed] << '\n';
  }
  std::cout << prefix << ".classification="
            << psm_gate::order_classification_name(result.classification)
            << '\n';
}

[[nodiscard]] bool psm_close(double observed, double expected) {
  return std::isfinite(observed) && std::abs(observed - expected) <= 1.0e-12;
}

int run_psm_impl(const Options &options, bool &attempt_consumed) {
  if (options.device != "cuda" || !torch::cuda::is_available()) {
    throw std::runtime_error("PSM authoritative map requires CUDA");
  }
  if (options.steps > 0 || options.seeds > 0 || !options.weak_views) {
    throw std::runtime_error(
        "PSM accepts no training-step/seed/weak-view override");
  }
  const char *workspace = std::getenv("CUBLAS_WORKSPACE_CONFIG");
  if (workspace == nullptr || std::string(workspace) != ":4096:8") {
    throw std::runtime_error("PSM requires CUBLAS_WORKSPACE_CONFIG=:4096:8");
  }
  const torch::Device device(torch::kCUDA, 0);
  at::set_num_threads(1);
  at::set_num_interop_threads(1);
  at::globalContext().setDeterministicCuDNN(true);
  at::globalContext().setDeterministicAlgorithms(true, false);
  std::cout << std::setprecision(17) << std::boolalpha;
  std::cout << "schema=wikimyei.mtf_jepa_mae_vicreg.psm.v1\n";
  std::cout << "module_only=true\n";
  std::cout << "device=cuda:0\n";
  std::cout << "model_seeds=17,31,47\n";
  std::cout << "arms=channel,channel_domain,channel_domain_scale,channel_"
               "domain_scale_bin,encoder\n";
  std::cout << "representation_width=96\n";
  std::cout << "optimizer_constructed=false\n";
  std::cout << "optimizer_steps=0\n";
  std::cout << "backward_calls=0\n";
  std::cout << "launcher_augmentation=false\n";
  std::cout << "model_row_batch_size=" << kModelRowBatchSize << '\n';
  std::cout << "probe_sample_ladder=32,64,128,256\n";
  std::cout << "bootstrap_replicates=" << kRssmBootstrapReplicates << '\n';
  std::cout << "bootstrap_seed=" << kRssmBootstrapSeed << '\n';
  std::cout << "psm.attempt.consumption_boundary=first_accepted_row_psm_"
               "probe_fit_or_endpoint\n";

  const auto tokenizer_plan = rssm_tokenizer_plan_receipt();
  const auto token_layout = psm_token_layout_receipt();
  const auto partitions = psm_partition_receipt();
  const auto rssm_projection = rssm_make_token_projection();
  const auto psm_projection = psm_make_projection(rssm_projection);
  const auto projection =
      psm_projection_receipt(rssm_projection, psm_projection);
  std::cout << "psm.tokenizer_plan.total_tokens="
            << tokenizer_plan.total_tokens << '\n';
  std::cout << "psm.tokenizer_plan.clipped_collisions="
            << tokenizer_plan.clipped_full_history_collisions << '\n';
  std::cout << "psm.tokenizer_plan.shorter_tokens="
            << tokenizer_plan.shorter_window_tokens << '\n';
  std::cout << "psm.tokenizer_plan.shorter_tokens_changed="
            << tokenizer_plan.shorter_tokens_changed << '\n';
  std::cout << "psm.tokenizer_plan.pass=" << tokenizer_plan.pass << '\n';
  std::cout << "psm.token_layout.pass=" << token_layout.pass << '\n';
  emit_fingerprint("psm.token_layout.hash", token_layout.layout_hash);
  std::cout << "psm.partition.pass=" << partitions.pass << '\n';
  std::cout << "psm.partition.idempotence_max_abs="
            << partitions.idempotence_max_abs << '\n';
  std::cout << "psm.partition.nesting_max_abs=" << partitions.nesting_max_abs
            << '\n';
  emit_fingerprint("psm.projection.rssm_hash",
                   hash_tensor_stable_bytes(rssm_projection));
  emit_fingerprint("psm.projection.psm_hash",
                   hash_tensor_stable_bytes(psm_projection));
  std::cout << "psm.projection.orthogonality_error="
            << projection.orthogonality_error << '\n';
  std::cout << "psm.projection.contrast_mean_error="
            << projection.contrast_mean_error << '\n';
  std::cout << "psm.projection.block_sum_identity_error="
            << projection.block_sum_identity_error << '\n';
  std::cout << "psm.projection.pass=" << projection.pass << '\n';

  auto normalizer = generate_dataset(0, 256);
  auto probe_train = generate_dataset(1000000, 256);
  auto probe_validation = generate_dataset(2000000, 128);
  auto test = generate_dataset(3000000, 256);
  const std::array<uint64_t, 4> mask_hashes_before{
      hash_tensor_stable_bytes(normalizer.mask),
      hash_tensor_stable_bytes(probe_train.mask),
      hash_tensor_stable_bytes(probe_validation.mask),
      hash_tensor_stable_bytes(test.mask)};
  const std::array<uint64_t, 4> target_hashes_before{
      hash_tensor_stable_bytes(normalizer.target),
      hash_tensor_stable_bytes(probe_train.target),
      hash_tensor_stable_bytes(probe_validation.target),
      hash_tensor_stable_bytes(test.target)};
  rssm_emit_dataset_identity("psm.data.normalizer", normalizer,
                             "unnormalized");
  rssm_emit_dataset_identity("psm.data.probe_train", probe_train,
                             "unnormalized");
  rssm_emit_dataset_identity("psm.data.probe_validation", probe_validation,
                             "unnormalized");
  rssm_emit_dataset_identity("psm.data.test", test, "unnormalized");
  const auto normalization = fit_normalization(normalizer);
  for (Dataset *dataset :
       {&normalizer, &probe_train, &probe_validation, &test}) {
    normalize(*dataset, normalization);
    validate_dataset(*dataset);
  }
  auto reversed_train = rssm_reversed_dataset(probe_train);
  auto reversed_validation = rssm_reversed_dataset(probe_validation);
  auto reversed_test = rssm_reversed_dataset(test);
  rssm_emit_dataset_identity("psm.data.normalizer", normalizer, "normalized");
  rssm_emit_dataset_identity("psm.data.probe_train", probe_train,
                             "normalized");
  rssm_emit_dataset_identity("psm.data.probe_validation", probe_validation,
                             "normalized");
  rssm_emit_dataset_identity("psm.data.test", test, "normalized");
  rssm_emit_dataset_identity("psm.data.reversed_train", reversed_train,
                             "normalized");
  rssm_emit_dataset_identity("psm.data.reversed_validation",
                             reversed_validation, "normalized");
  rssm_emit_dataset_identity("psm.data.reversed_test", reversed_test,
                             "normalized");
  const std::array<const Dataset *, 4> normalized{
      &normalizer, &probe_train, &probe_validation, &test};
  bool normalization_preserved_identity = true;
  for (std::size_t index = 0; index < normalized.size(); ++index) {
    normalization_preserved_identity =
        normalization_preserved_identity &&
        mask_hashes_before[index] ==
            hash_tensor_stable_bytes(normalized[index]->mask) &&
        target_hashes_before[index] ==
            hash_tensor_stable_bytes(normalized[index]->target);
  }
  const bool dataset_identity_exact =
      normalization_preserved_identity && normalizer.group_begin == 0 &&
      normalizer.data.size(0) == 256 && probe_train.group_begin == 1000000 &&
      probe_train.data.size(0) == 256 &&
      probe_validation.group_begin == 2000000 &&
      probe_validation.data.size(0) == 128 && test.group_begin == 3000000 &&
      test.data.size(0) == 256 &&
      rssm_group_pair_exact(probe_train, reversed_train) &&
      rssm_group_pair_exact(probe_validation, reversed_validation) &&
      rssm_group_pair_exact(test, reversed_test);
  std::cout << "psm.data.normalization_preserved_identity="
            << normalization_preserved_identity << '\n';
  std::cout << "psm.data.identity_exact=" << dataset_identity_exact << '\n';
  emit_fingerprint("psm.normalization.mean_hash",
                   hash_tensor_stable_bytes(normalization.mean));
  emit_fingerprint("psm.normalization.inv_std_hash",
                   hash_tensor_stable_bytes(normalization.inv_std));

  const std::array<const Dataset *, kPsmDatasetCount> datasets{
      &probe_train,       &probe_validation, &test,
      &reversed_train,    &reversed_validation,
      &reversed_test};
  const auto shuffle_train =
      rssm_sattolo_permutation(256, kRssmShuffleTrainTag);
  const auto shuffle_validation =
      rssm_sattolo_permutation(128, kRssmShuffleValidationTag);
  const auto shuffle_test =
      rssm_sattolo_permutation(256, kRssmShuffleTestTag);
  const auto shuffled_train_target =
      probe_train.target.index_select(0, shuffle_train).contiguous();
  const auto shuffled_validation_target =
      probe_validation.target.index_select(0, shuffle_validation).contiguous();
  const auto shuffled_test_target =
      test.target.index_select(0, shuffle_test).contiguous();
  const auto order_fit_permutations = rssm_order_fit_permutations();
  const auto order_fit_targets = rssm_order_fit_targets(nullptr);
  const auto shuffled_order_fit_targets =
      rssm_order_fit_targets(&order_fit_permutations);
  const auto order_validation_target = rssm_order_labels(128);
  const auto order_test_target = rssm_order_labels(256);
  const auto order_shuffle_validation =
      rssm_sattolo_permutation(256, kRssmOrderShuffleValidationTag);
  const auto order_shuffle_test =
      rssm_sattolo_permutation(512, kRssmOrderShuffleTestTag);
  const auto shuffled_order_validation_target =
      order_validation_target.index_select(0, order_shuffle_validation)
          .contiguous();
  const auto shuffled_order_test_target =
      order_test_target.index_select(0, order_shuffle_test).contiguous();
  bool order_shuffle_balanced =
      rssm_order_target_balanced(shuffled_order_validation_target) &&
      rssm_order_target_balanced(shuffled_order_test_target);
  for (const auto &target : shuffled_order_fit_targets) {
    order_shuffle_balanced =
        order_shuffle_balanced && rssm_order_target_balanced(target);
  }
  bool permutations_valid =
      rssm_permutation_receipt(shuffle_train).pass &&
      rssm_permutation_receipt(shuffle_validation).pass &&
      rssm_permutation_receipt(shuffle_test).pass &&
      rssm_permutation_receipt(order_shuffle_validation).pass &&
      rssm_permutation_receipt(order_shuffle_test).pass;
  rssm_emit_permutation("psm.shuffle.fit", shuffle_train);
  rssm_emit_permutation("psm.shuffle.validation", shuffle_validation);
  rssm_emit_permutation("psm.shuffle.test", shuffle_test);
  for (std::size_t index = 0; index < order_fit_permutations.size(); ++index) {
    permutations_valid =
        permutations_valid &&
        rssm_permutation_receipt(order_fit_permutations[index]).pass;
    rssm_emit_permutation("psm.order_shuffle.fit.n_" +
                              std::to_string(kRssmSampleLadder[index]),
                          order_fit_permutations[index]);
  }
  rssm_emit_permutation("psm.order_shuffle.validation",
                        order_shuffle_validation);
  rssm_emit_permutation("psm.order_shuffle.test", order_shuffle_test);
  const auto bootstrap_rows = rssm_bootstrap_rows(256);
  const bool bootstrap_valid = rssm_bootstrap_contract(bootstrap_rows, 256);
  const auto ridge = rssm_ridge_equivalence_fixture();
  std::cout << "psm.permutations_valid=" << permutations_valid << '\n';
  std::cout << "psm.order_shuffle_balanced=" << order_shuffle_balanced << '\n';
  emit_fingerprint("psm.bootstrap.table_hash",
                   rssm_tensor_vector_hash(bootstrap_rows));
  std::cout << "psm.bootstrap.valid=" << bootstrap_valid << '\n';
  std::cout << "psm.ridge.pass=" << ridge.pass << '\n';

  PsmMeasurements measurements{};
  std::array<std::array<PsmFeatureSet, kPsmDatasetCount>,
             psm_gate::kSeedCount>
      features_by_seed{};
  bool all_public_exact = true;
  bool all_direct_exact = true;
  bool all_repeated_exact = true;
  bool all_parameters_rng_exact = true;
  bool all_order_exact = true;
  bool all_cardinality_exact = true;
  bool all_capture_layout_exact = true;
  bool all_feature_contracts = true;
  bool cross_seed_token_structure_exact = true;
  bool metadata_plan_exact = true;
  bool metadata_initialized = false;
  uint64_t metadata_reference = 0;
  std::array<uint64_t, kPsmDatasetCount> token_mask_references{};
  double channel_served_max_abs = 0.0;
  double encoder_projection_max_abs = 0.0;

  for (std::size_t seed_index = 0; seed_index < measurements.size();
       ++seed_index) {
    auto &measurement = measurements[seed_index];
    measurement.seed = kAttributionSeeds[seed_index];
    set_paired_rng(measurement.seed, device);
    auto model = mtf::MtfJepaMaeVicreg(
        attribution_config(device, kJmcdArms[kJmcdCombinedIndex]));
    const bool initial_mode = model->is_training();
    const auto parameters_before = snapshot_parameters(model);
    const uint64_t parameter_hash_before =
        rssm_parameter_snapshot_hash(parameters_before);
    const auto generator_before = current_generator_state_snapshot(device);
    for (std::size_t dataset_index = 0; dataset_index < datasets.size();
         ++dataset_index) {
      const auto first = rssm_capture_once(model, *datasets[dataset_index],
                                           device,
                                           /*check_direct_encoder=*/true);
      const auto second = rssm_capture_once(model, *datasets[dataset_index],
                                            device,
                                            /*check_direct_encoder=*/true);
      measurement.public_sandwich_exact =
          measurement.public_sandwich_exact && first.public_sandwich_exact &&
          second.public_sandwich_exact;
      measurement.direct_encoder_exact =
          measurement.direct_encoder_exact && first.direct_encoder_exact &&
          second.direct_encoder_exact;
      measurement.repeated_capture_exact =
          measurement.repeated_capture_exact &&
          rssm_capture_exact(first, second);
      measurement.production_order_exact =
          measurement.production_order_exact && first.production_order_exact &&
          second.production_order_exact;
      measurement.cardinality_exact = measurement.cardinality_exact &&
                                      first.cardinality_exact &&
                                      second.cardinality_exact;
      measurement.token_layout_exact =
          measurement.token_layout_exact &&
          psm_capture_layout_exact(first, token_layout) &&
          psm_capture_layout_exact(second, token_layout);
      if (seed_index == 0) {
        token_mask_references[dataset_index] =
            first.token_mask_structure_hash;
      } else {
        cross_seed_token_structure_exact =
            cross_seed_token_structure_exact &&
            token_mask_references[dataset_index] ==
                first.token_mask_structure_hash;
      }
      if (!metadata_initialized) {
        metadata_reference = first.metadata_structure_hash;
        metadata_initialized = true;
      } else {
        metadata_plan_exact = metadata_plan_exact &&
                              metadata_reference ==
                                  first.metadata_structure_hash;
      }
      auto features =
          psm_feature_set(first, psm_projection, rssm_projection);
      channel_served_max_abs =
          std::max(channel_served_max_abs, features.channel_served_max_abs);
      encoder_projection_max_abs = std::max(
          encoder_projection_max_abs, features.encoder_projection_max_abs);
      measurement.feature_contracts_exact =
          measurement.feature_contracts_exact && features.pass;
      features_by_seed[seed_index][dataset_index] = std::move(features);
      const std::string prefix =
          "psm.seed_" + std::to_string(measurement.seed) + ".capture." +
          kPsmDatasetNames[dataset_index];
      emit_fingerprint(prefix + ".encoder_hash", first.encoder_source_hash);
      emit_fingerprint(prefix + ".served_hash", first.served_source_hash);
      emit_fingerprint(prefix + ".encoder_float64_hash",
                       first.encoder_float64_hash);
      emit_fingerprint(prefix + ".served_float64_hash",
                       first.served_float64_hash);
      emit_fingerprint(prefix + ".token_mask_structure_hash",
                       first.token_mask_structure_hash);
      emit_fingerprint(prefix + ".metadata_structure_hash",
                       first.metadata_structure_hash);
      for (std::size_t arm_index = 0; arm_index < kPsmArmCount; ++arm_index) {
        emit_fingerprint(
            prefix + ".arm." + kPsmArmNames[arm_index] + ".float64_hash",
            hash_tensor_stable_bytes(
                features_by_seed[seed_index][dataset_index]
                    .arms[arm_index]
                    .by_channel));
      }
    }
    const auto generator_after = current_generator_state_snapshot(device);
    const auto parameters_after = snapshot_parameters(model);
    const uint64_t parameter_hash_after =
        rssm_parameter_snapshot_hash(parameters_after);
    measurement.parameters_and_rng_unchanged =
        parameter_max_abs_diff(model, parameters_before) == 0.0 &&
        parameter_hash_before == parameter_hash_after &&
        generator_state_snapshot_equal(generator_before, generator_after) &&
        model->is_training() == initial_mode;
    all_public_exact =
        all_public_exact && measurement.public_sandwich_exact;
    all_direct_exact = all_direct_exact && measurement.direct_encoder_exact;
    all_repeated_exact =
        all_repeated_exact && measurement.repeated_capture_exact;
    all_parameters_rng_exact = all_parameters_rng_exact &&
                               measurement.parameters_and_rng_unchanged;
    all_order_exact = all_order_exact && measurement.production_order_exact;
    all_cardinality_exact =
        all_cardinality_exact && measurement.cardinality_exact;
    all_capture_layout_exact =
        all_capture_layout_exact && measurement.token_layout_exact;
    all_feature_contracts =
        all_feature_contracts && measurement.feature_contracts_exact;
    const std::string prefix =
        "psm.seed_" + std::to_string(measurement.seed);
    std::cout << prefix << ".public_sandwich_exact="
              << measurement.public_sandwich_exact << '\n';
    std::cout << prefix << ".direct_encoder_exact="
              << measurement.direct_encoder_exact << '\n';
    std::cout << prefix << ".repeated_capture_exact="
              << measurement.repeated_capture_exact << '\n';
    std::cout << prefix << ".parameters_and_rng_unchanged="
              << measurement.parameters_and_rng_unchanged << '\n';
    std::cout << prefix << ".production_order_exact="
              << measurement.production_order_exact << '\n';
    std::cout << prefix << ".cardinality_exact="
              << measurement.cardinality_exact << '\n';
    std::cout << prefix << ".token_layout_exact="
              << measurement.token_layout_exact << '\n';
    std::cout << prefix << ".feature_contracts_exact="
              << measurement.feature_contracts_exact << '\n';
    emit_fingerprint(prefix + ".parameter_hash_before", parameter_hash_before);
    emit_fingerprint(prefix + ".parameter_hash_after", parameter_hash_after);
    emit_fingerprint(prefix + ".cpu_generator_hash_before",
                     generator_before.digest.cpu);
    emit_fingerprint(prefix + ".cpu_generator_hash_after",
                     generator_after.digest.cpu);
    emit_fingerprint(prefix + ".cuda_generator_hash_before",
                     generator_before.digest.cuda);
    emit_fingerprint(prefix + ".cuda_generator_hash_after",
                     generator_after.digest.cuda);
  }

  const bool prefit_mechanics =
      tokenizer_plan.pass && token_layout.pass && partitions.pass &&
      projection.pass && dataset_identity_exact && permutations_valid &&
      order_shuffle_balanced && bootstrap_valid && ridge.pass &&
      all_public_exact && all_repeated_exact && all_parameters_rng_exact &&
      all_direct_exact && all_order_exact && all_cardinality_exact &&
      all_capture_layout_exact && all_feature_contracts &&
      cross_seed_token_structure_exact && metadata_plan_exact &&
      channel_served_max_abs <= 1.0e-10 &&
      encoder_projection_max_abs <= 1.0e-12;
  std::cout << "psm.prefit.channel_served_max_abs="
            << channel_served_max_abs << '\n';
  std::cout << "psm.prefit.encoder_projection_max_abs="
            << encoder_projection_max_abs << '\n';
  std::cout << "psm.prefit.cross_seed_token_structure_exact="
            << cross_seed_token_structure_exact << '\n';
  std::cout << "psm.prefit.metadata_plan_exact=" << metadata_plan_exact
            << '\n';
  std::cout << "psm.prefit.mechanics_pass=" << prefit_mechanics << '\n';
  if (!prefit_mechanics) {
    std::cout << "psm.attempt.consumed=false\n";
    std::cout << "training_authorized=false\n";
    std::cout << "long_run_authorized=false\n";
    std::cout << "production_or_end_to_end_authorized=false\n";
    std::cout << "follow_on_repair_authorized=false\n";
    std::cout << "execution_status=psm_prefit_mechanics_failed\n";
    return 3;
  }

  attempt_consumed = true;
  std::cout << "psm.attempt.consumed=true\n";
  for (std::size_t seed_index = 0; seed_index < measurements.size();
       ++seed_index) {
    auto &measurement = measurements[seed_index];
    const auto &features = features_by_seed[seed_index];
    for (std::size_t arm_index = 0; arm_index < kPsmArmCount; ++arm_index) {
      const auto &train = features[kPsmTrainDataset].arms[arm_index].flat;
      const auto &validation =
          features[kPsmValidationDataset].arms[arm_index].flat;
      const auto &test_features =
          features[kPsmTestDataset].arms[arm_index].flat;
      measurement.real[arm_index] = rssm_probe_curve(
          train, validation, test_features, probe_train.target,
          probe_validation.target, test.target, /*dual=*/false);
      measurement.shuffled[arm_index] = rssm_probe_curve(
          train, validation, test_features, shuffled_train_target,
          shuffled_validation_target, shuffled_test_target, /*dual=*/false);
      const auto order_train = rssm_interleave_pairs(
          train, features[kPsmReversedTrainDataset].arms[arm_index].flat);
      const auto order_validation_features = rssm_interleave_pairs(
          validation,
          features[kPsmReversedValidationDataset].arms[arm_index].flat);
      const auto order_test_features = rssm_interleave_pairs(
          test_features,
          features[kPsmReversedTestDataset].arms[arm_index].flat);
      measurement.order[arm_index] = rssm_order_curve(
          order_train, order_validation_features, order_test_features,
          order_fit_targets, order_validation_target, order_test_target,
          /*dual=*/false);
      measurement.order_shuffled[arm_index] = rssm_order_curve(
          order_train, order_validation_features, order_test_features,
          shuffled_order_fit_targets, shuffled_order_validation_target,
          shuffled_order_test_target, /*dual=*/false);
      validate_probe_curve_finite(measurement.real[arm_index],
                                  "PSM real probe");
      validate_probe_curve_finite(measurement.shuffled[arm_index],
                                  "PSM shuffled probe");
      rssm_validate_order_curve_finite(measurement.order[arm_index],
                                       "PSM order probe");
      rssm_validate_order_curve_finite(measurement.order_shuffled[arm_index],
                                       "PSM shuffled order probe");
      const std::string prefix =
          "psm.seed_" + std::to_string(measurement.seed) + ".arm." +
          kPsmArmNames[arm_index];
      rssm_emit_probe_curve(prefix + ".probe", measurement.real[arm_index]);
      rssm_emit_probe_curve(prefix + ".shuffled_probe",
                            measurement.shuffled[arm_index]);
      rssm_emit_order_curve(prefix + ".order_probe",
                            measurement.order[arm_index]);
      rssm_emit_order_curve(prefix + ".order_shuffled_probe",
                            measurement.order_shuffled[arm_index]);
    }
    const auto &reference_train =
        features[kPsmTrainDataset].rssm_encoder_reference_flat;
    const auto &reference_validation =
        features[kPsmValidationDataset].rssm_encoder_reference_flat;
    const auto &reference_test =
        features[kPsmTestDataset].rssm_encoder_reference_flat;
    measurement.rssm_encoder_reference = rssm_probe_curve(
        reference_train, reference_validation, reference_test,
        probe_train.target, probe_validation.target, test.target,
        /*dual=*/false);
    measurement.rssm_encoder_order_reference = rssm_order_curve(
        rssm_interleave_pairs(
            reference_train,
            features[kPsmReversedTrainDataset].rssm_encoder_reference_flat),
        rssm_interleave_pairs(
            reference_validation,
            features[kPsmReversedValidationDataset]
                .rssm_encoder_reference_flat),
        rssm_interleave_pairs(
            reference_test,
            features[kPsmReversedTestDataset].rssm_encoder_reference_flat),
        order_fit_targets, order_validation_target, order_test_target,
        /*dual=*/false);
    validate_probe_curve_finite(measurement.rssm_encoder_reference,
                                "PSM RSSM encoder reference");
    rssm_validate_order_curve_finite(
        measurement.rssm_encoder_order_reference,
        "PSM RSSM encoder order reference");
    const std::string reference_prefix =
        "psm.seed_" + std::to_string(measurement.seed) +
        ".reference.rssm_encoder";
    rssm_emit_probe_curve(reference_prefix + ".probe",
                          measurement.rssm_encoder_reference);
    rssm_emit_order_curve(reference_prefix + ".order_probe",
                          measurement.rssm_encoder_order_reference);
  }

  constexpr std::array<double, psm_gate::kSeedCount> kExpectedChannelAulc{
      0.51029806802386968, 0.5121433689059538, 0.53534605970626181};
  constexpr std::array<double, psm_gate::kSeedCount> kExpectedEncoderAulc{
      0.58626145257333262, 0.56999408500250559, 0.58033945194633074};
  bool references_reproduced = projection.rssm_projection_hash_exact;
  double channel_order_mean = 0.0;
  double encoder_order_mean = 0.0;
  for (std::size_t seed = 0; seed < measurements.size(); ++seed) {
    const double channel =
        measurements[seed].real[psm_index(PsmArm::channel)].area;
    const double encoder = measurements[seed].rssm_encoder_reference.area;
    const bool channel_exact = psm_close(channel, kExpectedChannelAulc[seed]);
    const bool encoder_exact = psm_close(encoder, kExpectedEncoderAulc[seed]);
    references_reproduced =
        references_reproduced && channel_exact && encoder_exact;
    channel_order_mean +=
        measurements[seed].order[psm_index(PsmArm::channel)].area;
    encoder_order_mean +=
        measurements[seed].rssm_encoder_order_reference.area;
    const std::string prefix =
        "psm.reference.seed_" + std::to_string(kAttributionSeeds[seed]);
    std::cout << prefix << ".channel_aulc=" << channel << '\n';
    std::cout << prefix << ".channel_aulc_exact=" << channel_exact << '\n';
    std::cout << prefix << ".rssm_encoder_aulc=" << encoder << '\n';
    std::cout << prefix << ".rssm_encoder_aulc_exact=" << encoder_exact
              << '\n';
  }
  channel_order_mean /= static_cast<double>(measurements.size());
  encoder_order_mean /= static_cast<double>(measurements.size());
  const bool channel_order_exact =
      psm_close(channel_order_mean, 0.57454427083333337);
  const bool encoder_order_exact =
      psm_close(encoder_order_mean, 0.9560546875);
  references_reproduced = references_reproduced && channel_order_exact &&
                          encoder_order_exact;
  std::cout << "psm.reference.channel_order_mean=" << channel_order_mean
            << '\n';
  std::cout << "psm.reference.channel_order_exact=" << channel_order_exact
            << '\n';
  std::cout << "psm.reference.rssm_encoder_order_mean=" << encoder_order_mean
            << '\n';
  std::cout << "psm.reference.rssm_encoder_order_exact="
            << encoder_order_exact << '\n';
  std::cout << "psm.reference.all_exact=" << references_reproduced << '\n';

  bool continuous_shuffle_pass = true;
  bool order_shuffle_pass = true;
  std::array<psm_gate::OrderInput, kPsmArmCount> order_inputs{};
  for (std::size_t arm_index = 0; arm_index < kPsmArmCount; ++arm_index) {
    const auto arm = static_cast<PsmArm>(arm_index);
    const auto real = psm_curves_for(measurements, arm, /*shuffled=*/false);
    const auto shuffled =
        psm_curves_for(measurements, arm, /*shuffled=*/true);
    const auto real_summary =
        rssm_curve_interval(real, test.target, bootstrap_rows);
    const auto shuffled_summary =
        rssm_curve_interval(shuffled, shuffled_test_target, bootstrap_rows);
    const auto order =
        psm_order_curves_for(measurements, arm, /*shuffled=*/false);
    const auto shuffled_order =
        psm_order_curves_for(measurements, arm, /*shuffled=*/true);
    order_inputs[arm_index] =
        psm_order_input(order, order_test_target, bootstrap_rows);
    const auto shuffled_order_input = psm_order_input(
        shuffled_order, shuffled_order_test_target, bootstrap_rows);
    const bool arm_continuous_shuffle =
        shuffled_summary.point <= 0.02 &&
        shuffled_summary.interval.high <= 0.05;
    const bool arm_order_shuffle =
        shuffled_order_input.point <= 0.55 &&
        shuffled_order_input.high <= 0.60;
    continuous_shuffle_pass =
        continuous_shuffle_pass && arm_continuous_shuffle;
    order_shuffle_pass = order_shuffle_pass && arm_order_shuffle;
    const std::string prefix =
        "psm.summary.arm." + std::string(kPsmArmNames[arm_index]);
    std::cout << prefix << ".aulc.point=" << real_summary.point << '\n';
    std::cout << prefix << ".aulc.bootstrap_95_low="
              << real_summary.interval.low << '\n';
    std::cout << prefix << ".aulc.bootstrap_95_high="
              << real_summary.interval.high << '\n';
    std::array<double, kFamilies> family_mean{};
    for (std::size_t seed = 0; seed < real.size(); ++seed) {
      std::cout << prefix << ".aulc.seed_" << kAttributionSeeds[seed] << "="
                << real[seed].area << '\n';
      const auto families = rssm_family_areas(real[seed]);
      for (std::size_t family = 0; family < family_mean.size(); ++family) {
        family_mean[family] += families[family];
      }
    }
    for (std::size_t family = 0; family < family_mean.size(); ++family) {
      family_mean[family] /= static_cast<double>(real.size());
      std::cout << prefix << ".family_" << kFamilyNames[family] << "_aulc="
                << family_mean[family] << '\n';
    }
    std::cout << prefix << ".shuffled_aulc.point="
              << shuffled_summary.point << '\n';
    std::cout << prefix << ".shuffled_aulc.bootstrap_95_low="
              << shuffled_summary.interval.low << '\n';
    std::cout << prefix << ".shuffled_aulc.bootstrap_95_high="
              << shuffled_summary.interval.high << '\n';
    std::cout << prefix << ".continuous_shuffle_pass="
              << arm_continuous_shuffle << '\n';
    psm_emit_order(prefix + ".order", order_inputs[arm_index]);
    psm_emit_order(prefix + ".order_shuffled", shuffled_order_input);
    std::cout << prefix << ".order_shuffle_pass=" << arm_order_shuffle
              << '\n';
  }

  psm_gate::GateInput gate_input{};
  const auto channel_curves =
      psm_curves_for(measurements, PsmArm::channel, /*shuffled=*/false);
  const auto encoder_curves =
      psm_curves_for(measurements, PsmArm::encoder, /*shuffled=*/false);
  gate_input.encoder_minus_channel = psm_continuous_input(rssm_contrast(
      encoder_curves, test.target, channel_curves, test.target,
      bootstrap_rows));
  gate_input.encoder_order = order_inputs[psm_index(PsmArm::encoder)];
  gate_input.channel_order = order_inputs[psm_index(PsmArm::channel)];
  constexpr std::array<PsmArm, psm_gate::kCandidateCount> candidate_arms{
      PsmArm::channel_domain, PsmArm::channel_domain_scale,
      PsmArm::channel_domain_scale_bin};
  for (std::size_t candidate = 0; candidate < candidate_arms.size();
       ++candidate) {
    const auto curves = psm_curves_for(measurements, candidate_arms[candidate],
                                       /*shuffled=*/false);
    gate_input.candidates[candidate].minus_encoder = psm_continuous_input(
        rssm_contrast(curves, test.target, encoder_curves, test.target,
                      bootstrap_rows));
    gate_input.candidates[candidate].minus_channel = psm_continuous_input(
        rssm_contrast(curves, test.target, channel_curves, test.target,
                      bootstrap_rows));
    gate_input.candidates[candidate].order =
        order_inputs[psm_index(candidate_arms[candidate])];
  }
  gate_input.validity = {
      .no_training_or_end_to_end = true,
      .capture_and_identity_exact =
          dataset_identity_exact && all_public_exact && all_repeated_exact &&
          all_direct_exact && all_order_exact && all_cardinality_exact &&
          all_capture_layout_exact && cross_seed_token_structure_exact &&
          metadata_plan_exact,
      .parameters_and_rng_unchanged = all_parameters_rng_exact,
      .partitions_valid = partitions.pass && all_feature_contracts,
      .projection_valid = projection.pass,
      .deterministic_tables_valid =
          permutations_valid && order_shuffle_balanced && bootstrap_valid &&
          ridge.pass,
      .references_reproduced = references_reproduced,
      .shuffled_controls_pass =
          continuous_shuffle_pass && order_shuffle_pass};
  const auto gate = psm_gate::evaluate(gate_input);

  psm_emit_continuous("psm.summary.contrast.encoder_minus_channel",
                      gate_input.encoder_minus_channel);
  for (std::size_t candidate = 0; candidate < candidate_arms.size();
       ++candidate) {
    const std::string prefix =
        "psm.summary.contrast." +
        std::string(kPsmArmNames[psm_index(candidate_arms[candidate])]);
    psm_emit_continuous(prefix + "_minus_encoder",
                        gate_input.candidates[candidate].minus_encoder);
    psm_emit_continuous(prefix + "_minus_channel",
                        gate_input.candidates[candidate].minus_channel);
  }
  std::cout << "psm.summary.validity.numeric_inputs="
            << gate.numeric_inputs_valid << '\n';
  std::cout << "psm.summary.validity.capture_and_identity_exact="
            << gate_input.validity.capture_and_identity_exact << '\n';
  std::cout << "psm.summary.validity.parameters_and_rng_unchanged="
            << gate_input.validity.parameters_and_rng_unchanged << '\n';
  std::cout << "psm.summary.validity.partitions_valid="
            << gate_input.validity.partitions_valid << '\n';
  std::cout << "psm.summary.validity.projection_valid="
            << gate_input.validity.projection_valid << '\n';
  std::cout << "psm.summary.validity.deterministic_tables_valid="
            << gate_input.validity.deterministic_tables_valid << '\n';
  std::cout << "psm.summary.validity.references_reproduced="
            << gate_input.validity.references_reproduced << '\n';
  std::cout << "psm.summary.validity.continuous_shuffle_pass="
            << continuous_shuffle_pass << '\n';
  std::cout << "psm.summary.validity.order_shuffle_pass="
            << order_shuffle_pass << '\n';
  std::cout << "psm.summary.validity.mechanics_valid=" << gate.mechanics_valid
            << '\n';
  std::cout << "psm.summary.gate.boundary_reproduced="
            << gate.boundary_reproduced << '\n';
  std::cout << "psm.summary.gate.classification="
            << psm_gate::terminal_classification_name(gate.classification)
            << '\n';
  std::cout << "psm.summary.gate.reference_audit_pending=true\n";
  std::cout << "training_authorized=false\n";
  std::cout << "long_run_authorized=false\n";
  std::cout << "production_or_end_to_end_authorized=false\n";
  std::cout << "follow_on_repair_authorized=false\n";
  const bool valid_science = gate.numeric_inputs_valid &&
                             gate.mechanics_valid && references_reproduced &&
                             continuous_shuffle_pass && order_shuffle_pass;
  std::cout << "execution_status=psm_measurements_complete\n";
  return valid_science ? 0 : 3;
}

int run_psm(const Options &options) {
  bool attempt_consumed = false;
  try {
    return run_psm_impl(options, attempt_consumed);
  } catch (...) {
    if (!attempt_consumed) {
      std::cout << std::boolalpha;
      std::cout << "psm.attempt.consumed=false\n";
      std::cout << "training_authorized=false\n";
      std::cout << "long_run_authorized=false\n";
      std::cout << "production_or_end_to_end_authorized=false\n";
      std::cout << "follow_on_repair_authorized=false\n";
      std::cout << "execution_status=psm_prefit_exception\n";
    }
    throw;
  }
}

} // namespace

int main(int argc, char **argv) {
  try {
    const auto options = parse_options(argc, argv);
    if (options.experiment == "quality") {
      return run(options);
    }
    if (options.experiment == "objective-mask-attribution" ||
        options.experiment == "vicreg-variance-component-necessity" ||
        options.experiment == "outer-augmentation-representation-training" ||
        options.experiment == "jepa-mae-core-decomposition") {
      return run_objective_mask_attribution(options);
    }
    if (options.experiment == "outer-augmentation-training-preflight") {
      return run_outer_augmentation_preflight(options);
    }
    if (options.experiment == "jepa-mae-core-decomposition-preflight") {
      return run_jmcd_preflight(options);
    }
    if (options.experiment ==
        "representation-surface-sufficiency-map-preflight") {
      return run_rssm_preflight(options);
    }
    if (options.experiment == "representation-surface-sufficiency-map") {
      return run_rssm(options);
    }
    if (options.experiment == "pooling-structure-mechanism-map-preflight") {
      return run_psm_preflight(options);
    }
    if (options.experiment == "pooling-structure-mechanism-map") {
      return run_psm(options);
    }
    throw std::runtime_error(
        "--experiment must be quality, objective-mask-attribution, "
        "vicreg-variance-component-necessity, outer-augmentation-training-"
        "preflight, outer-augmentation-representation-training, jepa-mae-"
        "core-decomposition-preflight, jepa-mae-core-decomposition, or "
        "representation-surface-sufficiency-map-preflight, representation-"
        "surface-sufficiency-map, pooling-structure-mechanism-map-preflight, "
        "or pooling-structure-mechanism-map");
  } catch (const c10::Error &error) {
    std::cerr << "representation_quality_error="
              << error.what_without_backtrace() << '\n';
  } catch (const std::exception &error) {
    std::cerr << "representation_quality_error=" << error.what() << '\n';
  }
  return 2;
}
