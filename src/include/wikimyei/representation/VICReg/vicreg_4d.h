/* vicreg_4d.h */
#pragma once

#include <torch/torch.h>
#include <torch/nn/utils/clip_grad.h>
#include <optional>
#include <vector>
#include <type_traits>

#include "piaabo/dconfig.h"

#include "wikimyei/representation/VICReg/vicreg_4d_types.h"
#include "wikimyei/representation/VICReg/vicreg_4d_losses.h"
#include "wikimyei/representation/VICReg/vicreg_4d_encoder.h"
#include "wikimyei/representation/VICReg/vicreg_4d_projector.h"
#include "wikimyei/representation/VICReg/vicreg_4d_averagedModel.h"
#include "wikimyei/representation/VICReg/vicreg_4d_augmentations.h"
#include "wikimyei/representation/VICReg/vicreg_4d_augmentations_utils.h"

#include "wikimyei/representation/VICReg/vicreg_4d_dataset.h"
#include "wikimyei/representation/VICReg/vicreg_4d_dataloader.h"

#include "piaabo/torch_compat/optim/optimizers.h"
#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_dataset.h"
#include "camahjucunu/data/memory_mapped_datafile.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "camahjucunu/dsl/observation_pipeline/observation_pipeline.h"
#include "camahjucunu/dsl/jkimyei_specs/jkimyei_specs.h"
#include "jkimyei/training_setup/jk_setup.h"

RUNTIME_WARNING("(vicreg_4d.h)[] TRAIN VICReg to the recostruction objective; or at least flag it or cohefficient it; [IMPORTANT].\n");
RUNTIME_WARNING("(vicreg_4d.h)[] for improving performance remember doing torch::jit::Module scripted = torch::jit::freeze(torch::jit::script::Module(my_encoder));\n");

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

namespace cuwacunu {
namespace wikimyei {
namespace vicreg_4d {

/**
 * @class VICReg_4D
 * @brief Self-supervised encoder trained with the VICReg objective on rank-4 inputs [B, C, T, D].
 *
 * @details
 *  The model processes mini-batches of multichannel time-series (or similar) tensors:
 *    - B: batch size
 *    - C: input channels
 *    - T: timesteps
 *    - D: per-timestep feature dimension
 *
 *  Major components:
 *   - _encoder_net
 *       Feature extractor that maps [B, C, T, D] → latent representations.
 *   - _swa_encoder_net
 *       Averaged copy of the encoder (SWA/EMA). Used for evaluation and export.
 *       If @ref enable_buffer_averaging is true, running buffers (e.g., BN stats)
 *       are averaged; otherwise they are copied.
 *   - _projector_net
 *       MLP head used only during training to compute the VICReg loss in the
 *       projector space. Not required for downstream inference.
 *   - aug
 *       Stochastic augmentations (e.g., time warps and random drops) used to form
 *       two correlated views per input for the VICReg objective.
 *
 *  Training wiring:
 *   - @ref optimizer updates encoder + projector parameters.
 *   - @ref lr_sched is an abstract LR scheduler that may tick per-batch or per-epoch.
 *   - @ref loss_obj computes the VICReg loss (invariance/variance/covariance terms).
 *
 *  Device/precision policy:
 *   - @ref device and @ref dtype indicate the intended compute placement and precision.
 *     Submodules are constructed to honor these. Public encode() methods accept tensors
 *     on any device and move them as needed.
 *
 *  Checkpointing:
 *   - save() writes submodule weights and minimal metadata needed to reconstruct
 *     the module and its training setup.
 *   - load() hydrates submodule weights and, if available, optimizer state.
 */
class VICReg_4D : public torch::nn::Module {
public:
  std::string component_name;
  // ── Model shape (input contract) and training setup ──────────────────────────
  int C;                                  // Number of input channels.
  int T;                                  // Number of timesteps.
  int D;                                  // Number of per-timestep features.

  // ── Encoder & projector hyperparameters (topology) ───────────────────────────
  int encoding_dims;          // Latent representation width produced by the encoder.
  int channel_expansion_dim;  // Intermediate channel expansion within the encoder.
  int fused_feature_dim;      // Size of fused feature vector after spatiotemporal fusion.
  int encoder_hidden_dims;    // Hidden width used inside encoder residual blocks.
  int encoder_depth;          // Number of residual blocks in the encoder.
  std::string projector_mlp_spec; // Projector MLP layout (e.g., "8192-8192-8192").

