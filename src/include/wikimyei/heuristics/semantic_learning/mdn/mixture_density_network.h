/* mixture_density_network.h */
/**
 * @file mixture_density_network.h
 * @brief Mixture Density Network (MDN) model composed of a residual MLP backbone
 *        and a diagonal-Gaussian mixture head, wired to training components via @c jk_setup_t.
 *
 * The model predicts mixture parameters (log mixing weights, means, standard deviations)
 * for a target vector y ∈ R^{Dy}, given an input vector x ∈ R^{De}. It is designed to plug
 * directly into your training infrastructure:
 * - Optimizer and LR scheduler are built from the provided @c jk_setup_t.
 * - Loss is an NLL over a diagonal Gaussian mixture (configurable via the loss row).
 *
 * ### Shapes
 * - Input:  x [B, De]
 * - Output: log_pi [B, K], mu [B, K, Dy], sigma [B, K, Dy]
 *
 * ### Notes
 * - The module moves itself to the requested device/dtype at construction.
 * - @c trainable_params_ is collected from all parameters with @c requires_grad.
 * - If @c optimizer_threshold_reset >= 0, an Adam step clamp utility (if available in your stack)
 *   can be applied each step to guard against numerical spikes.
 *
 */

#pragma once
#include <torch/torch.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "piaabo/dutils.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_types.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_utils.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_head.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_backbone.h"
#include "wikimyei/heuristics/semantic_learning/mdn/mixture_density_network_loss.h"

#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "piaabo/torch_compat/optim/optimizers.h"
#include "camahjucunu/BNF/implementations/observation_pipeline/observation_pipeline.h"
#include "camahjucunu/BNF/implementations/training_components/training_components.h"
#include "jkimyei/training_components/jk_setup.h"

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

/**
 * @class MdnModelImpl
 * @brief MDN model (residual MLP backbone + MDN head) with training wiring to @c jk_setup_t.
 *
 * @details
 * Constructs a Mixture Density Network that maps inputs of size @c De to mixture
 * parameters for a @c K-component diagonal Gaussian over @c Dy targets. The optimizer,
 * learning-rate scheduler, and loss object are derived from @c jk_setup.
 *
 * The internal layout:
 *  - @c Backbone : residual MLP producing a feature vector of size @c H
 *  - @c MdnHead  : predicts @c {log_pi, mu, sigma} with shapes [B,K], [B,K,Dy], [B,K,Dy]
 *  - @c loss_obj : NLL over the mixture (configurable via loss table row)
 *
 * The module is moved to the requested device/dtype during construction, and
 * all trainable parameters are collected into @c trainable_params_ for optimizer setup.
 */
struct MdnModelImpl : torch::nn::Module {
  // network
  int64_t De{0};
  int64_t Dy{0};
  int64_t K{0};
  int64_t H{0};
  int64_t depth{0};

  // training wiring
  cuwacunu::jkimyei::jk_setup_t  jk_setup;
  std::vector<at::Tensor>        trainable_params_;
  std::unique_ptr<torch::optim::Optimizer> optimizer;
  std::unique_ptr<cuwacunu::jkimyei::LRSchedulerAny> lr_sched;
  std::unique_ptr<MdnNLLLoss>    loss_obj;

  // misc
  torch::Dtype  dtype;
  torch::Device device;
  double grad_clip{1.0};
  int    optimizer_threshold_reset{-1};

  // network
  Backbone backbone{nullptr};
  MdnHead  head{nullptr};

