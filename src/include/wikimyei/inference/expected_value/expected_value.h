/* expected_value.h */
#pragma once
/**
 * @file expected_value.h
 * @brief MDN-backed expected-value wrapper exposing E[target|X] in configured
 *        target space (API + template methods).
 *
 * Responsibilities
 *  - Own an MDN (semantic_model) that models p(target|X).
 *  - Expose the conditional expectation E[target|X] in target space. For
 *    source-side log-return price targets, this is expected log-return, not
 *    native-price expectation.
 *  - Train with MDN NLL + optional per-channel EMA reweighting +
 * horizon/feature weights.
 *  - Provide telemetry (per-channel / per-horizon NLL).
 *  - Strict checkpoint format v4 (safe state-dict + required metadata).
 *
 * Loader concept (for the template methods):
 *   sample_batch.encoding         : Tensor [B,De] or [B,T',De]
 *   sample_batch.mask             : Tensor [B,C,T] or [B,T] when encoding is a
 *                                   temporal sequence and the reducer requires
 *                                   valid-step semantics
 *   sample_batch.future_features  : Tensor [B,C,Hf,D]
 *   sample_batch.future_mask      : Tensor [B,C,Hf] (0=ignore, 1=valid)
 * The template methods only rely on these members and .to(...) being available.
 *
 * Shapes
 *  - future_features: [B, C, Hf, D]
 *  - selected targets: [B, C, Hf, Dy]  (from target_dims_)
 *  - encoding:         [B, De] or [B, T', De] (reduced by temporal_reducer_)
 *  - MDN outputs:      log_pi [B,C,Hf,K], mu/sigma [B,C,Hf,K,Dy]
 */

#include <torch/serialize.h>
#include <torch/torch.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// Project headers
#include "piaabo/dconfig.h"
#include "piaabo/dutils.h"
#include "piaabo/torch_compat/network_analytics.h"

#include "wikimyei/inference/mdn/mixture_density_network.h"
#include "wikimyei/inference/mdn/mixture_density_network_backbone.h"
#include "wikimyei/inference/mdn/mixture_density_network_head.h"
#include "wikimyei/inference/mdn/mixture_density_network_loss.h"
#include "wikimyei/inference/mdn/mixture_density_network_types.h"
#include "wikimyei/inference/mdn/mixture_density_network_utils.h"

#include "jkimyei/optim/optimizer_utils.h"

#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "camahjucunu/dsl/observation_pipeline/observation_spec.h"
#include "jkimyei/training_setup/jk_setup.h"

DEV_WARNING("(expected_value.h)[] this distribution allows: volatily is just a "
            "function, entropy is just a function.");
DEV_WARNING("(expected_value.h)[] this distribution allows: hamilton jacobi "
            "bellman equation optimization of porfolio.");

namespace cuwacunu {
namespace wikimyei {

class ExpectedValue {
public:
  cuwacunu::iitepi::contract_hash_t contract_hash;
  // -------- training wiring --------
  std::string component_name;

  // -------- target selection --------
  std::vector<int64_t> target_dims_;

  // -------- optional static weights --------
  std::vector<float> static_channel_weights_; // size C
  std::vector<float> static_feature_weights_; // size Dy

  // -------- model (architecture) --------
  cuwacunu::wikimyei::mdn::MdnModel semantic_model{
      nullptr}; // NOTE: initialize to nullptr

  // -------- horizon weighting policy --------
  enum class HorizonPolicy { Uniform, NearTerm, VeryNearTerm };
  HorizonPolicy horizon_policy_{HorizonPolicy::Uniform};
  float gamma_near_{0.95f};
  float gamma_very_{0.80f};

  // -------- encoding temporal reduction policy --------
  enum class TemporalReducer { LastValid, MaskedMean, Mean };
  TemporalReducer temporal_reducer_{TemporalReducer::LastValid};

