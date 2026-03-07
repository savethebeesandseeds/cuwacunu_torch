/* vicreg_4d.h */
#pragma once

#include <torch/torch.h>

#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include "piaabo/dconfig.h"
#include "piaabo/torch_compat/network_analytics.h"

#include "wikimyei/representation/VICReg/vicreg_4d_types.h"
#include "wikimyei/representation/VICReg/vicreg_4d_losses.h"
#include "wikimyei/representation/VICReg/vicreg_4d_encoder.h"
#include "wikimyei/representation/VICReg/vicreg_4d_projector.h"
#include "wikimyei/representation/VICReg/vicreg_4d_averagedModel.h"
#include "wikimyei/representation/VICReg/vicreg_4d_augmentations.h"

#include "wikimyei/representation/VICReg/vicreg_4d_dataset.h"
#include "wikimyei/representation/VICReg/vicreg_4d_dataloader.h"

#include "jkimyei/optim/optimizer_utils.h"
#include "camahjucunu/data/observation_sample.h"
#include "camahjucunu/data/memory_mapped_dataloader.h"
#include "jkimyei/training_setup/jk_setup.h"

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
 *       Feature extractor that maps [B, C, T, D] -> latent representations.
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
  struct training_policy_t {
    bool vicreg_use_swa{true};
    bool vicreg_detach_to_cpu{false};
    int swa_start_iter{1000};
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
    double pending_loss_sum{0.0};
    int pending_loss_count{0};
    double last_committed_loss_mean{0.0};
  };

  cuwacunu::iitepi::contract_hash_t contract_hash;
  std::string component_name;

  // -- Model shape (input contract) and training setup --
  int C;                                  // Number of input channels.
  int T;                                  // Number of timesteps.
  int D;                                  // Number of per-timestep features.

  // -- Encoder & projector hyperparameters (topology) --
  int encoding_dims;          // Latent representation width produced by the encoder.
  int channel_expansion_dim;  // Intermediate channel expansion within the encoder.
  int fused_feature_dim;      // Size of fused feature vector after spatiotemporal fusion.
  int encoder_hidden_dims;    // Hidden width used inside encoder residual blocks.
  int encoder_depth;          // Number of residual blocks in the encoder.
  std::string projector_mlp_spec; // Projector MLP layout (e.g., "8192-8192-8192").
  ProjectorOptions effective_projector_options{}; // Effective projector options after optional design override.

  // -- Execution policy --
  torch::Dtype  dtype;        // Floating-point precision for parameters and computation.
  torch::Device device;       // Target device for parameters and forward passes.
  int  optimizer_threshold_reset; // Adam/AdamW moment-exponent clamp step; -1 disables.
  bool enable_buffer_averaging;   // If true, SWA averages buffers; otherwise copies them.
  std::string network_design_network_id{};  // Optional network_design id used to build this model.
  std::string network_design_join_policy{}; // Optional network_design join policy.
  std::string network_design_grammar{};     // Optional network_design grammar text.
  std::string network_design_dsl{};         // Optional network_design DSL text.

  // -- Submodules (registered) --
  VICReg_4D_Encoder               _encoder_net{nullptr};        // Trainable base encoder.
  StochasticWeightAverage_Encoder _swa_encoder_net{nullptr};    // Averaged encoder for eval/export.
  VICReg_4D_Projector             _projector_net{nullptr};      // Training-only projector head.
  VICReg_4D_Augmentation          aug;                          // Stochastic data augmentations.

  // -- Training plumbing --
  std::vector<at::Tensor>                            trainable_params_; // Stable list passed to optimizer.
  std::unique_ptr<torch::optim::Optimizer>           optimizer;         // Optimizer over @ref trainable_params_.
  std::unique_ptr<cuwacunu::jkimyei::LRSchedulerAny> lr_sched;          // Type-erased LR scheduler.
  std::unique_ptr<VicRegLoss>                        loss_obj;          // VICReg loss computation.
  training_policy_t training_policy{};
  runtime_state_t runtime_state{};

  // --- constructors (decl only; bodies in .cpp) ---
  struct init_from_contract_t {
    int encoding_dims{0};
    int channel_expansion_dim{0};
    int fused_feature_dim{0};
    int encoder_hidden_dims{0};
    int encoder_depth{0};
    std::string projector_mlp_spec{};
    torch::Dtype dtype{torch::kFloat32};
    torch::Device device{torch::kCPU};
    int optimizer_threshold_reset{-1};
    bool enable_buffer_averaging{false};
    std::optional<ProjectorOptions> projector_options_override{};
    std::string network_design_network_id{};
    std::string network_design_join_policy{};
    std::string network_design_grammar{};
    std::string network_design_dsl{};
  };

  VICReg_4D(
    const cuwacunu::iitepi::contract_hash_t& contract_hash_,
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
    bool enable_buffer_averaging_ = false,
    std::optional<ProjectorOptions> projector_options_override_ = std::nullopt,
    std::string network_design_network_id_ = {},
    std::string network_design_join_policy_ = {},
    std::string network_design_grammar_ = {},
    std::string network_design_dsl_ = {}
  );

  VICReg_4D(
    const cuwacunu::iitepi::contract_hash_t& contract_hash_,
    const std::string& component_name_,
    int C_, int T_, int D_,
    init_from_contract_t resolved_
  );

  // config-driven delegating constructor
  VICReg_4D(
    const cuwacunu::iitepi::contract_hash_t& contract_hash_,
    const std::string& component_name,
    int C_, int T_, int D_
  );

  // strict checkpoint constructor
  VICReg_4D(const cuwacunu::iitepi::contract_hash_t& contract_hash_,
            const std::string& checkpoint_path,
            torch::Device override_device = torch::kCPU);

  struct train_step_result_t {
    torch::Tensor loss{};
    bool optimizer_step_applied{false};
    bool skipped{false};
  };

  void reset_runtime_training_state();
  [[nodiscard]] train_step_result_t train_one_batch(
      const torch::Tensor& data,
      const torch::Tensor& mask,
      int swa_start_iter = -1,
      bool verbose = false);
  [[nodiscard]] bool finalize_pending_training_step(int swa_start_iter = -1);
  [[nodiscard]] int runtime_optimizer_steps() const noexcept {
    return runtime_state.optimizer_steps;
  }

  // --- training (template stays in header) ---
  template<typename Dataset_t, typename Datasample_t, typename Datatype_t>
  std::vector<std::pair<int, double>> fit(
    cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Datasample_t, Datatype_t, torch::data::samplers::SequentialSampler>& dataloader,
    int n_epochs = -1,
    int n_iters = -1,
    int swa_start_iter = -1,
    bool verbose = false
  ) {
    int epoch_count = 0;
    int iter_count  = 0;
    bool stop_loop  = false;
    std::vector<std::pair<int, double>> loss_log;

    const int effective_swa_start_iter =
        (swa_start_iter >= 0) ? swa_start_iter : training_policy.swa_start_iter;
    clear_pending_runtime_state_(/*reset_optimizer_steps=*/false);

    while (!stop_loop) {
      if (n_epochs >= 0 && epoch_count >= n_epochs) break;

      double cum_loss = 0.0;
      int epoch_iters = 0;

      for (auto& sample_batch : dataloader.inner()) {
        if (n_iters >= 0 && iter_count >= n_iters) { stop_loop = true; break; }

        auto collated_sample = Datasample_t::collate_fn_past(sample_batch);
        auto step_result = train_one_batch(
            collated_sample.features,
            collated_sample.mask,
            effective_swa_start_iter,
            verbose);
        if (!step_result.optimizer_step_applied) continue;

        cum_loss += runtime_state.last_committed_loss_mean;
        ++epoch_iters;
        ++iter_count;
      }

      if (!stop_loop && runtime_state.has_pending_grad) {
        if (n_iters < 0 || iter_count < n_iters) {
          if (finalize_pending_training_step(effective_swa_start_iter)) {
            cum_loss += runtime_state.last_committed_loss_mean;
            ++epoch_iters;
            ++iter_count;
          }
        } else {
          clear_pending_runtime_state_(/*reset_optimizer_steps=*/false);
        }
      }

      ++epoch_count;

      if (!stop_loop && epoch_iters > 0) {
        if (optimizer_threshold_reset >= 0) {
          cuwacunu::jkimyei::optim::clamp_adam_step(
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

        const auto analytics =
            cuwacunu::piaabo::torch_compat::summarize_module_network_analytics(
                *this);
        log_info(
            "[VICReg::network_analytics] epoch=%d finite=%.6f std=%.6f near_zero=%.6f entropy=%.6f tensor_cv=%.6f max_abs=%.6f\n",
            epoch_count,
            analytics.finite_ratio,
            analytics.stddev,
            analytics.near_zero_ratio,
            analytics.abs_energy_entropy,
            analytics.tensor_rms_cv,
            analytics.max_abs);

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

  template<
    typename Datasample_t,
    std::enable_if_t<
      std::is_same_v<Datasample_t, cuwacunu::camahjucunu::data::observation_sample_t>, int
    > = 0
  >
  Datasample_t encode(
    Datasample_t batch,
    bool use_swa = true,
    bool detach_to_cpu = false);

  template<
    typename Datasample_t,
    std::enable_if_t<
      std::is_same_v<Datasample_t, cuwacunu::camahjucunu::data::observation_sample_t>, int
    > = 0
  >
  torch::Tensor encode_projected(
    const Datasample_t& batch,
    bool use_swa = true,
    bool detach_to_cpu = false);

  template<typename Dataset_t, typename Datasample_t, typename Datatype_t, typename Sampler = torch::data::samplers::SequentialSampler>
  RepresentationDataloaderView<VICReg_4D, Dataset_t, Datasample_t, Datatype_t, Sampler>
  make_representation_dataloader(
    cuwacunu::camahjucunu::data::MemoryMappedDataLoader<Dataset_t, Datasample_t, Datatype_t, Sampler>& raw_loader,
    bool use_swa = true,
    bool debug = false);

  template<typename Dataset_t, typename Datasample_t, typename Datatype_t>
  RepresentationDatasetView<VICReg_4D, Dataset_t, Datasample_t, Datatype_t>
  make_representation_dataset(
    Dataset_t& raw_dataset,
    torch::Device device,
    bool use_swa = true,
    bool detach_to_cpu = true);

  void save(const std::string& filepath);
  void load(const std::string& filepath);
  void load_jkimyei_training_policy(const cuwacunu::jkimyei::jk_component_t& jk_component);
  void display_model() const;

private:
  void clear_pending_runtime_state_(bool reset_optimizer_steps);
  [[nodiscard]] std::vector<torch::Tensor> params_with_grad_() const;
  void apply_gradient_clipping_(const std::vector<torch::Tensor>& params_with_grad) const;
  [[nodiscard]] bool grads_are_finite_(const std::vector<torch::Tensor>& params_with_grad) const;
  [[nodiscard]] bool commit_pending_training_step_(int swa_start_iter);
};

} // namespace vicreg_4d
} // namespace wikimyei
} // namespace cuwacunu

#include "wikimyei/representation/VICReg/vicreg_4d_adapters.h"