  /**
   * @brief Construct an MDN with explicit architectural dimensions and a training setup.
   *
   * @param De_     Input dimensionality (number of features per sample).
   * @param Dy_     Target dimensionality (e.g., 1 for scalar regression).
   * @param K_      Number of mixture components in the diagonal Gaussian mixture.
   * @param H_      Hidden feature size produced by the backbone (channel width).
   * @param depth_  Residual depth (number of residual MLP blocks in the backbone).
   * @param jk_setup_ Training setup object providing:
   *                  - optimizer builder (@c opt_builder)
   *                  - LR scheduler builder (@c sched_builder)
   *                  - loss configuration row (used by @c MdnNLLLoss)
   * @param dtype_  Torch dtype for module parameters and computations (default: @c torch::kFloat32).
   * @param device_ Torch device for the module (e.g., @c torch::kCPU or @c torch::kCUDA).
   * @param grad_clip_ Maximum norm for gradient clipping; if <= 0, clipping is disabled (default: 1.0).
   * @param optimizer_threshold_reset_ Optional clamp/reset threshold applied to optimizer state
   *        (implementation-dependent; set < 0 to disable; default: -1).
   *
   * @note
   * - The module and its parameters are moved to (@p device_, @p dtype_) during construction.
   * - Optimizer, scheduler, and loss are created immediately from @p jk_setup_.
   * - Parameters with @c requires_grad==true are collected into @c trainable_params_.
   */
  MdnModelImpl(
      int64_t De_,            // input dims
      int64_t Dy_,            // target dims
      int64_t K_,             // mixture comps
      int64_t H_,             // features hidden
      int64_t depth_,         // residual depth
      cuwacunu::jkimyei::jk_setup_t jk_setup_,
      torch::Dtype dtype_ = torch::kFloat32,
      torch::Device device_ = torch::kCPU,
      double grad_clip_ = 1.0,
      int optimizer_threshold_reset_ = -1
    ) : 
      De(De_), 
      Dy(Dy_), 
      K(K_), 
      H(H_), 
      depth(depth_),
      jk_setup(std::move(jk_setup_)), 
      dtype(dtype_), 
      device(device_),
      grad_clip(grad_clip_), 
      optimizer_threshold_reset(optimizer_threshold_reset_)
  {
    // build network
    BackboneOptions bopt{De, H, depth};
    backbone = register_module("backbone", Backbone(bopt));
    MdnHeadOptions hopt{H, Dy, K};
    head = register_module("head", MdnHead(hopt));

    // move whole module to device/dtype early to avoid mismatches
    this->to(device, dtype, /*non_blocking=*/false);

    // collect trainable parameters (ensures lifetime ties to module)
    trainable_params_.clear();
    for (auto& p : this->parameters(/*recurse=*/true)) {
      if (p.requires_grad()) trainable_params_.push_back(p);
    }

    // optimizer & scheduler & loss
    TORCH_CHECK(jk_setup.opt_builder,  "[MDN::ctor] opt_builder is null");
    TORCH_CHECK(jk_setup.sched_builder,"[MDN::ctor] sched_builder is null");
    optimizer = jk_setup.opt_builder->build(trainable_params_);
    lr_sched  = jk_setup.sched_builder->build(*optimizer);
    loss_obj  = std::make_unique<MdnNLLLoss>(jk_setup);

    display_model();
    warm_up();
  }

    /**
   * @brief Defacto constructor for VICReg model from configuration file
   *
   * @param target_conf target in .config file
   * @param target_setup target in <training_components>.instruction file
   * @param De_ input dims
   * @param Dy_ target dims
   */
  MdnModelImpl(
    const char* target_conf,  /* target in .config file */
    const char* target_setup, /* target in <training_components>.instruction file */
    int64_t De_,              /* input dims */
    int64_t Dy_               /* target dims */
  ) : MdnModelImpl(
      De_,                                                                                                    /* input dims */
      Dy_,                                                                                                    /* target dims */
      cuwacunu::piaabo::dconfig::config_space_t::get<int>     (target_conf, "mixture_comps"),                 /* K_ */
      cuwacunu::piaabo::dconfig::config_space_t::get<int>     (target_conf, "features_hidden"),               /* H_ */
      cuwacunu::piaabo::dconfig::config_space_t::get<int>     (target_conf, "residual_depth"),                /* depth */
      cuwacunu::jkimyei::build_training_setup_component(
        cuwacunu::camahjucunu::BNF::trainingPipeline().decode(
          cuwacunu::piaabo::dconfig::config_space_t::training_components_instruction()
      ), target_setup),                                                                                       /* jk_setup */
      cuwacunu::piaabo::dconfig::config_dtype                 (target_conf),
      cuwacunu::piaabo::dconfig::config_device                (target_conf),
      cuwacunu::piaabo::dconfig::config_space_t::get<int>     (target_conf, "grad_clip"),                     /* grad_clip */
      cuwacunu::piaabo::dconfig::config_space_t::get<int>     (target_conf, "optimizer_threshold_reset")      /* optimizer_threshold_reset */
    )
  {
    log_info("Initialized MdnModel from Configuration files...\n");
  }

  // -------- forward --------
  MdnOut forward(const torch::Tensor& x) {
    auto h = backbone->forward(x);
    return head->forward(h);
  }