  // -------- optional dynamic channel EMA weighting --------
  bool use_channel_ema_weights_{false};
  double ema_alpha_{0.95};
  torch::Tensor channel_ema_; // [C], device float

  struct training_policy_t {
    int accumulate_steps{1};
    double clip_norm{0.0};
    double clip_value{0.0};
    bool skip_on_nan{true};
    bool zero_grad_set_to_none{true};
  };

  struct runtime_state_t {
    int optimizer_steps{0};
    int accum_counter{0};
    bool has_pending_grad{false};
    std::uint64_t trained_samples{0};
    std::uint64_t skipped_batches{0};
    std::uint64_t pending_sample_count{0};
    double pending_loss_sum{0.0};
    int pending_loss_count{0};
    double last_committed_loss_mean{0.0};
  };

  struct train_step_result_t {
    torch::Tensor loss{};
    bool skipped{false};
    bool optimizer_step_applied{false};
  };

  training_policy_t training_policy{};
  runtime_state_t runtime_state{};

  // -------- Training plumbing --------
  std::vector<at::Tensor> trainable_params_;
  std::unique_ptr<torch::optim::Optimizer> optimizer;
  std::unique_ptr<cuwacunu::jkimyei::LRSchedulerAny> lr_sched;
  std::unique_ptr<cuwacunu::wikimyei::mdn::MdnNLLLoss> loss_obj;

  double grad_clip{1.0};
  // Adam/AdamW step-counter clamp threshold applied post optimizer step (-1
  // disables).
  int optimizer_threshold_reset{-1};

  // -------- Telemetry / checkpoint bookkeeping --------
  int64_t total_iters_trained_{0};
  int64_t total_epochs_trained_{0};
  int64_t scheduler_batch_steps_{0};
  int64_t scheduler_epoch_steps_{0};
  int telemetry_every_{0}; // 0 = disabled

  // Best metric tracking (persisted in checkpoints)
  double best_metric_{std::numeric_limits<double>::infinity()};
  int best_epoch_{-1};

  // Latest telemetry (for dashboards)
  torch::Tensor
      last_per_channel_nll_; // [C] on device; accessors return CPU clones
  torch::Tensor last_per_horizon_nll_; // [Hf]

public:
  // ---------- ctor ----------
  explicit ExpectedValue(
      const cuwacunu::iitepi::contract_hash_t &contract_hash_,
      const std::string &component_name_);

  // ---------- basic API ----------
  /// Note: these always reflect the underlying MdnModel.
  [[nodiscard]] inline const torch::Device &device() const {
    return semantic_model->device;
  }
  [[nodiscard]] inline torch::Dtype dtype() const {
    return semantic_model->dtype;
  }
  [[nodiscard]] inline int64_t Dy() const { return semantic_model->Dy; }
  [[nodiscard]] inline int64_t K() const { return semantic_model->K; }

  inline cuwacunu::wikimyei::mdn::MdnOut forward(const torch::Tensor &x) {
    return semantic_model->forward(x);
  }
  cuwacunu::wikimyei::mdn::MdnOut
  forward_from_encoding(const torch::Tensor &encoding);
  cuwacunu::wikimyei::mdn::MdnOut
  forward_from_encoding(const torch::Tensor &encoding,
                        const torch::Tensor &source_mask);
  // Expected value == E[target|X] in target space from the MDN output.
  inline torch::Tensor
  expected_value_from_out(const cuwacunu::wikimyei::mdn::MdnOut &out) const {
    return cuwacunu::wikimyei::mdn::mdn_expectation(out);
  }
  // Convenience: compute E[target|X] directly from the encoded input.
  torch::Tensor
  predict_expected_value_from_encoding(const torch::Tensor &encoding);
  torch::Tensor
  predict_expected_value_from_encoding(const torch::Tensor &encoding,
                                       const torch::Tensor &source_mask);