  // ── Execution policy ─────────────────────────────────────────────────────────
  torch::Dtype  dtype;        // Floating-point precision for parameters and computation.
  torch::Device device;       // Target device for parameters and forward passes.
  int  optimizer_threshold_reset; // Adam/AdamW moment-exponent clamp step; -1 disables.
  bool enable_buffer_averaging;   // If true, SWA averages buffers; otherwise copies them.

  // ── Submodules (registered) ──────────────────────────────────────────────────
  VICReg_4D_Encoder               _encoder_net{nullptr};        // Trainable base encoder.
  StochasticWeightAverage_Encoder _swa_encoder_net{nullptr};    // Averaged encoder for eval/export.
  VICReg_4D_Projector             _projector_net{nullptr};      // Training-only projector head.
  VICReg_4D_Augmentation          aug;                          // Stochastic data augmentations.

  // ── Training plumbing ────────────────────────────────────────────────────────
  std::vector<at::Tensor>                            trainable_params_; // Stable list passed to optimizer.
  std::unique_ptr<torch::optim::Optimizer>           optimizer;         // Optimizer over @ref trainable_params_.
  std::unique_ptr<cuwacunu::jkimyei::LRSchedulerAny> lr_sched;          // Type-erased LR scheduler.
  std::unique_ptr<VicRegLoss>                        loss_obj;          // VICReg loss computation.

  // --- constructors (decl only; bodies in .cpp) ---
  VICReg_4D(
    const std::string& component_name_,
    int C_, int T_, int D_,
    int encoding_dims_, 
    int channel_expansion_dim_, 
    int fused_feature_dim_, 
    int encoder_hidden_dims_, 
    int encoder_depth_, 
    std::string projector_mlp_spec_, 
    torch::Dtype dtype_ = torch::kFloat32,
    torch::Device device_ = torch::kCPU,
    int optimizer_threshold_reset_ = -1,
    bool enable_buffer_averaging_ = false
  );

  // config-driven delegating constructor
  VICReg_4D(
    const std::string& component_name, 
    int C_, int T_, int D_
  );

  // strict checkpoint constructor
  VICReg_4D(const std::string& checkpoint_path,
            torch::Device override_device = torch::kCPU);