  // -------- training loop (like VICReg) --------
  template <typename Q, typename KBatch, typename Td>
  std::vector<std::pair<int, double>> fit(
      cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Q, KBatch, Td, torch::data::samplers::SequentialSampler>& dataloader,
      int n_epochs = -1,
      int n_iters  = -1,
      bool verbose = false)
  {
    int epoch_count = 0;
    int iter_count  = 0;
    bool stop_loop  = false;
    std::vector<std::pair<int, double>> loss_log;

    this->train();

    while (!stop_loop) {
      if (n_epochs >= 0 && epoch_count >= n_epochs) break;

      double cum_loss = 0.0;
      int epoch_iters = 0;

      for (auto& sample_batch : dataloader.inner()) {
        if (n_iters >= 0 && iter_count >= n_iters) { stop_loop = true; break; }

        optimizer->zero_grad();

        auto collated = KBatch::collate_fn(sample_batch);
        auto x = collated.features.to(device, /*non_blocking=*/true); // [B, De]
        auto y = collated.targets.to(device,  /*non_blocking=*/true); // [B, Dy]

        TORCH_CHECK(x.dim() == 2 && x.size(1) == De, "[MDN::fit] x shape must be [B, De]");
        TORCH_CHECK(y.dim() == 2 && y.size(1) == Dy, "[MDN::fit] y shape must be [B, Dy]");

        auto out  = forward(x);
        auto loss = (*loss_obj)(out, y);

        loss.backward();

        if (grad_clip > 0.0) {
          torch::nn::utils::clip_grad_norm_(trainable_params_, grad_clip);
        }

        if (optimizer_threshold_reset >= 0) {
          cuwacunu::piaabo::torch_compat::optim::clamp_adam_step(
              *optimizer, static_cast<int64_t>(optimizer_threshold_reset));
        }

        optimizer->step();

        // per-batch scheduler
        if (lr_sched && lr_sched->mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerBatch)
          lr_sched->step();

        cum_loss += loss.template item<double>();
        ++epoch_iters;
        ++iter_count;
      }

      ++epoch_count;

      if (!stop_loop && epoch_iters > 0) {
        const double avg_loss = cum_loss / static_cast<double>(epoch_iters);

        // per-epoch scheduler (with/without metric)
        if (lr_sched) {
          using Mode = cuwacunu::jkimyei::LRSchedulerAny::Mode;
          if (lr_sched->mode == Mode::PerEpoch) {
            lr_sched->step();
          } else if (lr_sched->mode == Mode::PerEpochWithMetric) {
            lr_sched->step(avg_loss);
          }
        }

        if (verbose && (epoch_count == 1 || epoch_count % 50 == 0 || (n_epochs > 0 && epoch_count == n_epochs))) {
          double cur_lr = std::numeric_limits<double>::quiet_NaN();
          // best-effort LR display across common opts
          try {
            const auto& pg = optimizer->param_groups().front();
            // Try Adam
            if (auto* ao = dynamic_cast<torch::optim::AdamOptions*>(&pg.options())) {
              cur_lr = ao->lr();
            } else if (auto* so = dynamic_cast<torch::optim::SGDOptions*>(&pg.options())) {
              cur_lr = so->lr();
            }
          } catch (...) {}

          if (std::isfinite(cur_lr)) {
            log_info("%s MDN %s [ %sEpoch # %s%5d%s ] \t%slr = %s%.6f%s, \t%sloss = %s%.5f%s \n",
              ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET,
              ANSI_COLOR_Dim_Gray, ANSI_COLOR_Dim_Blue, epoch_count, ANSI_COLOR_RESET,
              ANSI_COLOR_Dim_Gray, ANSI_COLOR_Dim_Magenta, cur_lr, ANSI_COLOR_RESET,
              ANSI_COLOR_Dim_Gray, ANSI_COLOR_Dim_Red, avg_loss, ANSI_COLOR_RESET
            );
          } else {
            log_info("%s MDN %s [ %sEpoch # %s%5d%s ] \t%sloss = %s%.5f%s \n",
              ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET,
              ANSI_COLOR_Dim_Gray, ANSI_COLOR_Dim_Blue, epoch_count, ANSI_COLOR_RESET,
              ANSI_COLOR_Dim_Gray, ANSI_COLOR_Dim_Red, avg_loss, ANSI_COLOR_RESET
            );
          }
        }

        loss_log.push_back({epoch_count, avg_loss});
      }
    }

    return loss_log;
  }