  // ---------- knobs ----------
  void set_horizon_policy(HorizonPolicy p) { horizon_policy_ = p; }
  void set_horizon_gammas(float near_gamma, float very_gamma) {
    gamma_near_ = near_gamma;
    gamma_very_ = very_gamma;
  }
  void set_static_channel_weights(std::vector<float> w) {
    static_channel_weights_ = std::move(w);
  }
  void set_static_feature_weights(std::vector<float> w) {
    static_feature_weights_ = std::move(w);
  }
  void enable_channel_ema_weights(bool on = true, double alpha = 0.95) {
    use_channel_ema_weights_ = on;
    ema_alpha_ = alpha;
  }
  void set_telemetry_every(int N) { telemetry_every_ = std::max(0, N); }

  // ---------- telemetry accessors ----------
  torch::Tensor get_last_per_channel_nll(bool to_cpu = true) const {
    if (!last_per_channel_nll_.defined())
      return torch::Tensor();
    return to_cpu ? last_per_channel_nll_.detach().to(torch::kCPU).clone()
                  : last_per_channel_nll_.detach().clone();
  }
  torch::Tensor get_last_per_horizon_nll(bool to_cpu = true) const {
    if (!last_per_horizon_nll_.defined())
      return torch::Tensor();
    return to_cpu ? last_per_horizon_nll_.detach().to(torch::kCPU).clone()
                  : last_per_horizon_nll_.detach().clone();
  }

  // ---------- helpers: targets & weights ----------
  static torch::Tensor select_targets(const torch::Tensor &future_features,
                                      const std::vector<int64_t> &target_dims);

  torch::Tensor build_horizon_weights(int64_t Hf, torch::Device dev,
                                      torch::Dtype dt) const;
  torch::Tensor build_channel_weights(int64_t C, torch::Device dev,
                                      torch::Dtype dt);
  torch::Tensor build_feature_weights(int64_t Dy, torch::Device dev,
                                      torch::Dtype dt) const;

  torch::Tensor channel_weights_from_ema(int64_t C);
  void update_channel_ema(const torch::Tensor &ch_mean_loss);

  // ---------- training ----------
  train_step_result_t
  train_one_batch(const torch::Tensor &encoding,
                  const torch::Tensor &future_features,
                  const torch::Tensor &future_mask,
                  const torch::Tensor &source_mask = torch::Tensor());
  bool finalize_pending_training_step();
  void reset_runtime_training_state();

  template <typename Loader>
  std::vector<std::pair<int, double>> fit(Loader &dataloader, int n_epochs = -1,
                                          int n_iters = -1,
                                          bool verbose = false);

  // ---------- evaluation ----------
  template <typename Loader>
  double evaluate_nll(Loader &dataloader, bool verbose = false);

  // ---------- checkpointing (strict format v4, no backward compatibility)
  // ----------
  bool save_checkpoint(const std::string &path,
                       bool write_network_analytics_sidecar = false) const;
  bool load_checkpoint(const std::string &path);

private:
  // ---------- archive helpers ----------
  static bool ar_try_read_tensor(torch::serialize::InputArchive &ar,
                                 const char *key, torch::Tensor &out);
  void display_model(bool display_semantic) const;
  void load_jkimyei_training_policy();
  void clear_pending_runtime_state_(bool reset_optimizer_steps);
  std::vector<torch::Tensor> params_with_grad_() const;
  void apply_gradient_clipping_(
      const std::vector<torch::Tensor> &params_with_grad) const;
  bool
  grads_are_finite_(const std::vector<torch::Tensor> &params_with_grad) const;
  bool commit_pending_training_step_();
  torch::Tensor reduce_encoding_(const torch::Tensor &encoding,
                                 const torch::Tensor &source_mask) const;
  torch::Tensor encoding_sequence_mask_from_source_mask_(
      const torch::Tensor &source_mask, int64_t batch_size,
      int64_t sequence_length, torch::Device dev) const;
  void validate_future_feature_width_(const torch::Tensor &future_features,
                                      const char *caller) const;
  static const char *temporal_reducer_name_(TemporalReducer reducer);
  static TemporalReducer parse_temporal_reducer_(std::string token);