  // --- training (template stays in header) ---
  template<typename Dataset_t, typename Datasample_t, typename Datatype_t>
  std::vector<std::pair<int, double>> fit(
    cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Datasample_t, Datatype_t, torch::data::samplers::SequentialSampler>& dataloader,
    int n_epochs = -1,
    int n_iters = -1,
    int swa_start_iter = 1000,
    bool verbose = false
  ) {
    int epoch_count = 0;
    int iter_count  = 0;
    bool stop_loop  = false;
    std::vector<std::pair<int, double>> loss_log;

    _encoder_net->train();
    _projector_net->train();
    _swa_encoder_net->encoder().train();

    std::vector<torch::Tensor> all_p;
    for (auto& p : _encoder_net->parameters())   all_p.push_back(p);
    for (auto& p : _projector_net->parameters()) all_p.push_back(p);

    while (!stop_loop) {
      if (n_epochs >= 0 && epoch_count >= n_epochs) break;

      double cum_loss = 0.0;
      int epoch_iters = 0;

      for (auto& sample_batch : dataloader.inner()) {
        if (n_iters >= 0 && iter_count >= n_iters) { stop_loop = true; break; }

        optimizer->zero_grad();

        auto collated_sample = Datasample_t::collate_fn_past(sample_batch);
        auto data = collated_sample.features.to(device);
        auto mask = collated_sample.mask.to(device);

        TORCH_CHECK(!data.requires_grad() && !data.grad_fn(),
            "[VICReg_4D::fit] data must not require grad");

        TORCH_CHECK(!mask.requires_grad() && !mask.grad_fn(),
            "[VICReg_4D::fit] mask must not require grad");

        TORCH_CHECK(data.dim() == 4,
            "[VICReg_4D::fit] data.dim()=" + std::to_string(data.dim()) +
            " (expected 4: [B,C,T,D])");

        TORCH_CHECK(data.size(1) == C && data.size(2) == T && data.size(3) == D,
            "[VICReg_4D::fit] data shape mismatch: "
            "got [C=" + std::to_string(data.size(1)) +
            ",T=" + std::to_string(data.size(2)) +
            ",D=" + std::to_string(data.size(3)) +
            "], expected [C=" + std::to_string(C) +
            ",T=" + std::to_string(T) +
            ",D=" + std::to_string(D) + "]");

        TORCH_CHECK(mask.dim() == 3,
            "[VICReg_4D::fit] mask.dim()=" + std::to_string(mask.dim()) +
            " (expected 3: [B,C,T])");

        TORCH_CHECK(mask.size(1) == C && mask.size(2) == T,
            "[VICReg_4D::fit] mask shape mismatch: "
            "got [C=" + std::to_string(mask.size(1)) +
            ",T=" + std::to_string(mask.size(2)) +
            "], expected [C=" + std::to_string(C) +
            ",T=" + std::to_string(T) + "]");

        // ── Augment two views
        auto [d1,m1] = aug.augment(data, mask);
        auto [d2,m2] = aug.augment(data, mask);

        // Check difference on the shared-valid region (robust under masking)
        if (verbose && (iter_count % 100 == 0)) {
          auto shared_bt   = (m1 & m2).all(/*dim=*/1);                    // [B,T]
          auto shared_mask = shared_bt.unsqueeze(1).unsqueeze(-1)         // [B,1,T,1]
                                      .expand_as(d1);                     // [B,C,T,D]
          const double diff = (d1.masked_select(shared_mask)
                             - d2.masked_select(shared_mask))
                                .abs().mean().template item<double>();
          TORCH_CHECK(diff > 1e-6, "[VICReg] augment produced identical views on shared valid region; check config.");
        }

        // ── Encode
        auto k1 = _encoder_net(d1, m1);   // encoder output
        auto k2 = _encoder_net(d2, m2);
        
        // ── Project
        // ── Select only positions valid in BOTH views (strict: all channels must be valid)
        auto valid_bt = (m1 & m2).all(/*dim=*/1);                // [B, T]

        // gather only the rows that are valid in both views
        auto expand_E = [&](int64_t E){ return valid_bt.unsqueeze(-1).expand({k1.size(0), k1.size(1), E}); };
        auto k1v = k1.masked_select(expand_E(k1.size(-1))).view({-1, k1.size(-1)}); // [N_eff, E]
        auto k2v = k2.masked_select(expand_E(k2.size(-1))).view({-1, k2.size(-1)});

        const int64_t N_eff = k1v.size(0);
        if (N_eff <= 1) {
          // still too small to estimate covariance — skip this mini-step
          continue;
        }

        // Project only valid rows → BN sees a consistent distribution
        auto z1v = _projector_net->forward_flat(k1v);            // [N_eff, Dz]
        auto z2v = _projector_net->forward_flat(k2v);            // [N_eff, Dz]

        // // (optional diagnostics — after we know N_eff > 1)
        // if (verbose && (iter_count % 50 == 0)) {
        //   auto z = z1v;
        //   auto zc = z - z.mean(0);
        //   auto stdz = torch::sqrt(zc.var(0, /*unbiased=*/false) + 1e-4);
        //   auto zw = zc / (stdz + 1e-4);
        //   auto corr = (zw.t().mm(zw)) / (zw.size(0) - 1);       // [E,E] correlation-like
        //   const double mean_abs_off_corr =
        //     off_diagonal(corr).abs().mean().template item<double>();
        //   log_info("[proj] mean|off-corr|=%.4f\n", mean_abs_off_corr);
        // }

        // ── VICReg loss terms
        auto terms = loss_obj->forward_terms(z1v, z2v);

        // Short early boost for covariance term (ramps from 3.0 → 1.0 over 3k iters)
        constexpr int cov_ramp_iters = 3000;
        double cov_boost = (iter_count < cov_ramp_iters)
                         ? (3.0 - 2.0 * (static_cast<double>(iter_count) / cov_ramp_iters))
                         : 1.0;

        // Compose the exact loss that is backpropped
        auto loss = loss_obj->sim_coeff_ * terms.inv
                  + loss_obj->std_coeff_ * terms.var
                  + (loss_obj->cov_coeff_ * cov_boost) * terms.cov;

        if (verbose && (iter_count % 500 == 0)) {
          const double loss_scalar = loss.template item<double>();
          log_info("[loss] optim=%.6f inv=%.6f var=%.6f cov=%.6f (cov_boost=%.2f)\n",
                   loss_scalar,
                   terms.inv.template item<double>(),
                   terms.var.template item<double>(),
                   terms.cov.template item<double>(),
                   cov_boost);
        }

        loss.backward();

        // Grad clip schedule: relaxed early, tighten later
        {
          std::vector<torch::Tensor> clip_vec;
          clip_vec.reserve(all_p.size());
          for (auto& p : all_p) {
            auto g = p.grad();
            if (g.defined()) clip_vec.push_back(p);
          }
          if (!clip_vec.empty()) {
            double clip_max = (iter_count < 1500) ? 5.0 : 1.0;
            torch::nn::utils::clip_grad_norm_(clip_vec, clip_max);
          }
        }

        optimizer->step();

        if (lr_sched && lr_sched->mode == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerBatch)
          lr_sched->step();

        if (iter_count >= swa_start_iter)
          _swa_encoder_net->update_parameters(_encoder_net);

        cum_loss += loss.template item<double>();
        ++epoch_iters;
        ++iter_count;
      }

      ++epoch_count;

      if (!stop_loop && epoch_iters > 0) {
        if (optimizer_threshold_reset >= 0) {
          cuwacunu::piaabo::torch_compat::optim::clamp_adam_step(
              *optimizer, static_cast<int64_t>(optimizer_threshold_reset));
        }

        const double avg_train_loss = cum_loss / static_cast<double>(epoch_iters);
        const double metric = avg_train_loss;

        if (lr_sched) {
          const auto m = lr_sched->mode;
          if      (m == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerEpoch)            lr_sched->step();
          else if (m == cuwacunu::jkimyei::LRSchedulerAny::Mode::PerEpochWithMetric)  lr_sched->step(metric);
        }

        if (verbose && (epoch_count == 1 || epoch_count % 50 == 0 || epoch_count == n_epochs)) {
          log_info("%s Representation Learning %s [ %sEpoch # %s%5d%s ] epoch_iters: %d \t%slr = %s%.6f%s, \t%sloss = %s%.5f%s \n",
            ANSI_COLOR_Dim_Green, ANSI_COLOR_RESET,
            ANSI_COLOR_Dim_Gray, ANSI_COLOR_Dim_Blue, epoch_count, ANSI_COLOR_RESET,
            epoch_iters,
            ANSI_COLOR_Dim_Gray, ANSI_COLOR_Dim_Magenta, optimizer->param_groups()[0].options().get_lr(), ANSI_COLOR_RESET,
            ANSI_COLOR_Dim_Gray, ANSI_COLOR_Dim_Red, metric, ANSI_COLOR_RESET
          );
        }

        loss_log.push_back({epoch_count, metric});
      }
    }

    return loss_log;
  }

