/* expected_value.h */
#pragma once
#include <torch/torch.h>

#include <chrono>
#include <limits>
#include <string>
#include <utility>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cmath>

#include "piaabo/dutils.h"

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

/**
 * @brief Semantic task: Expected Value E[y|x] via MDN (per-channel heads, multi-horizon, masked).
 *
 * The MdnModel is pure architecture (shared trunk + per-channel heads).
 * All training wiring (optimizer, scheduler, loss, grad-clip) lives here.
 *
 * Targets and masks:
 *  - y:      [B, C, Hf, Dy]   (selected dims from future_features)
 *  - mask:   [B, C, Hf]       (1 valid, 0 invalid)
 *
 * Optional weighting:
 *  - weights_ch  [C]    (per-channel)
 *  - weights_tau [Hf]   (per-horizon, built from a simple policy)
 *  - weights_dim [Dy]   (per-target-dimension/feature)
 */
class ExpectedValue {
public:
  using MdnOut = cuwacunu::wikimyei::mdn::MdnOut;
  
  // -------- training wiring --------
  cuwacunu::jkimyei::jk_setup_t  jk_setup;

  // -------- target selection --------
  // Indices into the last dimension D of future_features to predict.
  // Dy is implicitly target_dims_.size().
  std::vector<int64_t> target_dims_;
  // -------- optional static weights --------
  // Leave empty to default to ones.
  std::vector<float> static_channel_weights_; // size C
  std::vector<float> static_feature_weights_; // size Dy


  // -------- model (pure architecture) --------
  cuwacunu::wikimyei::mdn::MdnModel semantic_model;  // torch::nn::ModuleHolder<MdnModelImpl>

  // -------- scheduler stepping --------
  bool step_scheduler_per_iter_{true};

  // -------- horizon weighting policy --------
  enum class HorizonPolicy { Uniform, NearTerm, VeryNearTerm };
  HorizonPolicy horizon_policy_{HorizonPolicy::Uniform};
  float gamma_near_{0.95f};  // mild decay
  float gamma_very_{0.80f};  // strong decay

  // -------- optional dynamic channel EMA weighting --------
  bool use_channel_ema_weights_{false};
  double ema_alpha_{0.95};
  torch::Tensor channel_ema_;  // [C], device float

  // -------- Training plumbing --------
  std::vector<at::Tensor> trainable_params_; // Stable list passed to optimizer.
  std::unique_ptr<torch::optim::Optimizer> optimizer;
  std::unique_ptr<cuwacunu::jkimyei::LRSchedulerAny> lr_sched;
  std::unique_ptr<cuwacunu::wikimyei::mdn::MdnNLLLoss> loss_obj;

  double grad_clip{1.0};
  int    optimizer_threshold_reset{-1}; // placeholder: if you clamp optimizer state elsewhere

public:
  /**
   * @brief Primary constructor: Dy is derived from target_dims.size().
   *
   * Reads architecture (K,H,depth,C,Hf,dtype,device) from "VALUE_ESTIMATION" config section
   * via the model's config constructor. Training wiring is built here using @p target_setup.
   *
   * @param jk_setup_ decoded form of the <training_components>.instruction file
   */
  ExpectedValue(
    cuwacunu::jkimyei::jk_setup_t  jk_setup_
  ) : 
    jk_setup(std::move(jk_setup_))
  {
    static_channel_weights_ = cuwacunu::camahjucunu::observation_pipeline::inst.retrieve_channel_weights();
    static_feature_weights_ = cuwacunu::piaabo::dconfig::config_space_t::get<double>("VALUE_ESTIMATION", "target_weights");

    // grad_clip and optimizer_threshold_reset from config if present
    grad_clip = (double)cuwacunu::piaabo::dconfig::config_space_t::get<int>("VALUE_ESTIMATION", "grad_clip");
    optimizer_threshold_reset = cuwacunu::piaabo::dconfig::config_space_t::get<int>("VALUE_ESTIMATION", "optimizer_threshold_reset");

    target_dims_ = cuwacunu::piaabo::dconfig::config_space_t::get<int>("VALUE_ESTIMATION", "target_dims");
    semantic_model(cuwacunu::wikimyei::mdn::MdnModel(
      cuwacunu::piaabo::dconfig::config_space_t::get<int>("VICReg", "encoding_dims"),             // De
      target_dims_.size(),                                                                        // Dy
      cuwacunu::camahjucunu::observation_pipeline::inst.count_channels(),                         // C
      cuwacunu::camahjucunu::observation_pipeline::inst.max_future_sequence_length(),             // Hf
      cuwacunu::piaabo::dconfig::config_space_t::get<int>("VALUE_ESTIMATION", "mixture_comps"),   // K
      cuwacunu::piaabo::dconfig::config_space_t::get<int>("VALUE_ESTIMATION", "features_hidden"), // H
      cuwacunu::piaabo::dconfig::config_space_t::get<int>("VALUE_ESTIMATION", "residual_depth"),  // depth
      cuwacunu::piaabo::dconfig::config_dtype  ("VALUE_ESTIMATION"),
      cuwacunu::piaabo::dconfig::config_device ("VALUE_ESTIMATION"));

    
    // Collect trainable params from model
    trainable_params_.reserve(64);
    for (auto& p : semantic_model->parameters(/*recurse=*/true)) {
      if (p.requires_grad()) trainable_params_.push_back(p);
    }

    TORCH_CHECK(jk_setup.opt_builder,   "[ExpectedValue](ctor) opt_builder is null");
    TORCH_CHECK(jk_setup.sched_builder, "[ExpectedValue](ctor) sched_builder is null");

    optimizer = jk_setup.opt_builder->build(trainable_params_);
    lr_sched  = jk_setup.sched_builder->build(*optimizer);
    loss_obj  = std::make_unique<cuwacunu::wikimyei::mdn::MdnNLLLoss>(jk_setup);

    // Finalize
    log_info("%s[ExpectedValue](VALUE_ESTIMATION)%s ready.\n");
  }

  // ---------- basic API ----------
  inline const torch::Device& device() const { return semantic_model->device; }
  inline torch::Dtype dtype() const { return semantic_model->dtype; }
  inline int64_t Dy() const { return semantic_model->Dy; }
  inline int64_t K()  const { return semantic_model->K;  }

  // Forward (back-compat): x [B,De] -> MdnOut
  inline MdnOut forward(const torch::Tensor& x) { return semantic_model->forward(x); }

  // Forward from encoding [B,De] or [B,T',De]
  inline MdnOut forward_from_encoding(const torch::Tensor& encoding) {
    return semantic_model->forward_from_encoding(encoding);
  }

  // E[y|x] for generalized shapes: returns [B,C,Hf,Dy]
  inline torch::Tensor expected_value_from_out(const MdnOut& out) const {
    return cuwacunu::wikimyei::mdn::mdn_expectation(out);
  }

  inline torch::Tensor predict_expected_value_from_encoding(const torch::Tensor& encoding) {
    return expected_value_from_out(forward_from_encoding(encoding));
  }

  // ---------- setters for weighting controls ----------
  void set_horizon_policy(HorizonPolicy p) { horizon_policy_ = p; }
  void set_horizon_gammas(float near_gamma, float very_gamma) { gamma_near_ = near_gamma; gamma_very_ = very_gamma; }
  void set_static_channel_weights(std::vector<float> w) { static_channel_weights_ = std::move(w); }
  void set_static_feature_weights(std::vector<float> w) { static_feature_weights_ = std::move(w); }

  // ---------- helpers: targets & weights ----------
  // Build y: [B,C,Hf,Dy] from future_features: [B,C,Hf,D] selecting target_dims_.
  static torch::Tensor select_targets(const torch::Tensor& future_features,
                                      const std::vector<int64_t>& target_dims) {
    TORCH_CHECK(future_features.defined(), "[ExpectedValue::select_targets] future_features undefined");
    TORCH_CHECK(future_features.dim() == 4, "[ExpectedValue::select_targets] expecting [B,C,Hf,D]");
    const auto B  = future_features.size(0);
    const auto C  = future_features.size(1);
    const auto Hf = future_features.size(2);
    const auto D  = future_features.size(3);
    TORCH_CHECK(!target_dims.empty(), "[ExpectedValue::select_targets] empty target_dims");
    for (auto d : target_dims) {
      TORCH_CHECK(d >= 0 && d < D, "[ExpectedValue::select_targets] target dim out of range");
    }
    auto optsLong = future_features.options().dtype(torch::kLong);
    const auto Dy = static_cast<int64_t>(target_dims.size());
    auto idx = torch::from_blob(const_cast<int64_t*>(target_dims.data()), {Dy}, optsLong.device(torch::kCPU)).clone();
    idx = idx.to(future_features.device());

    auto flat = future_features.view({B*C*Hf, D});
    auto idx2 = idx.unsqueeze(0).expand({B*C*Hf, Dy});
    auto y_sel = flat.gather(/*dim=*/1, idx2);
    return y_sel.view({B, C, Hf, Dy});
  }

  // masked mean loss per channel over horizons
  static torch::Tensor masked_mean_loss_per_channel(const torch::Tensor& nll,      // [B,C,Hf]
                                                    const torch::Tensor& mask) {   // [B,C,Hf]
    auto valid = mask.to(nll.dtype());
    auto sumB = (nll * valid).sum(/*dim=*/0);           // [C,Hf]
    auto den  = valid.sum(/*dim=*/0).clamp_min(1.0);    // [C,Hf]
    auto ch_mean_over_h = (sumB / den).mean(/*dim=*/1); // [C]
    return ch_mean_over_h;
  }

  // EMA-derived channel weights: w_c = 1 / (eps + ema_c)
  torch::Tensor channel_weights_from_ema(int64_t C) {
    if (!use_channel_ema_weights_) {
      return torch::ones({C}, torch::TensorOptions().dtype(torch::kFloat32).device(device()));
    }
    if (!channel_ema_.defined() || channel_ema_.sizes() != torch::IntArrayRef({C})) {
      channel_ema_ = torch::ones({C}, torch::TensorOptions().dtype(torch::kFloat32).device(device()));
    }
    const float eps = 1e-6f;
    return 1.0f / (channel_ema_ + eps);
  }

  void update_channel_ema(const torch::Tensor& ch_mean_loss) {
    if (!use_channel_ema_weights_) return;
    if (!channel_ema_.defined()) {
      channel_ema_ = ch_mean_loss.detach();
      return;
    }
    channel_ema_.mul_(ema_alpha_).add_(ch_mean_loss.detach() * (1.0 - ema_alpha_));
  }

  // Build [Hf] according to policy
  torch::Tensor build_horizon_weights(int64_t Hf, torch::Device dev, torch::Dtype dt) const {
    if (Hf <= 0) return torch::Tensor();
    std::vector<float> w(Hf, 1.f);
    switch (horizon_policy_) {
      case HorizonPolicy::Uniform:
        break;
      case HorizonPolicy::NearTerm: {
        float g = gamma_near_;
        for (int64_t t = 0; t < Hf; ++t) w[t] = std::pow(g, (float)t);
        break;
      }
      case HorizonPolicy::VeryNearTerm: {
        float g = gamma_very_;
        for (int64_t t = 0; t < Hf; ++t) w[t] = std::pow(g, (float)t);
        break;
      }
    }
    auto wt = torch::from_blob(w.data(), {Hf}, torch::TensorOptions().dtype(torch::kFloat32)).clone();
    if (wt.dtype() != dt) wt = wt.to(dt);
    return wt.to(dev);
  }

  // Combine static channel weights and EMA weights
  torch::Tensor build_channel_weights(int64_t C, torch::Device dev, torch::Dtype dt) {
    if (C <= 0) return torch::Tensor();
    auto w = torch::ones({C}, torch::TensorOptions().dtype(dt).device(dev));

    if (!static_channel_weights_.empty()) {
      TORCH_CHECK((int64_t)static_channel_weights_.size() == C,
                  "[ExpectedValue] static_channel_weights size must equal C");
      auto ws = torch::from_blob(static_channel_weights_.data(), {C}, torch::TensorOptions().dtype(torch::kFloat32)).clone();
      if (ws.dtype() != dt) ws = ws.to(dt);
      w = w * ws.to(dev);
    }
    if (use_channel_ema_weights_) {
      auto w_ema = channel_weights_from_ema(C); // float on device
      if (w_ema.dtype() != dt) w_ema = w_ema.to(dt);
      w = w * w_ema;
    }
    return w;
  }

  // Build [Dy] from static_feature_weights_ or ones
  torch::Tensor build_feature_weights(int64_t Dy, torch::Device dev, torch::Dtype dt) const {
    if (Dy <= 0) return torch::Tensor();
    if (!static_feature_weights_.empty()) {
      TORCH_CHECK((int64_t)static_feature_weights_.size() == Dy,
                  "[ExpectedValue] static_feature_weights size must equal Dy");
      auto wd = torch::from_blob(static_feature_weights_.data(), {Dy}, torch::TensorOptions().dtype(torch::kFloat32)).clone();
      if (wd.dtype() != dt) wd = wd.to(dt);
      return wd.to(dev);
    }
    return torch::ones({Dy}, torch::TensorOptions().dtype(dt).device(dev));
  }

  // ---------- training ----------
  template <typename Loader>
  std::vector<std::pair<int, double>> fit(
    Loader& dataloader,
    int n_epochs = -1,
    int n_iters  = -1,
    bool verbose = false)
  {
    using Clock = std::chrono::steady_clock;
    const auto t0 = Clock::now();

    int epoch_count = 0;
    int iter_count  = 0;
    bool stop_loop  = false;
    std::vector<std::pair<int, double>> loss_log;

    double best_loss = std::numeric_limits<double>::infinity();
    int    best_epoch = -1;

    semantic_model->train();
    const bool do_clip = (grad_clip > 0.0);

    const int64_t C  = fmask.size(1);
    const int64_t Hf = fmask.size(2);
    const int64_t Dy = y.size(3);

    auto w_ch  = build_channel_weights(C,  semantic_model->device, semantic_model->dtype); // [C] or empty
    auto w_tau = build_horizon_weights(Hf, semantic_model->device, semantic_model->dtype); // [Hf] or empty
    auto w_dim = build_feature_weights(Dy, semantic_model->device, semantic_model->dtype); // [Dy] or empty

    while (!stop_loop) {
      if (n_epochs >= 0 && epoch_count >= n_epochs) break;

      double cum_loss = 0.0;
      int epoch_iters = 0;

      for (auto sample_batch : dataloader) {
        if (n_iters >= 0 && iter_count >= n_iters) { stop_loop = true; break; }

        TORCH_CHECK(sample_batch.encoding.defined(),
                    "[ExpectedValue::fit] encoding is undefined in batch.");
        TORCH_CHECK(sample_batch.future_features.defined() && sample_batch.future_mask.defined(),
                    "[ExpectedValue::fit] future_features and future_mask must be defined.");

        auto enc   = sample_batch.encoding.to(semantic_model->device, semantic_model->dtype, /*non_blocking=*/true, /*copy=*/false);
        auto fut   = sample_batch.future_features.to(semantic_model->device, semantic_model->dtype, true, false); // [B,C,Hf,D]
        auto fmask = sample_batch.future_mask.to(semantic_model->device, torch::kFloat32, true, false);           // [B,C,Hf]

        auto y   = select_targets(fut, target_dims_);              // [B,C,Hf,Dy]
        auto out = semantic_model->forward_from_encoding(enc);     // MdnOut

        auto loss = loss_obj->compute(out, y, fmask, w_ch, w_tau, w_dim);

        // Optional: update EMA-based channel weights (diagnostics / adaptive weighting)
        if (use_channel_ema_weights_) {
          const auto& log_pi = out.log_pi; // [B,C,Hf,K]
          const auto& mu     = out.mu;     // [B,C,Hf,K,Dy]
          auto sigma         = out.sigma;  // [B,C,Hf,K,Dy]
          sigma = sigma.clamp_min(loss_obj->sigma_min_);
          if (loss_obj->sigma_max_ > 0.0) sigma = sigma.clamp_max(loss_obj->sigma_max_);
          auto eps_t = sigma.new_full({}, loss_obj->eps_);
          static const double LOG2PI = std::log(2.0 * M_PI);

          const auto Bsz = y.size(0), K = log_pi.size(3);
          auto y_b  = y.unsqueeze(3).expand({Bsz,C,Hf,K,Dy});
          auto diff   = (y_b - mu) / (sigma + eps_t);
          auto perdim = -0.5 * diff.pow(2) - (sigma + eps_t).log() - 0.5 * LOG2PI; // [B,C,Hf,K,Dy]
          auto comp_logp = perdim.sum(-1);                                // [B,C,Hf,K]
          auto log_mix   = cuwacunu::wikimyei::mdn::logsumexp(log_pi + comp_logp, 3, false);
          auto nll_map   = -log_mix;                                      // [B,C,Hf]
          auto ch_mean   = masked_mean_loss_per_channel(nll_map, fmask);  // [C]
          update_channel_ema(ch_mean);
        }

        optimizer->zero_grad(true);
        loss.backward();

        if (do_clip) torch::nn::utils::clip_grad_norm_(semantic_model->parameters(), grad_clip);

        // If you have an Adam state clamp util, apply when optimizer_threshold_reset >= 0 here.

        optimizer->step();
        if (step_scheduler_per_iter_ && lr_sched) lr_sched->step();

        const double lval = loss.template item<double>();
        cum_loss += lval;
        ++epoch_iters;
        ++iter_count;

        if (verbose && (iter_count % 25 == 0)) {
          const double lr = cuwacunu::wikimyei::mdn::get_lr_generic(*optimizer);
          log_info("[EV::fit] iter=%d loss=%.6f lr=%.3g\n", iter_count, lval, lr);
        }

        loss_log.emplace_back(iter_count, lval);
      } // dataloader

      if (!step_scheduler_per_iter_ && lr_sched) lr_sched->step();

      const double epoch_loss = (epoch_iters > 0) ? (cum_loss / epoch_iters) : std::numeric_limits<double>::quiet_NaN();
      if (std::isfinite(epoch_loss) && epoch_loss < best_loss) {
        best_loss = epoch_loss;
        best_epoch = epoch_count;
      }

      const double lr = cuwacunu::wikimyei::mdn::get_lr_generic(*optimizer);
      log_info("%s[EV::fit]%s epoch=%d iters=%d avg_loss=%.6f best=%.6f@%d lr=%.3g\n",
               ANSI_COLOR_Cyan, ANSI_COLOR_RESET,
               epoch_count, epoch_iters, epoch_loss, best_loss, best_epoch, lr);

      ++epoch_count;
    } // while

    using Clock = std::chrono::steady_clock;
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(Clock::now().time_since_epoch()).count();
    log_info("%s[EV::fit]%s done, iters=%d, best=%.6f@%d\n",
             ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET, iter_count, best_loss, best_epoch);
    return loss_log;
  }

  // ---------- evaluation ----------
  template <typename Loader>
  double evaluate_nll(Loader& dataloader, bool verbose = false)
  {
    torch::NoGradGuard ng;
    semantic_model->eval();

    double cum = 0.0;
    int n = 0;

    for (auto sample_batch : dataloader) {
      TORCH_CHECK(sample_batch.encoding.defined(), "[ExpectedValue::evaluate_nll] encoding undefined");
      TORCH_CHECK(sample_batch.future_features.defined() && sample_batch.future_mask.defined(),
                  "[ExpectedValue::evaluate_nll] future_features/future_mask undefined");

      auto enc   = sample_batch.encoding.to(semantic_model->device, semantic_model->dtype, true, false);
      auto fut   = sample_batch.future_features.to(semantic_model->device, semantic_model->dtype, true, false); // [B,C,Hf,D]
      auto fmask = sample_batch.future_mask.to(semantic_model->device, torch::kFloat32, true, false);           // [B,C,Hf]

      auto y  = select_targets(fut, target_dims_);              // [B,C,Hf,Dy]
      auto o  = semantic_model->forward_from_encoding(enc);

      const int64_t C  = fmask.size(1);
      const int64_t Hf = fmask.size(2);
      const int64_t Dy = y.size(3);

      auto w_ch  = build_channel_weights(C,  semantic_model->device, semantic_model->dtype);
      auto w_tau = build_horizon_weights(Hf, semantic_model->device, semantic_model->dtype);
      auto w_dim = build_feature_weights(Dy, semantic_model->device, semantic_model->dtype);

      auto ls = loss_obj->compute(o, y, fmask, w_ch, w_tau, w_dim); // scalar

      const double lval = ls.template item<double>();
      cum += lval;
      ++n;
    }

    const double avg = (n > 0) ? (cum / n) : std::numeric_limits<double>::quiet_NaN();
    if (verbose) {
      log_info("%s[EV::eval]%s N=%d avg_nll=%.6f\n", ANSI_COLOR_Magenta, ANSI_COLOR_RESET, n, avg);
    }
    return avg;
  }
};

} // namespace wikimyei
} // namespace cuwacunu