  // Best-effort scheduler (de)serialization detection via SFINAE.
  template <typename T, typename = void> struct has_save : std::false_type {};
  template <typename T>
  struct has_save<T, std::void_t<decltype(std::declval<T &>().save(
                         std::declval<torch::serialize::OutputArchive &>()))>>
      : std::true_type {};

  template <typename T, typename = void> struct has_load : std::false_type {};
  template <typename T>
  struct has_load<T, std::void_t<decltype(std::declval<T &>().load(
                         std::declval<torch::serialize::InputArchive &>()))>>
      : std::true_type {};

  template <typename S>
  static bool try_save_scheduler(S &sched,
                                 torch::serialize::OutputArchive &ar) {
    if constexpr (has_save<S>::value) {
      sched.save(ar);
      return true;
    }
    return false;
  }
  template <typename S>
  static bool try_load_scheduler(S &sched, torch::serialize::InputArchive &ar) {
    if constexpr (has_load<S>::value) {
      sched.load(ar);
      return true;
    }
    return false;
  }

  static const char *
  scheduler_mode_name(cuwacunu::jkimyei::LRSchedulerAny::Mode mode);
};

// ======================================================================
// Template implementations
// ======================================================================

template <typename Loader>
std::vector<std::pair<int, double>>
ExpectedValue::fit(Loader &dataloader, int n_epochs, int n_iters,
                   bool verbose) {
  using Clock = std::chrono::steady_clock;
  const auto t0 = Clock::now();

  int epoch_count = 0;
  int iter_count = 0;
  bool stop_loop = false;
  std::vector<std::pair<int, double>> loss_log;

  semantic_model->train();

  while (!stop_loop) {
    if (n_epochs >= 0 && epoch_count >= n_epochs)
      break;

    double cum_loss = 0.0;
    int epoch_iters = 0;

    for (const auto &sample_batch : dataloader) {
      if (n_iters >= 0 && iter_count >= n_iters) {
        stop_loop = true;
        break;
      }

      TORCH_CHECK(sample_batch.encoding.defined(),
                  "[ExpectedValue::fit] encoding is undefined in batch.");
      TORCH_CHECK(sample_batch.future_features.defined() &&
                      sample_batch.future_mask.defined(),
                  "[ExpectedValue::fit] future_features and future_mask must "
                  "be defined.");

      const auto step =
          train_one_batch(sample_batch.encoding, sample_batch.future_features,
                          sample_batch.future_mask, sample_batch.mask);
      ++iter_count;
      if (step.skipped || !step.loss.defined()) {
        continue;
      }

      const double lval = step.loss.item().toDouble();
      cum_loss += lval;
      ++epoch_iters;

      loss_log.emplace_back(iter_count, lval);
    } // dataloader

    (void)finalize_pending_training_step();

    const double epoch_loss = (epoch_iters > 0)
                                  ? (cum_loss / epoch_iters)
                                  : std::numeric_limits<double>::quiet_NaN();
    if (std::isfinite(epoch_loss) && epoch_loss < best_metric_) {
      best_metric_ = epoch_loss;
      best_epoch_ = epoch_count;
    }

    if (lr_sched && epoch_iters > 0) {
      const auto mode = lr_sched->mode;
      if (mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerEpoch) {
        lr_sched->step();
        ++scheduler_epoch_steps_;
      } else if (mode ==
                 cuwacunu::jkimyei::LRSchedulerAny::Mode::PerEpochWithMetric) {
        const double metric =
            std::isfinite(epoch_loss) ? epoch_loss : best_metric_;
        lr_sched->step(metric);
        ++scheduler_epoch_steps_;
      }
    }

    const double lr = cuwacunu::wikimyei::mdn::get_lr_generic(*optimizer);

    ++epoch_count;
    ++total_epochs_trained_;

    if (epoch_iters > 0) {
      const auto analytics =
          cuwacunu::piaabo::torch_compat::summarize_module_network_analytics(
              *semantic_model);
      log_info(
          "[ExpectedValue::network_analytics] epoch=%d finite=%.6f std=%.6f "
          "near_zero=%.6f entropy=%.6f tensor_cv=%.6f max_abs=%.6f\n",
          epoch_count, analytics.finite_ratio, analytics.stddev,
          analytics.near_zero_ratio, analytics.abs_energy_entropy,
          analytics.tensor_rms_cv, analytics.max_abs);
    }

    if (verbose && (epoch_count % 50 == 0 || epoch_count == 1 ||
                    epoch_count >= n_epochs)) {
      log_info("[ExpectedValue::fit] "
               "epoch=%d\titers=%d\tavg_loss=%.6f\tbest=%.6f.at:%d\tlr=%.3g\n",
               epoch_count, epoch_iters, epoch_loss, best_metric_, best_epoch_,
               lr);
    }
  } // while

  const auto t1 = Clock::now();
  const auto ms =
      std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  log_info("%s[ExpectedValue::fit]%s done, iters=%lld, best=%.6f.at:%d, "
           "wall_ms=%lld\n",
           ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET,
           static_cast<long long>(total_iters_trained_), best_metric_,
           best_epoch_, static_cast<long long>(ms));
  return loss_log;
}

template <typename Loader>
double ExpectedValue::evaluate_nll(Loader &dataloader, bool verbose) {
  torch::NoGradGuard ng;
  semantic_model->eval();

  double cum = 0.0;
  int n = 0;

  for (const auto &sample_batch : dataloader) {
    TORCH_CHECK(sample_batch.encoding.defined(),
                "[ExpectedValue::evaluate_nll] encoding undefined");
    TORCH_CHECK(
        sample_batch.future_features.defined() &&
            sample_batch.future_mask.defined(),
        "[ExpectedValue::evaluate_nll] future_features/future_mask undefined");

    auto enc = sample_batch.encoding.to(semantic_model->device,
                                        semantic_model->dtype, true, false);
    auto smask = sample_batch.mask.defined()
                     ? sample_batch.mask.to(semantic_model->device,
                                            torch::kFloat32, true, false)
                     : torch::Tensor();

    auto fut = sample_batch.future_features.to(semantic_model->device,
                                               semantic_model->dtype, true,
                                               false); // [B,C,Hf,D]
    auto fmask = sample_batch.future_mask.to(
        semantic_model->device, torch::kFloat32, true, false); // [B,C,Hf]

    validate_future_feature_width_(fut, "[ExpectedValue::evaluate_nll]");
    auto y = select_targets(fut, target_dims_);
    auto o = forward_from_encoding(enc, smask);

    const int64_t C = fmask.size(1);
    const int64_t Hf = fmask.size(2);
    const int64_t Dy = y.size(3);

    auto w_ch =
        build_channel_weights(C, semantic_model->device, semantic_model->dtype);
    auto w_tau = build_horizon_weights(Hf, semantic_model->device,
                                       semantic_model->dtype);
    auto w_dim = build_feature_weights(Dy, semantic_model->device,
                                       semantic_model->dtype);

    auto ls = loss_obj->compute(o, y, fmask, w_ch, w_tau, w_dim); // scalar

    const double lval = ls.item().toDouble();
    cum += lval;
    ++n;
  }

  const double avg =
      (n > 0) ? (cum / n) : std::numeric_limits<double>::quiet_NaN();
  if (verbose) {
    log_info("%s[ExpectedValue::eval]%s N=%d avg_nll=%.6f\n",
             ANSI_COLOR_Magenta, ANSI_COLOR_RESET, n, avg);
  }
  return avg;
}

} // namespace wikimyei
} // namespace cuwacunu