  // --- non-template API (decl only; impl in .cpp) ---
  void warm_up();

  torch::Tensor encode(
    const torch::Tensor& data,
    const torch::Tensor& mask,
    bool use_swa = true,
    bool detach_to_cpu = false
  );

  torch::Tensor encode_projected(
    const torch::Tensor& data,
    const torch::Tensor& mask,
    bool use_swa = true,
    bool detach_to_cpu = false
  );

  // Convenience wrappers enabled only for observation_sample_t
  template<
    typename Datasample_t,
    std::enable_if_t<
      std::is_same_v<Datasample_t, cuwacunu::camahjucunu::data::observation_sample_t>, int
    > = 0
  >
  Datasample_t encode(
    Datasample_t batch,                       // by value: attach encoding and return
    bool use_swa = true,
    bool detach_to_cpu = false)
  {
    batch.encoding = this->encode(
      batch.features,
      batch.mask,
      use_swa,
      detach_to_cpu
    );
    return batch;
  }

  template<
    typename Datasample_t,
    std::enable_if_t<
      std::is_same_v<Datasample_t, cuwacunu::camahjucunu::data::observation_sample_t>, int
    > = 0
  >
  torch::Tensor encode_projected(
    const Datasample_t& batch,
    bool use_swa = true,
    bool detach_to_cpu = false)
  {
    return this->encode_projected(
      batch.features,
      batch.mask,
      use_swa,
      detach_to_cpu
    );
  }

  template<typename Dataset_t, typename Datasample_t, typename Datatype_t, typename Sampler = torch::data::samplers::SequentialSampler>
  auto make_representation_dataloader(
    cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Datasample_t, Datatype_t, Sampler>& raw_loader,
    bool use_swa = true, bool debug = false
  ) {
    return RepresentationDataloaderView<VICReg_4D, Dataset_t, Datasample_t, Datatype_t, Sampler>(raw_loader, *this, use_swa, debug);
  }

  template<typename Dataset_t, typename Datasample_t, typename Datatype_t>
  auto make_representation_dataset(
    Dataset_t& raw_dataset,
    torch::Device device, bool use_swa = true, bool detach_to_cpu = true
  ) {
    return RepresentationDatasetView<VICReg_4D, Dataset_t, Datasample_t, Datatype_t>(raw_dataset, *this, device, use_swa, detach_to_cpu);
  }

  void save(const std::string& filepath);
  void load(const std::string& filepath);
  void display_model() const;
};

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu
