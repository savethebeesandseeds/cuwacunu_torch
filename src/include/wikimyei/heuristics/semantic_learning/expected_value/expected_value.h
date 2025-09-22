/* expected_value.h */
#pragma once
/**
 * @file expected_value.h
 * @brief MDN-based expected value estimator (API + template methods).
 *
 * Responsibilities
 *  - Own an MDN (semantic_model) to model p(y|x) and produce expectations.
 *  - Train with MDN NLL + optional per-channel EMA reweighting + horizon/feature weights.
 *  - Provide telemetry (per-channel / per-horizon NLL).
 *  - Robust checkpointing implemented in .cpp via safe state-dict (no JIT pickler).
 *
 * Loader concept (for the template methods):
 *   sample_batch.encoding         : Tensor [B,De] or [B,T',De]
 *   sample_batch.future_features  : Tensor [B,C,Hf,D]
 *   sample_batch.future_mask      : Tensor [B,C,Hf] (0=ignore, 1=valid)
 * The template methods only rely on these members and .to(...) being available.
 *
 * Shapes
 *  - future_features: [B, C, Hf, D]
 *  - selected targets: [B, C, Hf, Dy]  (from target_dims_)
 *  - encoding:         [B, De] or [B, T', De] (mean-pooled over T' → [B, De])
 *  - MDN outputs:      log_pi [B,C,Hf,K], mu/sigma [B,C,Hf,K,Dy]
 */

#include <torch/torch.h>
#include <torch/serialize.h>

#include <chrono>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>
#include <type_traits>
#include <memory>

// Project headers
#include "piaabo/dutils.h"
#include "piaabo/dconfig.h"

#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_types.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_utils.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_head.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_backbone.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_loss.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network.h"

#include "piaabo/torch_compat/optim/optimizers.h"

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"
#include "camahjucunu/BNF/implementations/training_components/training_components.h"
#include "jkimyei/training_setup/jk_setup.h"

namespace cuwacunu {
namespace wikimyei {

class ExpectedValue {
public:
  // -------- training wiring --------
  std::string component_name;

  // -------- target selection --------
  std::vector<int64_t> target_dims_;

  // -------- optional static weights --------
  std::vector<float> static_channel_weights_; // size C
  std::vector<float> static_feature_weights_; // size Dy

  // -------- model (architecture) --------
  cuwacunu::wikimyei::mdn::MdnModel semantic_model{nullptr};  // NOTE: initialize to nullptr

  // -------- scheduler stepping --------
  bool step_scheduler_per_iter_{true};

  // -------- horizon weighting policy --------
  enum class HorizonPolicy { Uniform, NearTerm, VeryNearTerm };
  HorizonPolicy horizon_policy_{HorizonPolicy::Uniform};
  float gamma_near_{0.95f};
  float gamma_very_{0.80f};

  // -------- optional dynamic channel EMA weighting --------
  bool use_channel_ema_weights_{false};
  double ema_alpha_{0.95};
  torch::Tensor channel_ema_;  // [C], device float

  // -------- Training plumbing --------
  std::vector<at::Tensor> trainable_params_;
  std::unique_ptr<torch::optim::Optimizer> optimizer;
  std::unique_ptr<cuwacunu::jkimyei::LRSchedulerAny> lr_sched;
  std::unique_ptr<cuwacunu::wikimyei::mdn::MdnNLLLoss> loss_obj;

  double grad_clip{1.0};
  int    optimizer_threshold_reset{-1};

  // -------- Telemetry / checkpoint bookkeeping --------
  int64_t total_iters_trained_{0};
  int64_t total_epochs_trained_{0};
  int     telemetry_every_{0};  // 0 = disabled

  // Best metric tracking (persisted in checkpoints)
  double best_metric_{std::numeric_limits<double>::infinity()};
  int    best_epoch_{-1};

  // Latest telemetry (for dashboards)
  torch::Tensor last_per_channel_nll_; // [C] on device; accessors return CPU clones
  torch::Tensor last_per_horizon_nll_; // [Hf]

public:
  // ---------- ctor ----------
  explicit ExpectedValue(const std::string& component_name_);

  // ---------- basic API ----------
  /// Note: these always reflect the underlying MdnModel.
  [[nodiscard]] inline const torch::Device& device() const { return semantic_model->device; }
  [[nodiscard]] inline torch::Dtype dtype() const { return semantic_model->dtype; }
  [[nodiscard]] inline int64_t Dy() const { return semantic_model->Dy; }
  [[nodiscard]] inline int64_t K()  const { return semantic_model->K;  }

  inline cuwacunu::wikimyei::mdn::MdnOut forward(const torch::Tensor& x) { return semantic_model->forward(x); }
  inline cuwacunu::wikimyei::mdn::MdnOut forward_from_encoding(const torch::Tensor& encoding) {
    return semantic_model->forward_from_encoding(encoding);
  }
  inline torch::Tensor expected_value_from_out(const cuwacunu::wikimyei::mdn::MdnOut& out) const {
    return cuwacunu::wikimyei::mdn::mdn_expectation(out);
  }
  inline torch::Tensor predict_expected_value_from_encoding(const torch::Tensor& encoding) {
    return expected_value_from_out(forward_from_encoding(encoding));
  }

  // ---------- knobs ----------
  void set_horizon_policy(HorizonPolicy p) { horizon_policy_ = p; }
  void set_horizon_gammas(float near_gamma, float very_gamma) { gamma_near_ = near_gamma; gamma_very_ = very_gamma; }
  void set_static_channel_weights(std::vector<float> w) { static_channel_weights_ = std::move(w); }
  void set_static_feature_weights(std::vector<float> w) { static_feature_weights_ = std::move(w); }
  void enable_channel_ema_weights(bool on = true, double alpha = 0.95) { use_channel_ema_weights_ = on; ema_alpha_ = alpha; }
  void set_telemetry_every(int N) { telemetry_every_ = std::max(0, N); }

  // ---------- telemetry accessors ----------
  torch::Tensor get_last_per_channel_nll(bool to_cpu = true) const {
    if (!last_per_channel_nll_.defined()) return torch::Tensor();
    return to_cpu ? last_per_channel_nll_.detach().to(torch::kCPU).clone()
                  : last_per_channel_nll_.detach().clone();
  }
  torch::Tensor get_last_per_horizon_nll(bool to_cpu = true) const {
    if (!last_per_horizon_nll_.defined()) return torch::Tensor();
    return to_cpu ? last_per_horizon_nll_.detach().to(torch::kCPU).clone()
                  : last_per_horizon_nll_.detach().clone();
  }

  // ---------- helpers: targets & weights ----------
  static torch::Tensor select_targets(const torch::Tensor& future_features,
                                      const std::vector<int64_t>& target_dims);
  static torch::Tensor masked_mean_loss_per_channel(const torch::Tensor& nll,
                                                    const torch::Tensor& mask);
  /// Averages across (B,C) to obtain per-horizon loss, safely handling empty masks.
  /// Returns [Hf].
  static torch::Tensor masked_mean_loss_per_horizon(
                                                    const torch::Tensor& nll,
                                                    const torch::Tensor& mask);

  torch::Tensor build_horizon_weights(int64_t Hf, torch::Device dev, torch::Dtype dt) const;
  torch::Tensor build_channel_weights(int64_t C, torch::Device dev, torch::Dtype dt);
  torch::Tensor build_feature_weights(int64_t Dy, torch::Device dev, torch::Dtype dt) const;

  torch::Tensor channel_weights_from_ema(int64_t C);
  void update_channel_ema(const torch::Tensor& ch_mean_loss);

  // ---------- telemetry helper: compute per-(B,C,Hf) NLL map ----------
  torch::Tensor compute_nll_map(const cuwacunu::wikimyei::mdn::MdnOut& out,
                                const torch::Tensor& y,      // [B,C,Hf,Dy]
                                const torch::Tensor& mask);  // [B,C,Hf]

  // ---------- training ----------
  template <typename Loader>
  std::vector<std::pair<int, double>> fit(Loader& dataloader,
                                          int n_epochs = -1,
                                          int n_iters  = -1,
                                          bool verbose = false);

  // ---------- evaluation ----------
  template <typename Loader>
  double evaluate_nll(Loader& dataloader, bool verbose = false);

  // ---------- checkpointing (safe state-dict, no JIT pickler) ----------
  bool save_checkpoint(const std::string& path) const;
  bool load_checkpoint(const std::string& path, bool strict = true);

private:
  // ---------- archive helpers ----------
  static double  ar_try_read_scalar_double(torch::serialize::InputArchive& ar, const char* key, double def);
  static int64_t ar_try_read_scalar_int64 (torch::serialize::InputArchive& ar, const char* key, int64_t def);
  static bool    ar_try_read_tensor       (torch::serialize::InputArchive& ar, const char* key, torch::Tensor& out);
  void display_model(bool display_semantic) const;

  // ---------- scheduler helpers ----------
  void replay_scheduler_progress();

  // Best-effort scheduler (de)serialization detection via SFINAE.
  template <typename T, typename = void>
  struct has_save : std::false_type {};
  template <typename T>
  struct has_save<T, std::void_t<decltype(std::declval<T&>().save(std::declval<torch::serialize::OutputArchive&>()))>>
    : std::true_type {};

  template <typename T, typename = void>
  struct has_load : std::false_type {};
  template <typename T>
  struct has_load<T, std::void_t<decltype(std::declval<T&>().load(std::declval<torch::serialize::InputArchive&>()))>>
    : std::true_type {};

  template <typename S>
  static bool try_save_scheduler(S& sched, torch::serialize::OutputArchive& ar) {
    if constexpr (has_save<S>::value) { sched.save(ar); return true; }
    return false;
  }
  template <typename S>
  static bool try_load_scheduler(S& sched, torch::serialize::InputArchive& ar) {
    if constexpr (has_load<S>::value) { sched.load(ar); return true; }
    return false;
  }

  // ---------- optimizer guard ----------
  /// Legacy signature (kept for ABI). Avoids compiler "unused parameter" warnings by not naming it.
  void maybe_reset_optimizer_state(double /*clip_threshold*/);
  /// Preferred overload: pass a precomputed global grad-norm to avoid a second device→host sync.
  void maybe_reset_optimizer_state_by_norm(double global_grad_norm);
};

// ======================================================================
// Template implementations
// ======================================================================

template <typename Loader>
std::vector<std::pair<int, double>>
ExpectedValue::fit(Loader& dataloader, int n_epochs, int n_iters, bool verbose)
{
  using Clock = std::chrono::steady_clock;
  const auto t0 = Clock::now();

  int epoch_count = 0;
  int iter_count  = 0;
  bool stop_loop  = false;
  std::vector<std::pair<int, double>> loss_log;

  semantic_model->train();
  const bool do_clip = (grad_clip > 0.0);

  while (!stop_loop) {
    if (n_epochs >= 0 && epoch_count >= n_epochs) break;

    double cum_loss = 0.0;
    int epoch_iters = 0;

    for (const auto& sample_batch : dataloader) {
      if (n_iters >= 0 && iter_count >= n_iters) { stop_loop = true; break; }

      TORCH_CHECK(sample_batch.encoding.defined(), "[ExpectedValue::fit] encoding is undefined in batch.");
      TORCH_CHECK(sample_batch.future_features.defined() && sample_batch.future_mask.defined(),
                  "[ExpectedValue::fit] future_features and future_mask must be defined.");

      auto enc   = sample_batch.encoding.to(semantic_model->device, semantic_model->dtype, /*non_blocking=*/true, /*copy=*/false);
      auto fut   = sample_batch.future_features.to(semantic_model->device, semantic_model->dtype, true, false); // [B,C,Hf,D]
      auto fmask = sample_batch.future_mask.to(semantic_model->device, torch::kFloat32, true, false);           // [B,C,Hf]

      if (enc.dim() == 3) {           // [B, T', De] -> mean pool over time
        enc = enc.mean(/*dim=*/1);    // -> [B, De]
      }
      TORCH_CHECK(enc.dim() == 2,             "[ExpectedValue::fit] encoding must be [B,De]; got ", enc.sizes());
      TORCH_CHECK(fut.dim() == 4,             "[ExpectedValue::fit] future_features must be [B,C,Hf,D]");
      TORCH_CHECK(fmask.dim() == 3,           "[ExpectedValue::fit] future_mask must be [B,C,Hf]");
      TORCH_CHECK(fut.size(0) == enc.size(0), "[ExpectedValue::fit] batch size mismatch encoding vs future_features.");

      auto y   = select_targets(fut, target_dims_);              // [B,C,Hf,Dy]
      auto out = semantic_model->forward_from_encoding(enc);

      const int64_t C  = fmask.size(1);
      const int64_t Hf = fmask.size(2);
      const int64_t Dy = y.size(3);

      auto w_ch  = build_channel_weights(C,  semantic_model->device, semantic_model->dtype); // [C] or ones
      auto w_tau = build_horizon_weights(Hf, semantic_model->device, semantic_model->dtype); // [Hf] or ones
      auto w_dim = build_feature_weights(Dy, semantic_model->device, semantic_model->dtype); // [Dy] or ones

      auto loss = loss_obj->compute(out, y, fmask, w_ch, w_tau, w_dim);

      // --- Telemetry / EMA ---
      const int next_iter = iter_count + 1;
      const bool log_now = (telemetry_every_ > 0) && (next_iter % telemetry_every_ == 0);
      if (use_channel_ema_weights_ || log_now) {
        auto nll_map = compute_nll_map(out, y, fmask);             // [B,C,Hf]

        // Per-channel means (update EMA; also saved for telemetry)
        auto ch_mean = masked_mean_loss_per_channel(nll_map, fmask);  // [C]
        if (use_channel_ema_weights_) update_channel_ema(ch_mean);

        // Per-horizon means
        auto hz_mean = masked_mean_loss_per_horizon(nll_map, fmask);  // [Hf]

        last_per_channel_nll_ = ch_mean.detach();
        last_per_horizon_nll_ = hz_mean.detach();

        if (log_now) {
          auto ch_cpu = ch_mean.detach().to(torch::kCPU);
          auto hz_cpu = hz_mean.detach().to(torch::kCPU);
          const double ch_min = ch_cpu.min().item().toDouble();
          const double ch_max = ch_cpu.max().item().toDouble();
          const double hz_min = hz_cpu.min().item().toDouble();
          const double hz_max = hz_cpu.max().item().toDouble();
          log_info("[ExpectedValue::telemetry] it=%d chNLL[min=%.5f max=%.5f] hzNLL[min=%.5f max=%.5f]\n",
                   next_iter, ch_min, ch_max, hz_min, hz_max);

          if (use_channel_ema_weights_) {
            auto ema_cpu = channel_ema_.defined() ? channel_ema_.detach().to(torch::kCPU) : torch::Tensor();
            if (ema_cpu.defined()) {
              const double ema_min = ema_cpu.min().item().toDouble();
              const double ema_max = ema_cpu.max().item().toDouble();
              log_info("[ExpectedValue::telemetry] it=%d channelEMA[min=%.5f max=%.5f]\n",
                       next_iter, ema_min, ema_max);
            }
          }
        }
      }
      // -----------------------

      optimizer->zero_grad(true);
      loss.backward();

      double grad_norm_before_clip = 0.0;
      if (do_clip) {
        // Returns the total norm (pre-clip) and performs clipping in-place.
        grad_norm_before_clip =
          torch::nn::utils::clip_grad_norm_(semantic_model->parameters(), grad_clip);
      } else {
        // Compute the norm once without altering grads.
        double sumsq = 0.0;
        for (auto& p : semantic_model->parameters()) {
          if (p.grad().defined()) {
            auto g = p.grad();
            sumsq += g.pow(2).sum().item<double>();
          }
        }
        grad_norm_before_clip = std::sqrt(sumsq);
      }
      // Optional safety: reset momentum states if grads explode (uses the precomputed norm).
      maybe_reset_optimizer_state_by_norm(grad_norm_before_clip);

      optimizer->step();
      if (step_scheduler_per_iter_ && lr_sched) lr_sched->step();

      const double lval = loss.item().toDouble();
      cum_loss += lval;
      ++epoch_iters;
      ++iter_count;
      ++total_iters_trained_;

      loss_log.emplace_back(iter_count, lval);
    } // dataloader

    if (!step_scheduler_per_iter_ && lr_sched) lr_sched->step();

    const double epoch_loss = (epoch_iters > 0) ? (cum_loss / epoch_iters) : std::numeric_limits<double>::quiet_NaN();
    if (std::isfinite(epoch_loss) && epoch_loss < best_metric_) {
      best_metric_ = epoch_loss;
      best_epoch_  = epoch_count;
    }

    const double lr = cuwacunu::wikimyei::mdn::get_lr_generic(*optimizer);
    
    ++epoch_count;
    ++total_epochs_trained_;

    if (verbose && (epoch_count % 50 == 0 || epoch_count == 1 || epoch_count >= n_epochs)) {
      log_info("[ExpectedValue::fit] epoch=%d\titers=%d\tavg_loss=%.6f\tbest=%.6f.at:%d\tlr=%.3g\n",
        epoch_count, epoch_iters, epoch_loss, best_metric_, best_epoch_, lr);
    }
  } // while

  const auto t1 = Clock::now();
  const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count();
  log_info("%s[ExpectedValue::fit]%s done, iters=%lld, best=%.6f.at:%d, wall_ms=%lld\n",
           ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET,
           static_cast<long long>(total_iters_trained_), best_metric_, best_epoch_, static_cast<long long>(ms));
  return loss_log;
}

template <typename Loader>
double ExpectedValue::evaluate_nll(Loader& dataloader, bool verbose)
{
  torch::NoGradGuard ng;
  semantic_model->eval();

  double cum = 0.0;
  int n = 0;

  for (const auto& sample_batch : dataloader) {
    TORCH_CHECK(sample_batch.encoding.defined(), "[ExpectedValue::evaluate_nll] encoding undefined");
    TORCH_CHECK(sample_batch.future_features.defined() && sample_batch.future_mask.defined(),
                "[ExpectedValue::evaluate_nll] future_features/future_mask undefined");

    auto enc   = sample_batch.encoding.to(semantic_model->device, semantic_model->dtype, true, false);
    if (enc.dim() == 3) enc = enc.mean(/*dim=*/1);      // [B,T',De] → [B,De]
    TORCH_CHECK(enc.dim() == 2, "[ExpectedValue::evaluate_nll] encoding must be [B,De]; got ", enc.sizes());

    auto fut   = sample_batch.future_features.to(semantic_model->device, semantic_model->dtype, true, false); // [B,C,Hf,D]
    auto fmask = sample_batch.future_mask.to(semantic_model->device, torch::kFloat32, true, false);           // [B,C,Hf]

    auto y  = select_targets(fut, target_dims_);
    auto o  = semantic_model->forward_from_encoding(enc);

    const int64_t C  = fmask.size(1);
    const int64_t Hf = fmask.size(2);
    const int64_t Dy = y.size(3);

    auto w_ch  = build_channel_weights(C,  semantic_model->device, semantic_model->dtype);
    auto w_tau = build_horizon_weights(Hf, semantic_model->device, semantic_model->dtype);
    auto w_dim = build_feature_weights(Dy, semantic_model->device, semantic_model->dtype);

    auto ls = loss_obj->compute(o, y, fmask, w_ch, w_tau, w_dim); // scalar

    const double lval = ls.item().toDouble();
    cum += lval;
    ++n;
  }

  const double avg = (n > 0) ? (cum / n) : std::numeric_limits<double>::quiet_NaN();
  if (verbose) {
    log_info("%s[ExpectedValue::eval]%s N=%d avg_nll=%.6f\n", ANSI_COLOR_Magenta, ANSI_COLOR_RESET, n, avg);
  }
  return avg;
}

} // namespace wikimyei
} // namespace cuwacunu
