/* mixture_density_network.h
 *
 * Mixture Density Network with a shared backbone and per-channel, per-horizon
 * heads. This header declares the MdnModelImpl module (no training loop here).
 */
#pragma once
#include <algorithm>
#include <cmath>
#include <iostream>
#include <memory>
#include <string>
#include <torch/torch.h>
#include <vector>

#include "piaabo/dutils.h"
#include "wikimyei/inference/mdn/mixture_density_network_backbone.h"
#include "wikimyei/inference/mdn/mixture_density_network_head.h"
#include "wikimyei/inference/mdn/mixture_density_network_loss.h"
#include "wikimyei/inference/mdn/mixture_density_network_types.h"
#include "wikimyei/inference/mdn/mixture_density_network_utils.h"

namespace cuwacunu {
namespace wikimyei {
namespace mdn {

/**
 * # MdnModelImpl
 * Mixture Density Network with a **shared trunk** (Backbone) and **per-channel,
 * per-horizon heads** (ChannelHeads).
 *
 * ## What this file is (and isn't)
 * - **Pure architecture only**: no optimizer, scheduler, loss, or training loop
 * here.
 * - The *ExpectedValue* wrapper owns all training wiring and future/mask
 * shaping.
 * - This module models the predictive distribution p(F|X); the wrapper
 *   exposes E[F|X] in selected-future-feature space as its primary statistic.
 *
 * ## Shapes (by convention)
 * - Input encoding:
 *     - `[B, De]`                — single embedding per sample.
 *       Temporal sequence reduction belongs to the ExpectedValue wrapper, where
 *       the observation mask and reducer policy are available.
 * - Outputs (per batch, channel, horizon, mixture component, future feature):
 *     - `log_pi : [B, C, Hf, K]`
 *     - `mu     : [B, C, Hf, K, Df]`
 *     - `sigma  : [B, C, Hf, K, Df]` (positive; diagonal covariance)
 *
 * ## Key symbols
 * - `B`  : batch size
 * - `De` : input embedding dimension
 * - `Df` : selected future feature dimension (e.g., if predicting `{close}` then Df=1;
 * `{high,close}` then Df=2)
 * - `C`  : number of channels
 * - `Hf` : number of forecast horizons (time steps ahead)
 * - `K`  : number of mixture components
 * - `H`  : hidden feature width of the trunk
 * - `depth` : number of residual blocks in the trunk
 *
 * ## Typical call flow
 * ```
 * // encoding is already reduced to [B,De]
 * auto out = model->forward_from_encoding(encoding);
 * // out.{log_pi, mu, sigma} match shapes above
 * ```
 *
 * ## Gotchas / tips
 * - **Df must match your future feature selection**: If predict `{1,3}` from
 * `future_features[..., Dx]`, then construct the model with `Df=2` and make sure
 * the loss selects those same two dims.
 * - **C and Hf are architectural**: the head is built to output for *all*
 * channels and horizons. Pass the values intended to train/evaluate with.
 * - **Temporal reduction**: this class intentionally rejects `[B,Hx',De]`.
 *   Use ExpectedValue's explicit reducer policy before calling the MDN.
 */
struct MdnModelImpl : torch::nn::Module {
  // --- Architecture hyperparameters (immutable after construction)
  int64_t De{0};      ///< Input embedding dimension
  int64_t Df{0};      ///< Selected future width per (channel,horizon)
  int64_t C_axes{1};  ///< Number of channels (heads are replicated C times)
  int64_t Hf_axes{1}; ///< Forecast horizons per channel
  int64_t K{0};       ///< Mixture components
  int64_t H{0};       ///< Trunk hidden width
  int64_t depth{0};   ///< Trunk residual depth

  // --- Execution precision and device placement
  torch::Dtype dtype{torch::kFloat32};
  torch::Device device{torch::kCPU};

  // --- Submodules
  Backbone backbone{nullptr};     ///< Residual MLP trunk: [B,De] -> [B,H]
  ChannelHeads ch_heads{nullptr}; ///< Per-(C,Hf) MDN heads: [B,H] -> MdnOut

  /**
   * ## Constructor (explicit; recommended)
   * Build the architecture with explicit sizes (no config reads here).
   *
   * @param De_    Input embedding dim
   * @param Df_    Selected future dim per (channel,horizon)
   * @param C_     Number of channels (>=1)
   * @param Hf_    Number of forecast horizons per channel (>=1)
   * @param K_     Mixture components
   * @param H_     Trunk hidden width
   * @param depth_ Trunk residual depth (number of ResMLP blocks)
   * @param dtype_ Torch dtype for params and compute
   * @param device_ Torch device for the module
   */
  MdnModelImpl(int64_t De_, int64_t Df_, int64_t C_, int64_t Hf_, int64_t K_,
               int64_t H_, int64_t depth_,
               torch::Dtype dtype_ = torch::kFloat32,
               torch::Device device_ = torch::kCPU, bool display_model_ = true);

  /**
   * ## Temporal pooling guard for encodings
   * Accepts only `[B,De]`. Rank-3 sequences must be reduced before reaching the
   * pure MDN so the reduction policy cannot be implicit.
   */
  static torch::Tensor temporal_pool(const torch::Tensor &enc);

  /**
   * Forward from a single-step embedding `[B,De]`.
   */
  MdnOut forward(const torch::Tensor &x);

  /**
   * Forward from encoding `[B,De]`. Rank-3 encodings are rejected.
   */
  MdnOut forward_from_encoding(const torch::Tensor &encoding);

  /**
   * Convenience: conditional expectation E[F|X] directly from encoding
   * (expects an already-reduced `[B,De]` tensor).
   */
  torch::Tensor expectation_from_encoding(const torch::Tensor &encoding);

  /**
   * Lightweight warmup to initialize CUDA kernels / allocator paths.
   * No-op on CPU. Safe to remove if you don’t care about first-iteration
   * latency.
   */
  void warm_up();

  /// Hard assertion that all MDN tensors reside on the configured device.
  void assert_resident_on_device_or_throw(const char *context) const;

  /// Hard assertion plus a CUDA forward/expectation probe when device is CUDA.
  void verify_ready_on_device_or_throw(const char *context);

  /// Placeholder for any future inference utilities (sampling, etc.).
  std::vector<torch::Tensor> inference(const torch::Tensor & /*enc*/,
                                       const InferenceConfig & /*cfg*/);

  /// Pretty-print the current architecture and placement.
  void display_model() const;
};

TORCH_MODULE(MdnModel);

} // namespace mdn
} // namespace wikimyei
} // namespace cuwacunu