  // -------- warmup (GPU graph init etc.) --------
  void warm_up() {
    if (device.is_cuda()) {
      constexpr int B = 2;
      auto x = torch::zeros({B, De}, torch::TensorOptions().dtype(dtype).device(device));
      (void)forward(x); // just one dry pass
      torch::cuda::synchronize();
    }
  }

  // inference placeholder
  std::vector<torch::Tensor> inference(const torch::Tensor& /*x0*/, const InferenceConfig& /*cfg*/) {
    return {};
  }

  // -------- pretty print --------
  void display_model() const {
    std::string dev = device.str();
    const char* fmt =
      "\n%s \t[MDN] %s\n"
      "\t\t%s%-25s%s %s%-8lld%s\n"  // De
      "\t\t%s%-25s%s %s%-8lld%s\n"  // Dy
      "\t\t%s%-25s%s %s%-8lld%s\n"  // K
      "\t\t%s%-25s%s %s%-8lld%s\n"  // feature_dim
      "\t\t%s%-25s%s %s%-8lld%s\n"  // depth
      "\t\t%s%-25s%s %s%-8s%s\n"    // optimizer id
      "\t\t%s%-25s%s %s%-8s%s\n"    // scheduler id
      "\t\t%s%-25s%s %s%-8s%s\n"    // loss id
      "\t\t%s%-25s%s %s%-8.3f%s\n"  // grad_clip
      "\t\t%s%-25s%s %s%-8s%s\n"    // device
      "\t\t%s%-25s%s %s%-8d%s\n";   // optimizer_threshold_reset
    log_info(fmt,
      ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET,
      ANSI_COLOR_Bright_Grey, "Input dims (De):", ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, De, ANSI_COLOR_RESET,
      ANSI_COLOR_Bright_Grey, "Target dims (Dy):", ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, Dy, ANSI_COLOR_RESET,
      ANSI_COLOR_Bright_Grey, "Mixture comps (K):", ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, K, ANSI_COLOR_RESET,
      ANSI_COLOR_Bright_Grey, "Feature dim:", ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, H, ANSI_COLOR_RESET,
      ANSI_COLOR_Bright_Grey, "Depth:",       ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, depth, ANSI_COLOR_RESET,
      ANSI_COLOR_Bright_Grey, "Optimizer:",   ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, jk_setup.opt_conf.id.c_str(), ANSI_COLOR_RESET,
      ANSI_COLOR_Bright_Grey, "LR Scheduler:",ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, jk_setup.sch_conf.id.c_str(), ANSI_COLOR_RESET,
      ANSI_COLOR_Bright_Grey, "Loss:",        ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, jk_setup.loss_conf.id.c_str(), ANSI_COLOR_RESET,
      ANSI_COLOR_Bright_Grey, "Grad clip:",   ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, grad_clip, ANSI_COLOR_RESET,
      ANSI_COLOR_Bright_Grey, "Device:",      ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, dev.c_str(), ANSI_COLOR_RESET,
      ANSI_COLOR_Bright_Grey, "Opt thresh reset:", ANSI_COLOR_RESET, ANSI_COLOR_Bright_Blue, optimizer_threshold_reset, ANSI_COLOR_RESET
    );
  }

  // -------- (re-added here) simple rollout for scalar Dy=1 --------
  // x_seq: [T,B,De], returns [H,B]
  torch::Tensor rollout_sample_scalar(const torch::Tensor& x_seq, int64_t H) {
    TORCH_CHECK(x_seq.dim() == 3, "x_seq must be [T,B,De]");
    auto T = x_seq.size(0);
    TORCH_CHECK(H <= T, "H must be <= T for this helper");
    TORCH_CHECK(Dy == 1, "rollout_sample_scalar expects Dy==1");

    std::vector<torch::Tensor> preds;
    preds.reserve(H);
    for (int64_t t = 0; t < H; ++t) {
      auto out = forward(x_seq[t]);          // [B,De] -> MdnOut
      auto y_t = mdn_sample_one_step(out);   // [B,1]
      preds.push_back(y_t.squeeze(-1));      // [B]
    }
    return torch::stack(preds, /*dim=*/0);   // [H,B]
  }
};

TORCH_MODULE(MdnModel);

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
